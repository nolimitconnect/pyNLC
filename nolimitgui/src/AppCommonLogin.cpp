//============================================================================
// Copyright (C) 2018 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppCommon.h"	

#include "AccountMgr.h"
#include "AppletCreateAccount.h"
#include "AppletLaunchPage.h"
#include "AppSettings.h"
#include "AppletMgr.h"
#include "GuiHelpers.h"
#include "GuiRandConnectMgr.h"
#include "HomeWindow.h"

#include <P2PEngine/P2PEngine.h>
#include <P2PEngine/EngineSettings.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxParse.h>
#include <CoreLib/VxPtopUrl.h>

#include <GuiInterface/ICamCapture.h>

#include <QTimer>
#include <QUuid>
#include <QCoreApplication>

//============================================================================
static uint64_t getQuuidLoPart( QUuid& uuid )
{
    uint64_t u64LoPart;
    memcpy( &u64LoPart, &uuid, sizeof( uint64_t ) );
    return u64LoPart;
}

//============================================================================
static uint64_t getQuuidHiPart( QUuid& uuid )
{
    uint64_t u64HiPart;
    char * pTmp = (char *)&uuid;
    pTmp += 8;
    memcpy( &u64HiPart, pTmp, sizeof( uint64_t ) );
    return u64HiPart;
}

//============================================================================
// updates my ident in database and engine and global ident
void AppCommon::updateMyIdent( VxNetIdent* myIdent, bool permissionAndStateOnly )
{
    if( myIdent )
    {
        if( myIdent != getAppGlobals().getMyNetIdent() )
        {
            memcpy( getAppGlobals().getMyNetIdent(), myIdent, sizeof( VxNetIdent ) );
            myIdent = getAppGlobals().getMyNetIdent();
        }

       getAccountMgr().updateAccount( *myIdent );
       getEngine().fromGuiUpdateMyIdent( myIdent, permissionAndStateOnly );
    }
}

//============================================================================
void AppCommon::doLogin()
{
    bool bLastUserAccountLoaded = loadLastUserAccount();
    if( bLastUserAccountLoaded )
    {
        doAccountStartup();
    }
    else
    {
        applySoundSettings( true );  
        // user needs to create login and profile
        showCreateAccount();
    }
}

//============================================================================
void AppCommon::doAccountStartup( void )
{
    const uint64_t startupBeginMs = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "doAccountStartup begin at %llu ms", startupBeginMs );

    // tell engine were to load settings from
    loadAccountSpecificSettings( getAppGlobals().getMyNetIdent()->getOnlineName() );
    uint64_t startMs = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "doAccountStartup loadAccountSpecificSettings took %llu ms",
               startMs - startupBeginMs );

    getAppGlobals().getMyNetIdent()->setHasSharedWebCam( false ); // user must restart cam server each startup.. assume no shared cam yet

    applySoundSettings();
    uint64_t afterSoundMs = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "doAccountStartup applySoundSettings took %llu ms", afterSoundMs - startMs );

    completeLogin();
    uint64_t afterCompleteLoginMs = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "doAccountStartup completeLogin took %llu ms",
               afterCompleteLoginMs - afterSoundMs );

    // Apply network host settings on the next event-loop cycle so startup/login UI is not blocked.
    QTimer::singleShot( 0, this, SLOT(slotApplyStartupSettingsToEngine()) );
    LogModule( eLogStartup, LOG_VERBOSE, "doAccountStartup deferred sendAppSettingsToEngine at %llu ms", afterCompleteLoginMs );

    QTimer::singleShot( 1000, this, SLOT(slotAfterLoginComplete()) );

    uint64_t endMs = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "doAccountStartup total %llu ms (post-load %llu ms) alive %llu ms",
               endMs - startupBeginMs, endMs - startMs, endMs );
}

//============================================================================
void AppCommon::slotApplyStartupSettingsToEngine( void )
{
    const uint64_t beginMs = GetApplicationAliveMs();
    sendAppSettingsToEngine();
    const uint64_t endMs = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "slotApplyStartupSettingsToEngine took %llu ms at %llu ms",
               endMs - beginMs, endMs );
}

//============================================================================
void AppCommon::completeLogin( void )
{
    VxNetIdent* netIdent = getAppGlobals().getMyNetIdent();
    getEngine().fromGuiUserLoggedOn( netIdent );
    setLoginCompleted( true );

    m_UserMgr.updateMyIdent( netIdent );

    onUserLoggedOn();

    if( netIdent->getPluginPermission( ePluginTypeCamServer ) != eFriendStateIgnore &&
        m_AppSettings.getRunOnStartupCamServer() )
    {
        getFromGuiInterface().fromGuiStartPluginSession( ePluginTypeCamServer, netIdent->getMyOnlineId(), netIdent->getMyOnlineId() );
    }

    if( netIdent->getPluginPermission( ePluginTypeFileShareServer ) != eFriendStateIgnore &&
        m_AppSettings.getRunOnStartupFileShareServer() )
    {
        getFromGuiInterface().fromGuiStartPluginSession( ePluginTypeFileShareServer, netIdent->getMyOnlineId(), netIdent->getMyOnlineId() );
    }
}

//============================================================================
//! load last successful user account
bool AppCommon::loadLastUserAccount( void )
{
    m_strAccountUserName = m_AccountMgr.getLastLogin();

    if( 0 != m_strAccountUserName.length() )
    {
        // get identity out of database
        if( true == m_AccountMgr.getAccountByName( m_strAccountUserName.c_str(), *getAppGlobals().getMyNetIdent() ) )
        {
            getUserMgr().updateMyIdent( getAppGlobals().getMyNetIdent() );
            return true;
        }
        else
        {
            // remove old missing or corrupted account 
            m_AccountMgr.removeAccountByName( m_strAccountUserName.c_str() );
            LogMsg( LOG_INFO, "AppCommon:Could not retrieve user" );
        }
    }

    return false;
}

//============================================================================
void AppCommon::createAccountForUser( std::string& strUserName, VxNetIdent& userAccountIdent, const char* moodMsg, int gender, EAgeType age, int primaryLanguage, int contentType )
{
    QUuid uuidTmp = QUuid::createUuid();
    uint64_t u64HiPart = getQuuidHiPart( uuidTmp );
    uint64_t u64LoPart = getQuuidLoPart( uuidTmp );
    userAccountIdent.m_DirectConnectId.setVxGUID( u64HiPart, u64LoPart );

    SafeStrCopy( userAccountIdent.getOnlineName(), strUserName.c_str(), MAX_ONLINE_NAME_LEN );
    SafeStrCopy( userAccountIdent.getOnlineDescription(), moodMsg, MAX_ONLINE_DESC_LEN );
    userAccountIdent.setGender( (EGenderType)gender );
    userAccountIdent.setAgeType( age );
    userAccountIdent.setPrimaryLanguage( (ELanguageType)primaryLanguage );
    userAccountIdent.setPreferredContent( (EContentRating)contentType );

    userAccountIdent.setPluginPermissionsToDefaultValues();
    setupAccountResources( userAccountIdent );
}

//============================================================================
void AppCommon::setupAccountResources( VxNetIdent& userAccountIdent )
{
    std::string strUserName = userAccountIdent.getOnlineName();
    getEngine().fromGuiSetUserXferDir( getUserXferDirectoryFromAccountUserName( strUserName.c_str() ).c_str() );
    // kodi also needs the directory
    getEngine().fromGuiSetUserSpecificDir( getUserSpecificDataDirectoryFromAccountUserName( strUserName.c_str() ).c_str() );

    // get port to listen on 
    uint16_t tcpPort = getEngine().getEngineSettings().getTcpIpPort();
    userAccountIdent.m_DirectConnectId.setPort( tcpPort );

    // get current default ip
    InetAddress defaultIp = getEngine().fromGuiGetMyIpAddress();
    if( defaultIp.isValid() )
    {
        userAccountIdent.m_DirectConnectId.setIpAddress(defaultIp);
        std::string myIP = userAccountIdent.m_DirectConnectId.m_OnlineIp.toString();

        LogMsg( LOG_VERBOSE, "Account %s IP %s", strUserName.c_str(), myIP.c_str() );
    }

    copyAssetsToFoldersIfRequired();

    getEngine().getNetStatusAccum().setIpPort( tcpPort );
}

//============================================================================
std::string AppCommon::getUserXferDirectoryFromAccountUserName( const char* userName )
{
    std::string strUserDownloadDir = VxGetRootXferDirectory();
    strUserDownloadDir += userName;
    strUserDownloadDir += "/";
    VxFileUtil::makeDirectory( strUserDownloadDir );
    return strUserDownloadDir;
}

//============================================================================
std::string AppCommon::getUserSpecificDataDirectoryFromAccountUserName( const char* userName )
{
    std::string strUserSpecificDir = VxGetRootUserDataDirectory();
    strUserSpecificDir += "accounts/";
    VxFileUtil::makeDirectory( strUserSpecificDir );

    strUserSpecificDir += userName;
    strUserSpecificDir += "/";
    VxFileUtil::makeDirectory( strUserSpecificDir );
    return strUserSpecificDir;
}

//============================================================================
void AppCommon::loadAccountSpecificSettings( const char* userName )
{
    getEngine().fromGuiSetUserXferDir( getUserXferDirectoryFromAccountUserName( userName ).c_str() );
    std::string strUserSpecificDir = getUserSpecificDataDirectoryFromAccountUserName( userName );
    // media player also needs the directory
    getEngine().fromGuiSetUserSpecificDir( strUserSpecificDir.c_str() );

    copyAssetsToFoldersIfRequired();

    setIsAppInitialized( true );

    m_HomeWindow->getLaunchPage()->stopSpinner();
}

//============================================================================
void AppCommon::copyAssetsToFoldersIfRequired( void )
{
    // setup about me page if requrired
    std::string userAboutMePicture = VxGetAboutMePageServerDirectory();
    userAboutMePicture += "me.png";
    if( false == VxFileUtil::fileExists( userAboutMePicture.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/me.png", userAboutMePicture.c_str() );
    }

    std::string aboutMePageIndex = VxGetAboutMePageServerDirectory();
    aboutMePageIndex += "index.htm";
    if( false == VxFileUtil::fileExists( aboutMePageIndex.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/index.htm", aboutMePageIndex.c_str() );
    }

    std::string aboutMeFavIcon = VxGetAboutMePageServerDirectory();
    aboutMeFavIcon += "favicon.ico";
    if( false == VxFileUtil::fileExists( aboutMeFavIcon.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/favicon.ico", aboutMeFavIcon.c_str() );
    }

    std::string aboutMeThumb = VxGetAboutMePageServerDirectory();
    aboutMeThumb += "aboutme_thumb.png";
    if( false == VxFileUtil::fileExists( aboutMeThumb.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/nolimit_thumb.png", aboutMeThumb.c_str() );
    }

    // setup storyboard page if requrired
    std::string storyBoardIndexName = VxGetStoryBoardPageServerDirectory();
    storyBoardIndexName += "story_board.htm";
    if( false == VxFileUtil::fileExists( storyBoardIndexName.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/story_board.htm", storyBoardIndexName.c_str() );
    }

    std::string storyBoardBackground = VxGetStoryBoardPageServerDirectory();
    storyBoardBackground += "storyboard_background.png";
    if( false == VxFileUtil::fileExists( storyBoardBackground.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/storyboard_background.png", storyBoardBackground.c_str() );
    }

    std::string storyBoardMePicture = VxGetStoryBoardPageServerDirectory();
    storyBoardMePicture += "me.png";
    if( false == VxFileUtil::fileExists( storyBoardMePicture.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/me.png", storyBoardMePicture.c_str() );
    }

    std::string storyBoardFavIcon = VxGetStoryBoardPageServerDirectory();
    storyBoardFavIcon += "favicon.ico";
    if( false == VxFileUtil::fileExists( storyBoardFavIcon.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/favicon.ico", storyBoardFavIcon.c_str() );
    }

    std::string storyBoardThumb = VxGetStoryBoardPageServerDirectory();
    storyBoardThumb += "storyboard_thumb.png";
    if( false == VxFileUtil::fileExists( storyBoardThumb.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/nolimit_thumb.png", storyBoardThumb.c_str() );
    }
}

//============================================================================
void AppCommon::sendAppSettingsToEngine( void )
{
    // TODO figure out how engine network settings can be out of sync with gui network settings
    // for now assure engine has correct settings before log in
    bool validDbSettings = false;
    NetHostSetting selectedNetHostSetting;

    AccountMgr& dataHelper = getAccountMgr();
    std::vector<NetHostSetting> netSettingList;
    std::string lastSettingsName = dataHelper.getLastNetHostSettingName();
    if ((0 != lastSettingsName.length())
        && dataHelper.getAllNetHostSettings(netSettingList)
        && (0 != netSettingList.size()))
    {
        std::vector<NetHostSetting>::iterator iter;
        for (iter = netSettingList.begin(); iter != netSettingList.end(); ++iter)
        {
            NetHostSetting& netHostSetting = *iter;
            if (netHostSetting.getNetHostSettingName() == lastSettingsName)
            {
                // found last settings used
                validDbSettings = true;
                selectedNetHostSetting = netHostSetting;
            }
        }
    }

    if (validDbSettings)
    {
        getEngine().fromGuiApplyNetHostSettings( selectedNetHostSetting );
    } 
}

//============================================================================
void AppCommon::showUserNameInTitle()
{
    QString strTitle = VxGetApplicationTitle();
    strTitle += "-";
    strTitle += getAccountUserName().c_str();
    setWindowTitle( strTitle );
}

//============================================================================
void AppCommon::checkReadyToLaunchAfterLogonApplets( void )
{
    if( !m_LauchedAfterLogonApplets && isReadyToLaunchAfterLogonApplets() )
    {
        m_LauchedAfterLogonApplets = true;

#if defined(TARGET_OS_ANDROID)
        if( !getCamCaptureReady() )
        {
            // Start camera service once login/network is ready so camera devices are enumerated before Cam Settings is opened.
            ICamCapture::getICamCapture().startupCamCapture();
        }
#endif // defined(TARGET_OS_ANDROID)

        if( ICamCapture::getICamCapture().isCamCaptureRequested() && !ICamCapture::getICamCapture().isCamCaptureRunning() )
        {
            ICamCapture::getICamCapture().setCamCaptureEnable( true );
        }

        setCamCaptureReady( true );

        checkReadyToConnectToLastConnectedHost();
    }
}

//============================================================================
bool AppCommon::isReadyToLaunchAfterLogonApplets( void )
{
    return m_IsGuiSystemReady && m_PtopNetworkReady;
}

//============================================================================
void AppCommon::checkReadyToConnectToLastConnectedHost( void )
{
    if( m_LauchedAfterLogonApplets && !m_ConnectToLastConnectedHost )
    {
        m_ConnectToLastConnectedHost = true;
        std::string lastConnectedHost = getAppSettings().getLastHostJoined();
        if( !lastConnectedHost.empty() )
        {
            getUserJoinMgr().reconnectToLastConnectedHost( lastConnectedHost );
        }
    }
}

//============================================================================
void AppCommon::showCreateAccount( void )
{
    AppletCreateAccount* createAccount = new AppletCreateAccount( *this, m_HomeWindow );
    createAccount->setRootUserDataDirectory( VxGetRootUserDataDirectory() );
    connect( createAccount, SIGNAL(signalAccountCreated(bool)), this, SLOT(slotAccountCreated(bool)) );
    createAccount->show();
}

//============================================================================
void AppCommon::slotAccountCreated( bool wasCreated )
{
    if(!wasCreated)
    {
        VxSetAppIsShuttingDown( true );
        QCoreApplication::quit();
        return;
    }

    doAccountStartup();
}

//============================================================================
void AppCommon::slotAfterLoginComplete( void )
{
	m_RandConnectMgr.onLoginComplete();
}