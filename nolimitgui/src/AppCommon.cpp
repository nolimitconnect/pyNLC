//============================================================================
// Copyright (C) 2009 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppCommon.h"

#include "ToGuiActivityInterface.h"

#include "AdminAvailMgr.h"
#include "HomeWindow.h"
#include "GuiHelpers.h"
#include "GuiParams.h"
#include "LogMgr.h"
#include "MyIcons.h"
#include "SoundFxMgr.h"
#include "TodGameMgr.h"

#include "AppletCreateAccount.h"
#include "ActivityShowHelp.h"
#include "ActivityMsgBoxYesNo.h"

#include "AppletDownloads.h"
#include "AppletUploads.h"

#include "AccountMgr.h"
#include "AppModuleState.h"
#include "AppSettings.h"
#include "AppletMgr.h"
#include "AssetSendMgr.h"

#include "FileListReplySession.h"

#include "GuiThreadAppLoader.h"
#include "GuiFavoriteMgr.h"
#include "GuiMemberActiveMgr.h"
#include "GuiOfferSession.h"
#include "GuiPlayerMgr.h"
#include "GuiPluginMgr.h"
#include "GuiPushToTalkMgr.h"
#include "GuiSendQueueMgr.h"
#include "GuiRandConnectMgr.h"

#include "VxPushButton.h"

#include <BlobXferMgr/BlobInfo.h>
#include <P2PEngine/FileShareSettings.h>
#include <P2PEngine/EngineSettings.h>
#include <P2PEngine/P2PEngine.h>

#include <NetworkMonitor/NetworkMonitor.h>
#include <ThumbMgr/ThumbInfo.h>

#include <PktLib/VxCommon.h>

#include <CoreLib/AppVersion.h>
#include <CoreLib/ConnectId.h>
#include <CoreLib/IsBigEndianCpu.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxMemoryUsage.h>
#include <CoreLib/VxParse.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxGUID.h>
#include <CoreLib/VxSktUtil.h>
#include <CoreLib/VxTime.h>

#include <CoreLib/InetAddressParse.h>

#include <NetLib/VxPeerMgr.h>

#include <QApplication>
#include <QMainWindow>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFrame>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include "RenderGlWidget.h"

#include <signal.h>
#include <array>

#include "MediaPlayerNlc.h"
#include "RenderGlLogic.h"
#include "GuiAudioMgr.h"
#include "SoundFxMgr.h"

namespace
{
	AppCommon * g_AppCommon = 0;

	//============================================================================
	QString GetAppTitle( void )
	{
		return QObject::tr( "No Limit Connect" );
	}

	//============================================================================
	QString GetAppShortName( void )
	{
		//NOTE: do not translate or will cause new settings each time user changes languages
		return APP_NAME;
	}

    //============================================================================
    static void* AudioDevicesStartupThreadFunc( void* pvContext )
    {
        VxThread* poThread = (VxThread*)pvContext;
        poThread->setIsThreadRunning( true );
        AppCommon* appCommon = (AppCommon*)poThread->getThreadUserParam();
        if( appCommon )
        {
            appCommon->getAudioMgr().audioIoSystemStartup();
        }

        poThread->threadAboutToExit();
        return nullptr;
    }

	//============================================================================
	static void* EngineStartupThreadFunc( void* pvContext )
	{
		VxThread* poThread = (VxThread*)pvContext;
		poThread->setIsThreadRunning( true );
		AppCommon* appCommon = (AppCommon*)poThread->getThreadUserParam();
		if( appCommon )
		{
			std::string strAssetDir = VxGetRootUserDataDirectory() + "assets/";
			appCommon->getEngine().fromGuiAppStartup( strAssetDir.c_str(), VxGetRootUserDataDirectory().c_str() );
		}

		poThread->threadAboutToExit();
		return nullptr;
	}

	//============================================================================
	static void StartPlayerMgrOnGuiThread( AppCommon* appCommon )
	{
		if( appCommon )
		{
			appCommon->getPlayerMgr().playerMgrStartup();
		}
	}
}

//============================================================================
AppCommon& CreateAppInstance( QApplication* myApp, AppSettings& appSettings )
{
static AdminAvailMgr adminAvailMgr;
static AppModuleState appModuleState;
static AccountMgr accountMgr;
static GuiFavoriteMgr favoritMgr;
static GuiMemberActiveMgr memberActiveMgr;
static GuiPlayerMgr playerMgr;
static GuiPluginMgr pluginMgr;
static GuiPushToTalkMgr pushToTalkMgr;
static GuiRandConnectMgr randConnectMgr;
static GuiSendQueueMgr sendQueueMgr;
static AssetSendMgr assetSendMgr;

static MyIcons myIcons;
static TodGameMgr todGameMgr;
static SoundFxMgr soundFxMgr;

    if( !g_AppCommon )
    {
		// need sockets right away so can check for port in use when creating new account
		VxSocketsStartup();

        // constructor of AppCommon will set g_AppCommon
        new AppCommon( *myApp, adminAvailMgr, appModuleState, appSettings, accountMgr, favoritMgr,
					   memberActiveMgr, playerMgr, pluginMgr, pushToTalkMgr, randConnectMgr, 
					   sendQueueMgr, assetSendMgr, myIcons, todGameMgr, soundFxMgr );
    }

    return *g_AppCommon;
}

//============================================================================
AppCommon& GetAppInstance( void )
{
	return *g_AppCommon;
}

//============================================================================
void DestroyAppInstance()
{
    delete g_AppCommon;
    g_AppCommon = nullptr;
}

//============================================================================
AppCommon::AppCommon(	QApplication&	myQApp,
						AdminAvailMgr&  adminAvailMgr,
						AppModuleState& appModuleState,
						AppSettings&	appSettings, 
                        AccountMgr&	    accountMgr,
						GuiFavoriteMgr& favoritMgr,
						GuiMemberActiveMgr& memberActiveMgr,
					    GuiPlayerMgr&	playerMgr,
						GuiPluginMgr&	pluginMgr,
						GuiPushToTalkMgr& pushToTalkMgr,
						GuiRandConnectMgr& randConnectMgr,
						GuiSendQueueMgr& sendQueueMgr,
                        AssetSendMgr&   assetSendMgr,
						MyIcons&		myIcons,
						TodGameMgr&		todGameMgr,
        				SoundFxMgr& soundFxMgr )
: QWidget()
, m_QApp( myQApp )
, m_AppModuleState(appModuleState)
, m_AppGlobals( *this )
, m_AppSettings( appSettings )
, m_AppShortName( GetAppShortName() )
, m_AppTitle( GetAppTitle() )
, m_AccountMgr( accountMgr )
, m_AdminAvailMgr( adminAvailMgr )

, m_AudioMgr( *this )

, m_TodGameMgr( todGameMgr )

, m_ConnectIdListMgr( *this )
, m_FavoriteMgr( favoritMgr )
, m_FileXferMgr( *this )
, m_FriendRequestMgr( *this )
, m_ThumbMgr( *this )
, m_MemberActiveMgr( memberActiveMgr )
, m_OfferMgr( *this )
, m_PlayerMgr( playerMgr )
, m_PluginMgr( pluginMgr )
, m_PushToTalkMgr( pushToTalkMgr )
, m_RandConnectMgr( randConnectMgr )
, m_SendQueueMgr( sendQueueMgr )
, m_AssetSendMgr( assetSendMgr )
, m_UserMgr( *this )
, m_GroupieListMgr( *this )
, m_HostedListMgr( *this )
, m_HostJoinMgr( *this )
, m_UserJoinMgr( *this )
, m_WebPageMgr( *this )

, m_MyIcons( myIcons )
, m_AppTheme( *this )
, m_AppStyle( *this, m_AppTheme )
, m_AppDisplay( *this )

, m_CamLogic( *this )

, m_SoundFxMgr( soundFxMgr )

, m_OncePerSecondTimer( new QTimer( this ) )
, m_eLastSelectedWhichContactsToView( eFriendViewEverybody )
, m_bUserCanceledCreateProfile( false )
, m_LastNetworkState( eNetworkStateTypeUnknown )
, m_CamCaptureRotation( 0 )
, m_AppletMgr( *( new AppletMgr( *this, this) ) )
, m_GuiStartupTimer( new QTimer( this ) )
{
    g_AppCommon = this; // need a global instance that can accessed immediately with GetAppInstance() for objects created in ui files
	setGuiThreadId( VxGetCurrentThreadId() );

#if !defined(TARGET_OS_WINDOWS)
	// make your application ignore SIGPIPE. that sometimes happens when socket connection is broken
    // If you ignore the SIGPIPE signal, then the functions will return EPIPE error
	signal( SIGPIPE, SIG_IGN );
#endif // !defined(TARGET_OS_WINDOWS)

	connect( m_GuiStartupTimer, SIGNAL(timeout()), this, SLOT(slotGuiStartupTimer()) );
	connectSignals();
}

//============================================================================
int AppCommon::elapsedMilliseconds( void )
{
	return GetApplicationAliveMs();
}

//============================================================================
IFromGui& AppCommon::getFromGuiInterface( void )
{
    return getEngine().getFromGuiInterface();
}

//============================================================================
bool AppCommon::loadWithThread( void )
{
	const int loadStartMs = GetApplicationAliveMs();
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread begin at %d ms", loadStartMs );
	const std::string& rootUserDataDir = VxGetRootUserDataDirectory();
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread root user data dir: %s", rootUserDataDir.c_str() );
	GuiThreadAppLoader appLoaderThread( *this );
	appLoaderThread.start();
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread app loader thread started at %d ms", GetApplicationAliveMs() );

    // asset database and user specific setting database will be created in sub directory of account login
    // after user has logged into account

	GuiParams::requestPermission("android.permission.RECORD_AUDIO");
	// once settings has been loaded the audo can be started
    m_AudioDevicesThread.startThread( (VX_THREAD_FUNCTION_T)AudioDevicesStartupThreadFunc, this, "AudioDevicesStartupThreadFunc" );
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread audio startup thread started at %d ms", GetApplicationAliveMs() );

	getQApplication().setStyle( &m_AppStyle );
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread style applied at %d ms", GetApplicationAliveMs() );

	int waitAccountMgrStartMs = GetApplicationAliveMs();
	while( !appLoaderThread.getIsAccountMgrLoaded() )
	{
		GuiHelpers::processQtEvents();
	}
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread waited %d ms for account manager", GetApplicationAliveMs() - waitAccountMgrStartMs );

	m_ThumbMgr.onAppCommonCreated();
	m_UserMgr.onAppCommonCreated();
	m_OfferMgr.onAppCommonCreated();
	GuiHelpers::processQtEvents( 1 );
	m_HostedListMgr.onAppCommonCreated();
	m_HostJoinMgr.onAppCommonCreated();
	m_UserJoinMgr.onAppCommonCreated();
	GuiHelpers::processQtEvents( 1 );
	m_WebPageMgr.onAppCommonCreated();
	m_ConnectIdListMgr.onAppCommonCreated();
	m_MemberActiveMgr.onAppCommonCreated();
	GuiHelpers::processQtEvents( 1 );
	m_SendQueueMgr.onAppCommonCreated();
	m_AssetSendMgr.onAppCommonCreated();
	m_GroupieListMgr.onAppCommonCreated();
	GuiHelpers::processQtEvents( 1 );
	m_FriendRequestMgr.onAppCommonCreated();
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread manager onAppCommonCreated callbacks done at %d ms", GetApplicationAliveMs() );

	int waitIconsStartMs = GetApplicationAliveMs();
	while( !appLoaderThread.getIsIconsLoaded() )
	{
		GuiHelpers::processQtEvents();
	}
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread waited %d ms for icon load", GetApplicationAliveMs() - waitIconsStartMs );

	m_HomeWindow = new HomeWindow(*this, m_AppTitle);
	getAppTheme().selectTheme(getAppSettings().getLastSelectedTheme(), m_HomeWindow);

    connect( m_HomeWindow, SIGNAL(signalMainWindowResized()), this, SLOT(slotMainWindowResized()) );
	m_HomeWindow->initializeHomePage();
	startupAppCommon( m_HomeWindow->getAppletFrame( eAppletHomePage ), m_HomeWindow->getAppletFrame( eAppletMessengerFrame ) );
	if( !m_LoginBegin )
	{
		m_LoginBegin = true;
		LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread begin login at %d ms", GetApplicationAliveMs() );
		doLogin();
	}
    LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread home window initialized at %d ms", GetApplicationAliveMs() );

	connect( this, SIGNAL(signalInternalNetAvailStatus(ENetAvailStatus)), this, SLOT(slotInternalNetAvailStatus(ENetAvailStatus)), Qt::QueuedConnection );

	int waitLoaderCompleteStartMs = GetApplicationAliveMs();
	while( !appLoaderThread.getIsLoadComplete() )
	{
		GuiHelpers::processQtEvents();
	}
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread waited %d ms for app loader completion", GetApplicationAliveMs() - waitLoaderCompleteStartMs );

	appLoaderThread.quit();
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::loadWithThread done in %d ms", GetApplicationAliveMs() - loadStartMs );

	m_HomeWindow->show();
	return true;
}

//============================================================================
P2PEngine& AppCommon::getEngine( void ) 
{ 
	return GetPtoPEngine();
}

//============================================================================
void AppCommon::startupAppCommon( QFrame* appletFrame, QFrame* messangerFrame )
{
    if( m_AppCommonInitialized )
    {
        return;
    }

	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::startupAppCommon begin at %d ms", GetApplicationAliveMs() );

    m_AppCommonInitialized = true;

    m_AppletDownloads = new AppletDownloads( *this, appletFrame );
    m_AppletDownloads->hide();

    m_AppletUploads = new AppletUploads( *this, appletFrame );
	m_AppletUploads->hide();

	std::string strAssetDir = VxGetRootUserDataDirectory() + "assets/";
	VxFileUtil::makeDirectory( strAssetDir );

	m_GuiStartupTimer->setSingleShot( true );
	m_GuiStartupTimer->setInterval( 0 );
	m_GuiStartupTimer->start();
	m_EngineStartupStarted = false;
	m_GuiStartupAudioWaitStartMs = 0;
	m_GuiStartupAudioWaitBypassed = false;
	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::startupAppCommon queued startup timer at %d ms", GetApplicationAliveMs() );
}

//============================================================================
void AppCommon::slotGuiStartupTimer( void )
{
	static int guiStartupStep = 0;
	const int stepStartMs = GetApplicationAliveMs();
	static constexpr int kMaxAudioInitWaitMs = 35000;

    if( !m_AudioMgr.isAudioInitialized() )
    {
		if( 0 == m_GuiStartupAudioWaitStartMs )
		{
			m_GuiStartupAudioWaitStartMs = stepStartMs;
		}

        const int waitedMs = stepStartMs - m_GuiStartupAudioWaitStartMs;
        if( waitedMs < kMaxAudioInitWaitMs )
        {
        // on android audio devices can take up to 30 seconds
			LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer waiting for audio init at %d ms", stepStartMs );
			m_GuiStartupTimer->setInterval( 100 );
        m_GuiStartupTimer->start();
        return;
        }

        if( !m_GuiStartupAudioWaitBypassed )
        {
			m_GuiStartupAudioWaitBypassed = true;
			LogMsg( LOG_WARN, "AppCommon::slotGuiStartupTimer audio init timeout after %d ms; continuing startup", waitedMs );
        }
    }

	if( 0 == guiStartupStep )
	{	
		guiStartupStep = 1;
		m_GuiStartupTimer->setInterval( 0 );
		m_GuiStartupTimer->start();
	}
	else if( 1 == guiStartupStep )
	{
		if( !m_EngineStartupStarted )
		{
			m_EngineStartupThread.startThread( (VX_THREAD_FUNCTION_T)EngineStartupThreadFunc, this, "EngineStartupThreadFunc" );
			m_EngineStartupStarted = true;
			LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer step 2 (engine) started worker at %d ms", GetApplicationAliveMs() );
		}

		if( m_EngineStartupThread.isThreadRunning() )
		{
			GuiHelpers::processQtEvents( 1 );
			m_GuiStartupTimer->setInterval( 25 );
			m_GuiStartupTimer->start();
			return;
		}

		LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer step 2 (engine) complete at %d ms", GetApplicationAliveMs() );
		guiStartupStep = 2;
		m_GuiStartupTimer->setInterval( 25 );
		m_GuiStartupTimer->start();
		return;
	}
	else if( 2 == guiStartupStep )
	{
		LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer step 3 (playerMgr) begin at %d ms", stepStartMs );
		StartPlayerMgrOnGuiThread( this );
		GuiHelpers::processQtEvents( 1 );
		LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer step 3 (playerMgr) complete at %d ms", GetApplicationAliveMs() );
		m_OncePerSecondTimer->setInterval( 1000 ); 
		connect( m_OncePerSecondTimer, SIGNAL(timeout()), this, SLOT(onOncePerSecond()), Qt::UniqueConnection );
		m_OncePerSecondTimer->start();

		guiStartupStep = 3;
        m_GuiStartupTimer->setInterval( 4000 );
		m_GuiStartupTimer->start();
	}
	else if( 3 == guiStartupStep )
	{
        m_GuiStartupTimer->stop();
        // load sounds to play and sound hardware
		LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer step 1 (sndFxMgr) begin at %d ms", stepStartMs );
		m_SoundFxMgr.sndFxMgrStartup();
		LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer step 1 (sndFxMgr) complete at %d ms", GetApplicationAliveMs() );
        guiStartupStep = 4;
    }

	LogModule( eLogStartup, LOG_VERBOSE, "AppCommon::slotGuiStartupTimer startup complete at %d ms", GetApplicationAliveMs() );
}

//============================================================================
void AppCommon::shutdownAppCommon( void )
{
    static bool hasBeenShutdown = false;
    if( false == hasBeenShutdown )
    {
		if( confirmAppShutdown( m_HomeWindow ) )
		{
			hasBeenShutdown = true;
			VxSetAppIsShuttingDown( true );

			// queued so does not shutdown while dialog is still open
			emit signalShutdownApp();
		}
    }
}

//============================================================================
void AppCommon::slotShutdownApp( void )
{
	VxSetAppIsShuttingDown( true );
	m_CamLogic.shutdownCamLogic();
	m_SoundFxMgr.sndFxMgrShutdown();
	m_AudioMgr.audioIoSystemShutdown();

	fromGuiCloseEvent( eMediaModuleAll );
	ActivityBase* appPlayer = m_AppletMgr.findAppletDialog( eAppletPlayerNlc );
	if( appPlayer )
	{
		// to force media player stop before exit application
		appPlayer->onActivityFinish();
	}

	QApplication::closeAllWindows();
	getEngine().fromGuiAppShutdown();
	IMediaPlayerRequests::getNlcPlayer().fromGuiAppShutdown();
}

//============================================================================
void AppCommon::setIsMaxScreenSize( bool isMessagerFrame, bool isFullSizeWindow )
{
    m_HomeWindow->setIsMaxScreenSize( isMessagerFrame, isFullSizeWindow );
	emit signalExpandWindowChanged( isMessagerFrame, isFullSizeWindow );
    emit signalMainWindowResized();
}

//============================================================================
bool AppCommon::getIsMaxScreenSize( bool isMessagerFrame )
{
    return m_HomeWindow->getIsMaxScreenSize( isMessagerFrame );
}

//============================================================================
void AppCommon::switchWindowFocus( QWidget* appIconButton )
{
	m_HomeWindow->switchWindowFocus( appIconButton );
}

//============================================================================
QFrame* AppCommon::getAppletFrame( EApplet applet )
{
	return getHomeWindow().getAppletFrame( applet );
}

//============================================================================
ActivityBase* AppCommon::launchApplet( EApplet applet, QWidget* parent )
{
	return m_AppletMgr.launchApplet( applet, parent );
}

//============================================================================
ActivityBase* AppCommon::launchApplet( EApplet applet, QWidget* parent, QString launchParam, VxGUID& assetId )
{
	return m_AppletMgr.launchApplet( applet, parent, launchParam, assetId );
}

//============================================================================
void AppCommon::activityStateChange( ActivityBase* activity, bool isCreated )
{
	m_AppletMgr.activityStateChange( activity, isCreated );
}

//============================================================================
void AppCommon::slotMainWindowResized( void )
{
	emit signalMainWindowResized();
}

//============================================================================
void AppCommon::slotMainWindowMoved( void )
{
    emit signalMainWindowMoved();
}

//============================================================================
VxNetIdent* AppCommon::getMyNetIdent( void )
{
	return getAppGlobals().getMyNetIdent();
}

//============================================================================
VxGUID& AppCommon::getMyOnlineId( void )
{
    return getMyNetIdent()->getMyOnlineId();
}

//============================================================================
void AppCommon::applySoundSettings( bool useDefaultsInsteadOfSettings )
{
	if( useDefaultsInsteadOfSettings )
	{
		m_SoundFxMgr.muteNotifySound( false );
		m_SoundFxMgr.mutePhoneRing( false );
	}
	else
	{
		m_SoundFxMgr.muteNotifySound( m_AppSettings.getMuteNotifySound() );
		m_SoundFxMgr.mutePhoneRing( m_AppSettings.getMutePhoneRing() );
	}
}

//============================================================================
void AppCommon::playSound( ESndDef sndDef )
{
	if( getGuiThreadId() != VxGetCurrentThreadId() )
	{
		LogMsg( LOG_ERROR, "AppCommon::playSound cannot play from another thread" );
		vx_assert( false );
		return;
	}

	if( m_AppSettings.getDisableAllSoundEffects() )
	{
		return;
	}

	if( ( sndDef == eSndDefPaperShredder || sndDef == eSndDefNeckSnap ) && m_AppSettings.getDisableSndTrash() )
	{
		return;
	}
	else if( sndDef == eSndDefButtonClick && m_AppSettings.getDisableSndKeyClick() )
	{
		return;
	}
	else if( ( sndDef == eSndDefNotify1 || sndDef == eSndDefNotify2 ) && m_AppSettings.getDisableSndNotify() )
	{
		return;
	}
	else if( sndDef == eSndDefUserBellMessage && m_AppSettings.getDisableSndMsgRx() )
	{
		return;
	}

	getSoundFxMgr().playSnd( sndDef );
}

//============================================================================
void AppCommon::insertKeystroke( int keyCode )
{
	if( 0 == keyCode )
	{
		LogMsg( LOG_ERROR, "AppCommon::insertKeystroke 0 keyCode" );
		return;
	}

	QWidget*receiver = QApplication::focusWidget();
	if( 0 == receiver )
	{
		receiver = getQApplication().activeWindow();
	}

	if( 0 == receiver )
	{
		LogMsg( LOG_DEBUG,  "AppCommon::insertKeystroke no reciever" );
		return;
	}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    Qt::KeyboardModifiers keyFlags = 0;
#else
    Qt::KeyboardModifiers keyFlags;
#endif // QT_VERSION < QT_VERSION_CHECK(6,0,0)

	QString keyText = "";

	QKeyEvent * qtKeyEvent = new QKeyEvent( QEvent::KeyPress, keyCode, keyFlags, keyText );
	QCoreApplication::postEvent( receiver, qtKeyEvent );
	qtKeyEvent = new QKeyEvent( QEvent::KeyRelease, keyCode, keyFlags, keyText );
	QCoreApplication::postEvent( receiver, qtKeyEvent );
}

//============================================================================
void AppCommon::toGuiLog( int logFlags, const char* pMsg )
{
	if( VxIsAppShuttingDown() 
		|| ( 0 == pMsg ) )
	{
		return;
	}

    QString strMsg = pMsg;
#if QT_VERSION > QT_VERSION_CHECK(6,0,0)
	strMsg.remove(QRegularExpression("[\\n\\r]"));
#else
	strMsg.remove(QRegExp("[\\n\\r]"));
#endif // QT_VERSION > QT_VERSION_CHECK(6,0,0)

#ifdef TARGET_OS_WINDOWS
    qDebug() << strMsg;
#endif // TARGET_OS_WINDOWS
    emit signalLog( logFlags, strMsg );
}

//============================================================================
void AppCommon::toGuiAppErr( EAppErr eAppErr, const char* errMsg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	std::string formatedErr;
	if( errMsg && 0 != errMsg[0] )
	{
		StdStringFormat( formatedErr, "#App Error %d %s", eAppErr, errMsg );
	}
	else
	{
		StdStringFormat( formatedErr, "#App Error %d", eAppErr );
	}

	emit signalLog( LOG_ERROR, formatedErr.c_str() );
	emit signalStatusMsg( formatedErr.c_str() );
}

//============================================================================
void AppCommon::toGuiAppPopupErr( EAppErr eAppErr, const char* errMsg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalAppPopupErr( eAppErr, errMsg );
}

//============================================================================
void AppCommon::slotInternalAppPopupErr( EAppErr appErr, QString errMsg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	QString title{ QObject::tr("Unknown Error") };
	QString msg{ QObject::tr("ERROR:\n") };
	switch( appErr )
	{
	case eAppPopupErrNetworkHostConnectFail:
		title = QObject::tr("Failed to connect to Network host");
		break;
	case eAppPopupErrConnectTestHostConnectFail:
		title = QObject::tr("Failed to connect to Connection Test host");
		break;
	case eAppPopupErrVideoPlayFail:
		title = QObject::tr("Video Play Failed");
		break;
	case eAppPopupErrAudioPlayFail:
		title = QObject::tr("Audio Play Failed");
		break;
    case eAppPopupErrNoMicDevices:
        title = QObject::tr( "Sound In Device Error" );
		msg = QObject::tr("No Microphone Devices Available");
		break;
    case eAppPopupErrMicDeviceOutOfRange:
        title = QObject::tr( "Sound In Device Error" );
		msg = QObject::tr("Microphone Device Out Of Range");
		break;
    case eAppPopupErrMicDeviceFailedToInitialize:
        title = QObject::tr( "Sound In Device Error" );
		msg = QObject::tr("Failed to Initialize Microphone Device");
		break;
    case eAppPopupErrNoSpeakerDevices:
        title = QObject::tr( "Sound Out Device Error" );
		msg = QObject::tr("No Speaker Devices Available");
		break;
    case eAppPopupErrSpeakerDeviceOutOfRange:
        title = QObject::tr( "Sound Out Device Error" );
		msg = QObject::tr("Speaker Device Out Of Range");
		break;
    case eAppPopupErrSpeakerDeviceFailedToInitialize:
        title = QObject::tr( "Sound Out Device Error" );
		msg = QObject::tr("Failed to Initialize Speaker Device");
		break;
    case eAppPopupErrSpeakerDeviceInvaidFormat:
        title = QObject::tr( "Sound Out Device Error" );
		msg = QObject::tr("Speaker Device Invalid Format");
		break;
        
    case eAppPopupErrEchoDelayTestFail:
        title = QObject::tr( "Echo Delay Test Error" );
		msg = QObject::tr("Echo Delay Results (ms) ");
        msg += errMsg;
		break;

    case eAppPopupErrEchoDelayTestPass:
        title = QObject::tr( "Echo Delay Test Success" );
		msg = QObject::tr("Echo Delay Results (ms) ");
        msg += errMsg;
		break;
        
	default:
		break;
	}

	msg += errMsg;
	QMessageBox::warning( &getHomeWindow(), title, msg );
}

//============================================================================
void AppCommon::toGuiStatusMessage( const char* statusMsg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	if( statusMsg )
	{
		emit signalLog( LOG_INFO, statusMsg );
		emit signalStatusMsg( statusMsg );
	}
}

//============================================================================
// NOTE: toGuiUserMessage should be called from in gui on gui thread only
void AppCommon::toGuiUserMessage( const char* userMsg, ... )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}
	
	const int MAX_USER_MSG_SIZE = 2048;
	std::array<char, MAX_USER_MSG_SIZE> szBuffer;
	szBuffer[0] = 0;
	va_list arg_ptr;
	va_start(arg_ptr, userMsg);
#ifdef TARGET_OS_WINDOWS
	vsnprintf(szBuffer.data(), MAX_USER_MSG_SIZE, userMsg, (char*)arg_ptr);
#else
    vsnprintf(szBuffer.data(), MAX_USER_MSG_SIZE, userMsg, arg_ptr);
#endif //  TARGET_OS_WINDOWS
	szBuffer.data()[MAX_USER_MSG_SIZE - 1] = 0;
	va_end(arg_ptr);

	if( 0 != szBuffer[0] )
	{
		emit signalStatusMsg( szBuffer.data() );
	}
}

//============================================================================
void AppCommon::toGuiPluginMsg( EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, const char* paramMsg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	QString paramValue;
	if( paramMsg )
	{
		paramValue = paramMsg;
	}

	emit signalInternalPluginMessage( pluginType, onlineId, msgType, paramValue );
}

//============================================================================
void AppCommon::toGuiPluginCommError( EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, ECommErr commErr )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalPluginErrorMsg( pluginType, onlineId, msgType, commErr );
}

//============================================================================
void AppCommon::slotInternalPluginMessage( EPluginType pluginType, VxGUID onlineId, EPluginMsgType msgType, QString paramValue )
{
    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiPluginMsg( pluginType, onlineId, msgType, paramValue );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::slotInternalPluginErrorMsg( EPluginType pluginType, VxGUID onlineId, EPluginMsgType msgType, ECommErr commError )
{
	QString commErrDescription = GuiParams::describeCommError( commError );
    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiPluginMsg( pluginType, onlineId, msgType, commErrDescription );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
/// Send Network available status to GUI for display
void AppCommon::toGuiNetAvailableStatus( ENetAvailStatus netAvailStatus )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

    emit signalInternalNetAvailStatus( netAvailStatus );
}

//============================================================================
void AppCommon::slotInternalNetAvailStatus( ENetAvailStatus netAvailStatus )
{
	emit signalNetAvailStatus( netAvailStatus );
	if( GuiParams::isNetStatusPtoPReady( netAvailStatus ) )
	{
		if( !m_PtopNetworkReady )
		{
			m_PtopNetworkReady = true;
			checkReadyToLaunchAfterLogonApplets();
		}
	}
}

//============================================================================
void AppCommon::toGuiNetworkState( ENetworkStateType eNetworkState, const char* stateMsg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	m_LastNetworkState = eNetworkState;
	if( eNetworkStateTypeOnlineDirect == eNetworkState )
	{
		emit signalSetRelayHelpButtonVisibility( false );
	}
	else if( eNetworkStateTypeOnlineThroughRelay == eNetworkState )
	{
		emit signalSetRelayHelpButtonVisibility( true );
	}

	emit signalNetworkStateChanged( eNetworkState );
}

//============================================================================
void AppCommon::slotNetworkStateChanged( ENetworkStateType eNetworkState )
{
	switch( eNetworkState )
	{
	case eNetworkStateTypeUnknown:
	case eNetworkStateTypeLost:
	case eNetworkStateTypeNoInternetConnection:
		//ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateDisconnected ) );
		//ui.searchButton->setEnabled( false );
		//ui.searchButton->setIcon( getMyIcons().getIcon( eMyIconSearchDisabled ) );
		break;

	case eNetworkStateTypeAvail:
	case eNetworkStateTypeTestConnection:
	case eNetworkStateTypeWaitForRelay:
		//ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateDiscover ) );
		break;

	case eNetworkStateTypeOnlineDirect:
		//ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateDirectConnected ) );
		//ui.searchButton->setEnabled( true );
		//ui.searchButton->setIcon( getMyIcons().getIcon( eMyIconSearch ) );
		break;

	case eNetworkStateTypeOnlineThroughRelay:
		//ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateRelayConnected ) );
		//ui.searchButton->setEnabled( true );
		//ui.searchButton->setIcon( getMyIcons().getIcon( eMyIconSearch ) );
		break;

	case eNetworkStateTypeFailedResolveHostNetwork:
		//ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateHostFail ) );
		break;
    case eNetworkStateTypeFailedResolveHostGroup:
        //ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateHostFail ) );
        break;
    case eNetworkStateTypeFailedResolveHostGroupList:
        //ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateHostFail ) );
        break;
    case eMaxNetworkStateType:
        //ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateHostFail ) );
        break;
    case eNetworkStateTypeIpChange:
        //ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNetworkStateHostFail ) );
        break;
	}
}

//============================================================================
void AppCommon::toGuiHostAnnounceStatus( EHostType hostType, VxGUID& sessionId, EHostAnnounceStatus annStatus, const char* msg )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

	if( eHostAnnounceHandshaking == annStatus )
	{
		// ignore
		return;
	}

	if( !LogEnabled( eLogHostSearch ) )
	{
		return;
	}

    const char* hostStatus = DescribeHostAnnounceStatus( annStatus );
	const char* hostTypeStr = DescribeHostType( hostType );
    std::string formatedMsg;
    if( msg )
    {
        StdStringFormat( formatedMsg, "#%s %s %s", hostTypeStr, hostStatus, msg );
    }
    else
    {
        StdStringFormat( formatedMsg, "#%s %s", hostTypeStr, hostStatus );
    }

    emit signalLog( 0, formatedMsg.c_str() );
    emit signalStatusMsg( formatedMsg.c_str() );

    emit signalHostAnnounceStatus( hostType, sessionId, annStatus, formatedMsg.c_str() );
}

//============================================================================
void AppCommon::toGuiHostJoinStatus( EHostType hostType, VxGUID& sessionId, EHostJoinStatus joinStatus, const char* msg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	if( !LogEnabled( eLogHostJoin ) )
	{
		return;
	}

	const char* hostStatus = DescribeHostJoinStatus( joinStatus );
	std::string formatedMsg;
	if( msg )
	{
		StdStringFormat( formatedMsg, "#%s %s", hostStatus, msg );
	}
	else
	{
		StdStringFormat( formatedMsg, "#%s\n", hostStatus );
	}

	emit signalLog( 0, formatedMsg.c_str() );
	emit signalStatusMsg( formatedMsg.c_str() );

	emit signalHostJoinStatus( hostType, sessionId, joinStatus, formatedMsg.c_str() );
}

//============================================================================
void AppCommon::toGuiHostSearchStatus( EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr, const char* msg )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

	if( !LogEnabled( eLogHostJoin ) )
	{
		return;
	}

    QString hostStatus = GuiParams::describeHostSearchStatus( searchStatus );
    if( eCommErrNone != commErr )
    {
        hostStatus += GuiParams::describeCommError( commErr );
    }

    if( msg )
    {
        hostStatus += msg;
    }

    emit signalLog( 0, hostStatus );
    emit signalStatusMsg( hostStatus );

    emit signalHostSearchStatus( hostType, sessionId, searchStatus, hostStatus );
}

//============================================================================
void AppCommon::toGuiHostSearchResult( EHostType hostType, VxGUID& sessionId, HostedInfo& hostedInfo )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

	getHostedListMgr().toGuiHostSearchResult( hostType, sessionId, hostedInfo );
}

//============================================================================
void AppCommon::toGuiHostSearchComplete( EHostType hostType, VxGUID& sessionId )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	getHostedListMgr().toGuiHostSearchComplete( hostType, sessionId );
}

//============================================================================
void AppCommon::toGuiGroupieSearchStatus( EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr, const char* msg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	QString hostStatus = GuiParams::describeHostSearchStatus( searchStatus );
	if( eCommErrNone != commErr )
	{
		hostStatus += GuiParams::describeCommError( commErr );
	}

	if( msg )
	{
		hostStatus += msg;
	}

	emit signalLog( 0, hostStatus );
	emit signalStatusMsg( hostStatus );

	emit signalGroupieSearchStatus( hostType, sessionId, searchStatus, hostStatus );
}

//============================================================================
void AppCommon::toGuiGroupieSearchResult( EHostType hostType, VxGUID& sessionId, GroupieInfo& groupieInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	getGroupieListMgr().toGuiGroupieSearchResult( hostType, sessionId, groupieInfo );
}

//============================================================================
void AppCommon::toGuiGroupieSearchComplete( EHostType hostType, VxGUID& sessionId )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	getGroupieListMgr().toGuiGroupieSearchComplete( hostType, sessionId );
}

//============================================================================
void AppCommon::toGuiIsPortOpenStatus( EIsPortOpenStatus eIsPortOpenStatus, const char* msg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	const char* portOpenStatus = DescribePortOpenStatus( eIsPortOpenStatus );
	std::string formatedMsg;
	if( msg )
	{
		StdStringFormat( formatedMsg, "#%s %s", portOpenStatus, msg );
	}
	else
	{
		StdStringFormat( formatedMsg, "#%s", portOpenStatus );
	}

	emit signalLog( 0, formatedMsg.c_str() );
	emit signalStatusMsg( formatedMsg.c_str() );

	emit signalIsPortOpenStatus( eIsPortOpenStatus, formatedMsg.c_str() );
}

//============================================================================
void AppCommon::toGuiRunTestStatus( const char*testName, ERunTestStatus eRunTestStatus, const char* msg )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

    const char* runTestStatus = DescribeRunTestStatus( eRunTestStatus );
    std::string formatedMsg;
    if( msg )
    {
        StdStringFormat( formatedMsg, "#%s %s", runTestStatus, msg );
    }
    else
    {
        StdStringFormat( formatedMsg, "#%s", runTestStatus );
    }

    emit signalLog( 0, formatedMsg.c_str() );
    emit signalStatusMsg( formatedMsg.c_str() );

    emit signalRunTestStatus( testName,  eRunTestStatus, formatedMsg.c_str() );
}

//============================================================================
void AppCommon::toGuiRandomConnectStatus( ERandomConnectStatus eRandomConnectStatus, const char* msg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	const char* phoneShakeStatus = DescribeRandomConnectStatus( eRandomConnectStatus );
	std::string formatedMsg;
	if( msg )
	{
		StdStringFormat( formatedMsg, "#%s", msg );
	}
	else
	{
		StdStringFormat( formatedMsg, "#%s", phoneShakeStatus );
	}

	emit signalLog( 0, formatedMsg.c_str() );
	emit signalStatusMsg( formatedMsg.c_str() );

	emit signalRandomConnectStatus( eRandomConnectStatus, formatedMsg.c_str() );
}

//============================================================================
	// prompt user to confirm wants to shutdown app.. caller must call appCommonShutdown if answer is yes
bool AppCommon::confirmAppShutdown( QWidget* parentWindow )
{
	if( m_bUserCanceledCreateProfile )
	{
		// we are shutting down because user canceled creating profile
		return true;
	}
		    
    // Setting parent will keep message box in the center of QMainWindow

    static bool confirmedExit = false;
    if( confirmedExit )
    {
        return true;
    }

	// to get here we must be on the home page
	QString title = QObject::tr( "Close Application" );
	QString body = QObject::tr( "Are you sure you want to exit?" );

	ActivityMsgBoxYesNo confirmExit( *this, parentWindow, title, body );
	if( QDialog::Accepted == confirmExit.exec() )
	{
        confirmedExit = true;
        return true;
	}

	return false;
}

//============================================================================
bool AppCommon::userCanceled( void )
{
	return m_bUserCanceledCreateProfile;
}

//============================================================================
void AppCommon::onOncePerSecond( void )
{
    if( !getAppSettings().getIsAppSettingInitialized() )
    {
        LogMsg( LOG_ERROR, "%s called before settings initialized", __func__ );
        return;
    }

	static int64_t startTime = 0;
	if( getGuiCpuTimeEnable() )
	{
		startTime = GetHighResolutionTimeMs();
	}

	if( !m_LoginBegin )
    {
		m_LoginBegin = true;
		LogModule( eLogStartup, LOG_VERBOSE, "onOncePerSecond begin login at %d ms", GetApplicationAliveMs() );
		doLogin();
	}

	if( getLoginCompleted() )
    {
		getEngine().fromGuiOncePerSecond();
    }

	static int64_t endTime = 0;
	if( getGuiCpuTimeEnable() )
	{
		endTime = GetHighResolutionTimeMs();
		int elapsedTime = (int)(endTime - startTime);
		if( elapsedTime > 2 )
		{
			LogMsg( LOG_VERBOSE, "AppCommon::onOncePerSecond %d ms in function", elapsedTime );
		}
    }

// commented out but do not remove. usefull in debugging memory leaks
// #if defined(DEBUG)
// 	static int memShowCnt = 0;
// 	memShowCnt++;
// 	if( memShowCnt >= 10 )
// 	{
// 		memShowCnt = 0;
// 		uint64_t memoryInUse = 0;
// 		uint64_t totalMemory = 0;
// 		if( VxGetMemoryUsage( memoryInUse, totalMemory ) )
// 		{
// 			LogMsg( LOG_VERBOSE, "Memory usage %3.1f MB of %3.1f MB", (double)memoryInUse / 1000000.0,  (double)totalMemory / 1000000.0 );
// 		}
//     }
// #endif // defined(DEBUG)
}

//============================================================================
//! time to flash the notify icon
void AppCommon::slotOnNotifyIconFlashTimeout( bool bWhite )
{
	if( bWhite )
	{
		////ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNotifyStarWhite ) );	
	}
	else
	{
		////ui.notifyButton->setIcon( getMyIcons().getIcon( eMyIconNotifyStarGreen ) );	
	}
}

//============================================================================
void AppCommon::slotStatusMsg( QString strMsg )
{
	LogMsg( LOG_INFO, strMsg.toStdString().c_str() );
	//ui.StatusMsgLabel->setText( strMsg );
	if(strMsg.isEmpty())
	{
		//ui.StatusMsgLabel->setVisible( false );
	}
	else
	{
		//ui.StatusMsgLabel->setVisible( true );
	}
}

//============================================================================
void AppCommon::slotAppErr( EAppErr eAppErr, QString errMsg )
{
	//errMsg.remove(QRegExp("[\\n\\r]"));
	QMessageBox::warning(this, QObject::tr("Application Error"), errMsg );
}

//============================================================================
void AppCommon::toGuiTodGameAction( EPluginType	pluginType, VxGUID& onlineId, ETodGameAction todGameAction )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalToGuiTodGameAction( pluginType, onlineId, todGameAction );
}

//============================================================================
void AppCommon::slotInternalToGuiTodGameAction( EPluginType  pluginType, VxGUID onlineId, ETodGameAction todGameAction )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	getTodGameMgr().toGuiTodGameAction( pluginType, onlineId, todGameAction );
}

//============================================================================
void AppCommon::slotRelayHelpButtonClicked( void )
{
	ActivityShowHelp * helpDlg = new ActivityShowHelp( *this, this );
	helpDlg->show();
}

//============================================================================
void AppCommon::slotSetRelayHelpButtonVisibility( bool isVisible )
{
	//ui.m_RelayHelpButton->setVisible( isVisible );
}

//============================================================================
void AppCommon::toGuiAssetAdded( AssetBaseInfo* assetInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	if( assetInfo->isThumbAsset() )
	{
		// TODO thumbs should only go to ThumbMgr
		// thumbs are a support asset and never used as stand alone history widget
		return;
	}

	emit signalInternalToGuiAssetAdded( *assetInfo );
}

//============================================================================
void AppCommon::slotInternalToGuiAssetAdded( AssetBaseInfo assetInfo )
{
	if( m_ToGuiActivityInterfaceBusy )
	{
		LogMsg( LOG_ERROR, "%s m_ToGuiActivityInterfaceBusy is true", __func__ );
        vx_assert( false );
	}

	//playSound( eSndDefUserBellMessage ); // consumes too much resources on scan of folder with many items
	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiAssetAdded( assetInfo );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiAssetUpdated( AssetBaseInfo* assetInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalToGuiAssetUpdated( *assetInfo );
}

//============================================================================
void AppCommon::slotInternalToGuiAssetUpdated( AssetBaseInfo assetInfo )
{
    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiAssetUpdated( assetInfo );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiAssetRemoved( AssetBaseInfo* assetInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalToGuiAssetRemoved( *assetInfo );
}

//============================================================================
void AppCommon::slotInternalToGuiAssetRemoved( AssetBaseInfo assetInfo )
{
	// when assets are removed they might call wantToGuiActivityCallbacks and change the clientList
	// check if client is in list each time to avoid out of range vector interation crash
    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiAssetUpdated( assetInfo );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiAssetXferState( VxGUID& assetUniqueId, EAssetSendState assetSendState, int param )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalToGuiAssetXferState( assetUniqueId, assetSendState, param );
}

//============================================================================
void AppCommon::slotInternalToGuiAssetXferState( VxGUID assetUniqueId, EAssetSendState assetSendState, int param )
{
    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
		return;
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiAssetXferState( assetUniqueId, assetSendState, param );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiAssetSessionHistory( AssetBaseInfo* assetInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	// use pointers because there may be a lot of history objects and would otherwise fill the queued signals/slots
	AssetBaseInfo* newAssetInfo = new AssetBaseInfo( *assetInfo );
	emit signalInternalToGuiAssetSessionHistory( newAssetInfo );
}

//============================================================================
void AppCommon::slotInternalToGuiAssetSessionHistory( AssetBaseInfo* assetInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiAssetSessionHistory( *assetInfo );
	}

	m_ToGuiActivityInterfaceBusy = false;
	delete assetInfo;
}

//============================================================================
void AppCommon::toGuiAssetAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalToGuiAssetAction( assetAction, assetId, pos0to100000 );
}

//============================================================================
void AppCommon::slotInternalToGuiAssetAction( EAssetAction assetAction, VxGUID assetId, int pos0to100000 )
{
    if( ( eAssetActionRxNotifyNewMsg == assetAction )
        || ( eAssetActionRxViewingMsg == assetAction ) )
    {
        emit signalAssetViewMsgAction( assetAction, assetId, pos0to100000 );
        return;
    }

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
    for( auto& client : m_ToGuiActivityInterfaceList )
    {
        client->toGuiClientAssetAction( assetAction, assetId, pos0to100000 );
    }

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiMultiSessionAction( EMSessionAction mSessionAction, VxGUID onlineId, int pos0to100000 )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalMultiSessionAction( onlineId, mSessionAction, pos0to100000 );
}

//============================================================================
void AppCommon::slotInternalMultiSessionAction( VxGUID onlineId, EMSessionAction mSessionAction, int pos0to100000 )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
    for( auto& client : m_ToGuiActivityInterfaceList )
    {
        client->toGuiMultiSessionAction(  mSessionAction, onlineId, pos0to100000 );
    }

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiBlobAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalBlobAction( assetAction, assetId, pos0to100000 );
}

//============================================================================
void AppCommon::slotInternalBlobAction( EAssetAction assetAction, VxGUID assetId, int pos0to100000 )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	if( ( eAssetActionRxNotifyNewMsg == assetAction )
		|| ( eAssetActionRxViewingMsg == assetAction ) )
	{
		emit signalBlobViewMsgAction( assetAction, assetId, pos0to100000 );
		return;
	}

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
	for( auto& client : m_ToGuiActivityInterfaceList )
	{
		client->toGuiClientBlobAction( assetAction, assetId, pos0to100000 );
	}

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiBlobAdded( BlobInfo* blobInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalBlobAdded( *blobInfo );
}

//============================================================================
void AppCommon::slotInternalBlobAdded( BlobInfo blobInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
    for( auto& client : m_ToGuiActivityInterfaceList )
    {
        client->toGuiBlobAdded( blobInfo );
    }

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiBlobSessionHistory( BlobInfo* blobInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalBlobSessionHistory( *blobInfo );
}

//============================================================================
void AppCommon::slotInternalBlobSessionHistory( BlobInfo blobInfo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

	m_ToGuiActivityInterfaceBusy = true;
    for( auto& client : m_ToGuiActivityInterfaceList )
    {
        client->toGuiBlobSessionHistory( blobInfo );
    }

	m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::toGuiNetworkIsTested( bool requiresRelay, std::string& ipAddr, uint16_t ipPort )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalNetworkIsTested( requiresRelay, ipAddr.c_str(), ipPort );
}

//============================================================================
void AppCommon::slotInternalNetworkIsTested( bool requiresRelay, QString ipAddr, uint16_t ipPort )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	static bool firstTime = true;
	if( firstTime )
	{
		firstTime = false;
		if( m_AppSettings.getRunOnStartupCamServer() && eFriendStateIgnore != getMyNetIdent()->getPluginPermission( ePluginTypeCamServer ) )
		{
			getEngine().fromGuiStartPluginSession( ePluginTypeCamServer, getMyNetIdent()->getMyOnlineId(), getMyNetIdent()->getMyOnlineId() );
		}
	}
}

//============================================================================
void AppCommon::okMessageBox( QString title, QString msg )
{
	QMessageBox::information( this, title, msg, QMessageBox::Ok );
}

//============================================================================
void AppCommon::okMessageBox2( QString title, const char* msg, ... )
{
	char szBuffer[2048];
	va_list argList;
	va_start(argList, msg);
	vsnprintf( szBuffer, sizeof( szBuffer ), msg, argList );
	va_end(argList);

	okMessageBox( title, szBuffer );
}

//============================================================================
bool AppCommon::yesNoMessageBox( QString title, QString msg )
{
	if( QMessageBox::Yes == QMessageBox::question( this, title, msg, QMessageBox::Yes | QMessageBox::No ) )
	{
		return true;
	}

	return false;
}

//============================================================================
bool AppCommon::yesNoMessageBox2( QString title, const char* msg, ... )
{
	char szBuffer[2048];
	va_list argList;
	va_start(argList, msg);
	vsnprintf( szBuffer, sizeof( szBuffer ), msg, argList );
	va_end(argList);

	return yesNoMessageBox( title, szBuffer );
}

//============================================================================
void AppCommon::errMessageBox( QString title, QString msg )
{
	QMessageBox::warning( this, title, msg, QMessageBox::Ok );
}

//============================================================================
void AppCommon::errMessageBox2( QString title, const char* msg, ... )
{
	char szBuffer[2048];
	va_list argList;
	va_start(argList, msg);
	vsnprintf( szBuffer, sizeof( szBuffer ), msg, argList );
	va_end(argList);

	errMessageBox( title, szBuffer );
}

//============================================================================
void AppCommon::wantToGuiActivityCallbacks(	ToGuiActivityInterface *	callback, 
											bool						wantCallback )
{
static bool actCallbackShutdownComplete = false;
	if( VxIsAppShuttingDown() )
	{
		if( actCallbackShutdownComplete )
		{
			return;
		}

		actCallbackShutdownComplete = true;
		clearToGuiActivityInterfaceList();
		return;
	}

    if( m_ToGuiActivityInterfaceBusy )
    {
        LogMsg( LOG_DEBUG, "AppCommon::%s m_ToGuiActivityInterfaceBusy true", __func__ );
        vx_assert( false );
    }

    m_ToGuiActivityInterfaceBusy = true;

	if( wantCallback )
	{
		for( auto& client : m_ToGuiActivityInterfaceList )
		{
			if( client == callback )
			{
				LogMsg( LOG_INFO, "WARNING. Ignoring New ToGuiActivityInterface.h because already in list" );
                m_ToGuiActivityInterfaceBusy = false;
				return;
			}
		}

        m_ToGuiActivityInterfaceList.emplace_back( callback );
        m_ToGuiActivityInterfaceBusy = false;
		return;
	}

	for( auto iter = m_ToGuiActivityInterfaceList.begin(); iter != m_ToGuiActivityInterfaceList.end(); ++iter )
	{
		ToGuiActivityInterface* client = *iter;
		if( client == callback )
		{
			m_ToGuiActivityInterfaceList.erase( iter );
            m_ToGuiActivityInterfaceBusy = false;
			return;
		}
	}

    m_ToGuiActivityInterfaceBusy = false;
	LogMsg( LOG_INFO, "WARNING. ToGuiActivityInterface.h remove not found in list" );
	return;
}

//============================================================================
void AppCommon::clearToGuiActivityInterfaceList( void )
{
    m_ToGuiActivityInterfaceBusy = true;
	if( m_ToGuiActivityInterfaceList.size() )
	{
		m_ToGuiActivityInterfaceList.clear();
	}

    m_ToGuiActivityInterfaceBusy = false;
}

//============================================================================
void AppCommon::wantToGuiHardwareCtrlCallbacks(	ToGuiHardwareControlInterface *	callback, bool wantCallback )
{
static bool actCallbackShutdownComplete = false;
	if( VxIsAppShuttingDown() )
	{
		if( actCallbackShutdownComplete )
		{
			return;
		}

		actCallbackShutdownComplete = true;
		clearHardwareCtrlList();
		return;
	}

	if( m_ToGuiHardwareCtrlBusy )
	{
		LogMsg( LOG_ERROR, "AppCommon::wantToGuiHardwareCtrlCallbacks do NOT call while busy" );
	}

	if( wantCallback )
	{
		for( auto& client : m_ToGuiHardwareCtrlList )
		{
			if( client == callback )
			{
				LogMsg( LOG_INFO, "WARNING. Ignoring New wantToGuiHardwareCtrlCallbacks because already in list" );
				return;
			}
		}

		m_ToGuiHardwareCtrlList.emplace_back( callback );
		return;
	}

	for( auto iter = m_ToGuiHardwareCtrlList.begin(); iter != m_ToGuiHardwareCtrlList.end(); ++iter )
	{
		ToGuiHardwareControlInterface* client = *iter;
		if( client == callback )
		{
			m_ToGuiHardwareCtrlList.erase( iter );
			return;
		}
	}

	LogMsg( LOG_INFO, "WARNING. ToGuiHardwareControlInterface remove not found in list" );
	return;
}

//============================================================================
void AppCommon::clearHardwareCtrlList( void )
{
	if( m_ToGuiHardwareCtrlList.size() )
	{
		m_ToGuiHardwareCtrlList.clear();
	}
}

//============================================================================
void AppCommon::wantToGuiUserUpdateCallbacks( ToGuiUserUpdateInterface * callback, bool wantCallback )
{
	if( !callback || (ToGuiUserUpdateInterface *)0xFFFFFFFFFFFFFFFF == callback )
	{
		LogMsg( LOG_ERROR, "AppCommon::wantToGuiUserUpdateCallbacks invalid param" );
		vx_assert( false );
		return;
	}

    static bool userCallbackShutdownComplete = false;
    if( VxIsAppShuttingDown() )
    {
        if( userCallbackShutdownComplete )
        {
            return;
        }

        userCallbackShutdownComplete = true;
        clearUserUpdateClientList();
        return;
    }

	if( m_ToGuiUserUpdateClientBusy )
	{
		LogMsg( LOG_INFO, "AppCommon::wantToGuiUserUpdateCallbacks do NOT call while busy" );
	}

    if( wantCallback )
    {
        for( auto client : m_ToGuiUserUpdateClientList )
        {
            if( client == callback )
            {
                LogMsg( LOG_INFO, "WARNING. Ignoring New wantToGuiUserUpdateCallbacks because already in list" );
                return;
            }
        }

        m_ToGuiUserUpdateClientList.emplace_back( callback );
        return;
    }

    for( auto iter = m_ToGuiUserUpdateClientList.begin(); iter != m_ToGuiUserUpdateClientList.end(); ++iter )
    {
        ToGuiUserUpdateInterface* client = *iter;
        if( client == callback )
        {
            m_ToGuiUserUpdateClientList.erase( iter );
            return;
        }
    }

    LogMsg( LOG_INFO, "WARNING. ToGuiUserUpdateInterface remove not found in list" );
    return;
}

//============================================================================
void AppCommon::clearUserUpdateClientList( void )
{
    if( m_ToGuiUserUpdateClientList.size() )
    {
        m_ToGuiUserUpdateClientList.clear();
    }
}

//============================================================================
void AppCommon::refreshFriend( VxGUID& onlineId )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	VxGUID friendId( onlineId.getVxGUIDHiPart(), onlineId.getVxGUIDLoPart() );
	emit signalRefreshFriend( friendId );
}

//============================================================================
void AppCommon::onMessengerReady( bool isReady )
{
	if( isReady != m_IsMessengerReady )
	{
		m_IsMessengerReady = isReady;
		m_ThumbMgr.onMessengerReady( isReady );
		m_UserMgr.onMessengerReady( isReady );
		m_OfferMgr.onMessengerReady( isReady );
		m_HostJoinMgr.onMessengerReady( isReady );
		m_UserJoinMgr.onMessengerReady( isReady );
		m_WebPageMgr.onMessengerReady( isReady );
		m_ConnectIdListMgr.onMessengerReady( isReady );
		checkSystemReady();
	}
}

//============================================================================
void AppCommon::onUserLoggedOn( void )
{
	if( !m_IsLoggedOn )
	{
		m_IsLoggedOn = true;
		checkSystemReady();
	}
}

//============================================================================
bool AppCommon::checkSystemReady( void )
{
	if( !m_IsGuiSystemReady && m_IsMessengerReady && m_IsLoggedOn )
	{
		// one time only each application and user ready at startup
		m_IsGuiSystemReady = true;
		m_PushToTalkMgr.onSystemReady();
		
		m_ThumbMgr.onSystemReady( m_IsGuiSystemReady );
		m_UserMgr.onSystemReady( m_IsGuiSystemReady );
		m_OfferMgr.onSystemReady( m_IsGuiSystemReady );
		m_HostJoinMgr.onSystemReady( m_IsGuiSystemReady );
		m_UserJoinMgr.onSystemReady( m_IsGuiSystemReady );
		m_WebPageMgr.onSystemReady( m_IsGuiSystemReady );
		m_ConnectIdListMgr.onSystemReady( m_IsGuiSystemReady );

		emit signalSystemReady( m_IsGuiSystemReady );

		checkReadyToLaunchAfterLogonApplets();

		// qt camera for android is very processor sensitive so start as late as possible
		//m_CamLogic.camLogicStartup();
	}

	return m_IsGuiSystemReady;
}

//============================================================================
std::string AppCommon::getUserName( const VxGUID& onlineId )
{
    return m_UserMgr.getUserOnlineName( const_cast<VxGUID&>(onlineId) );
}

//============================================================================
std::string AppCommon::describeConnectId( ConnectId& connectionId )
{
	std::string desc = "skt id ";
	desc += connectionId.getSocketId().toHexString();
	desc += connectionId.isRelayed() ? " relayed " : " direct ";
	desc += describeGroupieId( connectionId.getGroupieId() );
	return desc;
}

//============================================================================
std::string AppCommon::describeHostedId( HostedId& hostedId )
{
	std::string adminDesc = DescribeHostType( hostedId.getHostType() );
	adminDesc += " admin ";
	adminDesc += describeUser( hostedId.getHostOnlineId() );
	return adminDesc;
}

//============================================================================
std::string AppCommon::describeGroupieId( GroupieId& groupieId )
{
	std::string desc = DescribeHostType( groupieId.getHostType() );
	desc += describeUser( groupieId.getHostOnlineId() );
	desc += " user ";
	desc += describeUser( groupieId.getUserOnlineId() );
	return desc;
}

//============================================================================
std::string AppCommon::describeUser( VxGUID& onlineId )
{
	std::string userDesc = m_UserMgr.getUserOnlineName( onlineId );
	userDesc += " id ";
	userDesc += onlineId.toOnlineIdString();
	return userDesc;
}

//============================================================================
std::string AppCommon::describeUser( GuiUser* guiUser )
{
	if( guiUser )
	{
		std::string userDesc = guiUser->getOnlineName();
		userDesc += " id ";
		userDesc += guiUser->getMyOnlineId().toOnlineIdString();
		return userDesc;
	}
	else
	{
		return "AppCommon::describeUser null user";
	}
}


//============================================================================
bool AppCommon::hasExistingAccount( void )
{
	if( !getAccountMgr().getLastLogin().empty() )
	{
		return true;
	}

	return false;
}

//============================================================================
void AppCommon::checkIsGuiThread( void )
{
	if( !VxIsGuiThreadId() )
	{
		vx_assert( false );
		LogMsg( LOG_FATAL, "AppCommon::checkIsGuiThread failed" );
		exit(99);
	}
}

//============================================================================
bool AppCommon::getIsMyPortOpen( void )
{
	return getEngine().getNetStatusAccum().isRxPortOpen();
}

//============================================================================
void AppCommon::toGuiAdminAvail( GroupieId& adminGroupieId, bool adminAvail )
{
	emit signalInternalToGuiAdminAvail( adminGroupieId, adminAvail );
}

//============================================================================
void AppCommon::slotInternalToGuiAdminAvail( GroupieId adminGroupieId, bool adminAvail )
{
	m_AdminAvailMgr.toGuiAdminAvail( adminGroupieId, adminAvail );
	m_UserMgr.refreshUser( adminGroupieId.getHostOnlineId() );
}
