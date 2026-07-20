/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"
#include "NlcUrl.h"
#include "NlcCoreUtil.h"

#include "CompileInfo.h"

#include "LangInfoKodi.h"

#include "HDRStatus.h"

#include "PlayListPlayer.h"
#include "ServiceManager.h"

#include "ApplicationActionListeners.h"
#include "ApplicationPlayer.h"

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"

#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlProfiler.h"
#include "guilib/GUIFontManager.h"
#include "guilib/StereoscopicsManager.h"
#include "guilib/TextureManager.h"

#include "music/MusicLibraryQueue.h"
#include "music/tags/MusicInfoTag.h"

#include "platform/Environment.h"
#include "playlists/PlayListFactory.h"
#include "threads/SystemClock.h"
#include "utils/ContentUtils.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/Screenshot.h"
#include "utils/Variant.h"
#include "video/Bookmark.h"
#include "video/VideoLibraryQueue.h"

#include "GUILargeTextureManager.h"

#include "GUIUserMessages.h"
#include "SectionLoader.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"

#include "guilib/LocalizeStrings.h"

#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "playlists/PlayList.h"
#include "playlists/SmartPlayList.h"

#include "profiles/ProfileManager.h"

#include "threads/SingleLock.h"
#include "utils/CPUInfo.h"
#include "utils/FileExtensionProvider.h"
#include "utils/RegExp.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/WindowSystemFactory.h"

#include "interfaces/AnnouncementManager.h"
#include "settings/DisplaySettings.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include <cmath>

#if defined(TARGET_OS_WINDOWS)
#include "threads/platform/win/Win32Exception.h"
#endif

#include "music/infoscanner/MusicInfoScanner.h"
#include "music/MusicUtils.h"
#include "music/MusicThumbLoader.h"

 // Windows includes
#include "guilib/GUIWindowManager.h"
#include "video/PlayerController.h"

// Dialog includes

#include "video/dialogs/GUIDialogVideoBookmarks.h"

#ifdef TARGET_WINDOWS
#include "platform/win32/win32util.h"
#endif

#ifdef TARGET_DARWIN_OSX
#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/osx/XBMCHelper.h"
#endif
#ifdef TARGET_DARWIN
#include "platform/darwin/DarwinUtils.h"
#endif

#include "storage/MediaManager.h"
#include "utils/AlarmClock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#if defined(TARGET_OS_ANDROID) || defined(TARGET_OS_LINUX)
#include "platform/linux/XHandle.h"
//#include "platform/linux/PlatformLinux.h"
#endif

#if defined(TARGET_OS_ANDROID)
# include "platform/nlc/KodiNlcApp.h"
#endif // defined(TARGET_OS_ANDROID) && !defined(HAVE_NLC_GUI)

#ifdef TARGET_OS_WINDOWS
#include "platform/Environment.h"
#endif

//TODO: XInitThreads
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

#include "FileItem.h"

#include "cores/FFmpeg.h"
#include "pictures/GUIWindowSlideShow.h"
#include "utils/CharsetConverter.h"

#include "settings/SettingsComponent.h"
#include "settings/Settings.h"

#include "AppInboundProtocol.h"

#include <mutex>

#include "MediaPlayerNlc.h"

#include <GuiInterface/INlcRender.h>

CApplication& GetKodiInstance()
{
    return GetNlcPlayerInstance();
}

using namespace ADDON;
using namespace XFILE;

//using namespace VIDEO;
using namespace MUSIC_INFO;

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace ActiveAE;

using namespace XbmcThreads;
using namespace std::chrono_literals;

using KODI::MESSAGING::HELPERS::DialogResponse;

using namespace std::chrono_literals;

#define MAX_FFWD_SPEED 5

//
// Utility function used to copy files from the application bundle
// over to the user data directory in Application Support/Kodi.
//
static void CopyUserDataIfNeeded( const std::string& strPath, const std::string& file, const std::string& destname = "" )
{
    std::string destPath;
    if( destname == "" )
        destPath = URIUtils::AddFileToFolder( strPath, file );
    else
        destPath = URIUtils::AddFileToFolder( strPath, destname );

    if( !CFile::Exists( destPath ) )
    {
        // special://xbmc 	Kodi's installation root directory. This path is read-only contains the Kodi binary, support libraries and default configuration files, skins, scripts and plugins. Users should not modify files or install addons in this directory. special://xbmc 	Kodi's installation root directory. This path is read-only contains the Kodi binary, support libraries and default configuration files, skins, scripts and plugins. Users should not modify files or install addons in this directory.
        // special://home 	Kodi's user specific (OS user) configuration directory. This path contains a writable version of the special://xbmc directories. Any addons should be installed here.
        // special://masterprofile 	Kodi's main configuration directory. Normally located at special://home/userdata, this directory contains global settings and sources, as well as any Kodi profile directories. Normally special://home/userdata
        // special://userdata 	Alias from special://masterprofile.

        // need to copy it across
        std::string srcFile = CSpecialProtocol::TranslatePath( "special://xbmc/" ) + "userdata/" + file;
        if( CFile::Exists( srcFile ) )
        {
            std::string destPath = URIUtils::AddFileToFolder( "special://masterprofile/", file );
            std::string destFile = CSpecialProtocol::TranslatePath( destPath.c_str() );
            LogMsg( LOG_DEBUG, "%s Copy %s to %s", __func__, srcFile.c_str(), destFile.c_str() );

            if( !CFile::Copy( srcFile, destFile ) |
                !CFile::Exists( destPath ) )
            {
                LogMsg( LOG_ERROR, "%s failed to copy %s to %s", __func__, srcFile.c_str(), destFile.c_str() );
            }
        }
        else
        {
            LogMsg( LOG_ERROR, "%s source file %s does not exist", __func__, srcFile.c_str() );
        }
    }
}

CApplication::CApplication( void )
    :
    m_WaitingExternalCalls( 0 )
{
    TiXmlBase::SetCondenseWhiteSpace( false );

#ifdef HAVE_X11
    XInitThreads();
#endif

    // register application components
    RegisterComponent( std::make_shared<CApplicationActionListeners>( m_critSection ) );
    RegisterComponent( std::make_shared<CApplicationPlayer>() );
}

CApplication::~CApplication( void )
{
    //if( !VxIsAppShuttingDown() )
    //{
    //    DeregisterComponent( typeid(CApplicationPlayer) );
    //    DeregisterComponent( typeid(CApplicationActionListeners) );
    //}
}

bool CApplication::OnEvent( XBMC_Event& newEvent )
{
    std::unique_lock<CCriticalSection> lock( m_portSection );
    m_portEvents.push_back( newEvent );
    return true;
}

void CApplication::HandlePortEvents()
{
    std::unique_lock<CCriticalSection> lock( m_portSection );
    while( !m_portEvents.empty() )
    {
        auto newEvent = m_portEvents.front();
        m_portEvents.pop_front();
        CSingleExit lock( m_portSection );
        switch( newEvent.type )
        {
        case XBMC_QUIT:
            if( !m_bStop )
                CServiceBroker::GetAppMessenger()->PostMsg( TMSG_QUIT );
            break;
        case XBMC_VIDEORESIZE:
            if( m_RenderSystemInitialized )
            {           
                CServiceBroker::GetWinSystem()->ResizeWindow( newEvent.resize.w, newEvent.resize.h, 0, 0 );
                CServiceBroker::GetWinSystem()->GetGfxContext().ApplyWindowResize( newEvent.resize.w, newEvent.resize.h );

                const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
                settings->SetInt( CSettings::SETTING_WINDOW_WIDTH, newEvent.resize.w );
                settings->SetInt( CSettings::SETTING_WINDOW_HEIGHT, newEvent.resize.h );
            }

            break;

        case XBMC_FULLSCREEN_UPDATE:
        {
            //if( CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreen )
            //{
            //    CServiceBroker::GetWinSystem()->ResizeWindow( newEvent.resize.w, newEvent.resize.h, 0, 0 );
            //}
            break;
        }
        case XBMC_VIDEOMOVE:
        {
            CServiceBroker::GetWinSystem()->OnMove( newEvent.move.x, newEvent.move.y );
        }
        break;
        case XBMC_MODECHANGE:
            CServiceBroker::GetWinSystem()->GetGfxContext().ApplyModeChange( newEvent.mode.res );
            break;
        case XBMC_SCREENCHANGE:
            CServiceBroker::GetWinSystem()->OnChangeScreen( newEvent.screen.screenIdx );
            break;
        case XBMC_USEREVENT:
            CServiceBroker::GetAppMessenger()->PostMsg( static_cast<uint32_t>(newEvent.user.code) );
            break;
        case XBMC_SETFOCUS:
        {

            // Send a mouse motion event with no dx,dy for getting the current guiitem selected
            OnAction( CAction( ACTION_MOUSE_MOVE, 0, static_cast<float>(newEvent.focus.x), static_cast<float>(newEvent.focus.y), 0, 0 ) );
            break;
        }
        default:
            //CServiceBroker::GetInputManager().OnEvent( newEvent );
            break;
        }
    }
}

//extern "C" void __stdcall init_emu_environ();
//extern "C" void __stdcall update_emu_environ();
//extern "C" void __stdcall cleanup_emu_environ();

bool CApplication::Create()
{

    m_bStop = false;
    setRenderThreadId( CThread::GetCurrentThreadId() );

    CServiceBroker::CreateLogging();
    const auto settingsComponent = std::make_shared<CSettingsComponent>();
    settingsComponent->Initialize();
    CServiceBroker::RegisterSettingsComponent(settingsComponent);

    CServiceBroker::RegisterCPUInfo( CCPUInfo::GetCPUInfo() );

    // Register JobManager service
    CServiceBroker::RegisterJobManager( std::make_shared<CJobManager>() );

    // Announcement service
    m_pAnnouncementManager = std::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
    m_pAnnouncementManager->Start();
    CServiceBroker::RegisterAnnouncementManager( m_pAnnouncementManager );

    const auto appMessenger = std::make_shared<CApplicationMessenger>();
    CServiceBroker::RegisterAppMessenger( appMessenger );

    m_ServiceManager.reset( new CServiceManager() );

    if( LogEnabled( eLogPlayerNlc ) )
    {
        CSpecialProtocol::LogPaths();
    }

    if( !m_ServiceManager->InitStageOne() )
    {
        onInitLevel( 1, false );
        return false;
    }

    onInitLevel( 1, true );

    // here we register all global classes for the CApplicationMessenger,
    // after that we can send messages to the corresponding modules
    appMessenger->RegisterReceiver( this );
    //appMessenger->RegisterReceiver( &CServiceBroker::GetPlaylistPlayer() );
    appMessenger->SetGUIThread( CThread::GetCurrentThreadId() );
    appMessenger->SetProcessThread( CThread::GetCurrentThreadId() );

    // Init our DllLoaders emu env
    //init_emu_environ();

    PrintStartupLog();

    // initialize network protocols
    //avformat_network_init();
    // set avutil callback
    av_log_set_callback( ff_avutil_log );

    CLog::Log( LOGINFO, "loading settings" );

    if( !settingsComponent->Load() )
    {
        LogMsg( LOG_ERROR, "%s failed to load settings", __func__ );
        return false;
    }

    CLog::Log( LOGINFO, "creating subdirectories" );
    //const std::shared_ptr<CProfileManager> profileManager = settingsComponent->GetProfileManager();
    //const std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
    //CLog::Log( LOGINFO, "userdata folder: %s",
    //           NlcUrl::GetRedacted( profileManager->GetProfileUserDataFolder() ).c_str() );
    //CLog::Log( LOGINFO, "recording folder: %s",
    //           NlcUrl::GetRedacted( settings->GetString( CSettings::SETTING_AUDIOCDS_RECORDINGPATH ) ).c_str() );
    //CLog::Log( LOGINFO, "screenshots folder: %s",
    //           NlcUrl::GetRedacted( settings->GetString( CSettings::SETTING_DEBUG_SCREENSHOTPATH ) ).c_str() );
    //CDirectory::Create( profileManager->GetUserDataFolder() );
    //CDirectory::Create( profileManager->GetProfileUserDataFolder() );
    //profileManager->CreateProfileFolders();

    //update_emu_environ();//apply the GUI settings

    // application inbound service
    m_pAppPort = std::make_shared<CAppInboundProtocol>( *this );
    CServiceBroker::RegisterAppPort( m_pAppPort );

    if( !m_ServiceManager->InitStageTwo() )
    {
        onInitLevel( 2, false );
        return false;
    }

    onInitLevel( 2, true );

    m_pActiveAE.reset( new ActiveAE::CActiveAE() );
    CServiceBroker::RegisterAE( m_pActiveAE.get() );

    CUtil::InitRandomSeed();

    if( !m_ServiceManager->InitStageThree( settingsComponent->GetProfileManager() ) )
    {
        onInitLevel( 3, false );
        return false;
    }

    onInitLevel( 3, true );

    m_lastRenderTime = std::chrono::steady_clock::now();
    return true;
}

bool CApplication::CreateGUI()
{
    m_frameMoveGuard.lock();

    std::string windowSystem = "qt";

    m_pWinSystem = KODI::WINDOWING::CWindowSystemFactory::CreateWindowSystem( windowSystem );

    if( !m_pWinSystem )
    {
        CLog::Log( LOGFATAL, "CApplication::%s - unable to create windowing system", __FUNCTION__ );
        return false;
    }

    CServiceBroker::RegisterWinSystem( m_pWinSystem.get() );

    if( !m_pWinSystem->InitWindowSystem() )
    {
        CLog::Log( LOGDEBUG, "CApplication::%s - unable to init %s windowing system", __FUNCTION__,
                    windowSystem.c_str() );
        m_pWinSystem->DestroyWindowSystem();
        m_pWinSystem.reset();
        CServiceBroker::UnregisterWinSystem();
        return false;
    }
    else
    {
        CLog::Log( LOGINFO, "CApplication::%s - using the %s windowing system", __FUNCTION__,
                    windowSystem.c_str() );
    }

    // Retrieve the matching resolution based on GUI settings
    //bool sav_res = false;
    //CDisplaySettings::GetInstance().SetCurrentResolution( CDisplaySettings::GetInstance().GetDisplayResolution() );
    //CLog::Log( LOGINFO, "Checking resolution %d",
    //           CDisplaySettings::GetInstance().GetCurrentResolution() );
    //if( !CServiceBroker::GetWinSystem()->GetGfxContext().IsValidResolution( CDisplaySettings::GetInstance().GetCurrentResolution() ) )
    //{
    //    CLog::Log( LOGINFO, "Setting safe mode %d", RES_DESKTOP );
    //    // defer saving resolution after window was created
    //    CDisplaySettings::GetInstance().SetCurrentResolution( RES_DESKTOP );
    //    sav_res = true;
    //}

    //// update the window resolution
    //const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    //CServiceBroker::GetWinSystem()->SetWindowResolution( settings->GetInt( CSettings::SETTING_WINDOW_WIDTH ), settings->GetInt( CSettings::SETTING_WINDOW_HEIGHT ) );

    //if( CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_startFullScreen && CDisplaySettings::GetInstance().GetCurrentResolution() == RES_WINDOW )
    //{
    //    // defer saving resolution after window was created
    //    CDisplaySettings::GetInstance().SetCurrentResolution( RES_DESKTOP );
    //    sav_res = true;
    //}

    //if( !CServiceBroker::GetWinSystem()->GetGfxContext().IsValidResolution( CDisplaySettings::GetInstance().GetCurrentResolution() ) )
    //{
    //    // Oh uh - doesn't look good for starting in their wanted screenmode
    //    CLog::Log( LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode" );
    //    CDisplaySettings::GetInstance().SetCurrentResolution( RES_DESKTOP );
    //    sav_res = true;
    //}

    if( !InitWindow() )
    {
        return false;
    }

    INlcRender::getINlcRender().verifyGlState();

    //if( sav_res )
    //    CDisplaySettings::GetInstance().SetCurrentResolution( RES_DESKTOP, true );

    m_pGUI.reset( new CGUIComponent() );
    m_pGUI->Init();

    INlcRender::getINlcRender().verifyGlState();

    // Splash requires gui component!!
    //CServiceBroker::GetRenderSystem()->ShowSplash( "" );


    RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
    CLog::Log( LOGINFO, "GUI format %dx%d, Display %s", info.iWidth, info.iHeight, info.strMode.c_str() );

    INlcRender::getINlcRender().verifyGlState();
    return true;
}

bool CApplication::InitWindow( RESOLUTION res )
{
    if( res == RES_INVALID )
    {
        res = RES_WINDOW;
        CDisplaySettings::GetInstance().SetCurrentResolution(res);
    }
    //    res = CDisplaySettings::GetInstance().GetCurrentResolution();

    // update the window resolution
    int winWidth{ 0 };
    int winHeight{ 0 };
    getRenderWindowSize( winWidth, winHeight );

    CDisplaySettings::GetInstance().SetResolutionInfo( res, winWidth, winHeight );

    if( !CServiceBroker::GetWinSystem()->CreateNewWindow( CSysInfo::GetAppName(),
                                                          false, CDisplaySettings::GetInstance().GetResolutionInfo( res ) ) )
    {
        CLog::Log( LOGFATAL, "CApplication::Create: Unable to create window" );
        return false;
    }

    if( !CServiceBroker::GetRenderSystem()->InitRenderSystem() )
    {
        CLog::Log( LOGFATAL, "CApplication::Create: Unable to init rendering system" );
        return false;
    }

    // at this point opengl should be initialized and ready
    INlcRender::getINlcRender().verifyGlState();

    // set GUI res and force the clear of the screen
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution( res, false );

    INlcRender::getINlcRender().verifyGlState();
    m_RenderSystemInitialized = true;
    return true;
}

bool CApplication::Initialize()
{
    m_pActiveAE->Start();

    //// load the language and its translated strings
    //if( !LoadLanguage( false ) )
    //    return false;

    //// load media manager sources (e.g. root addon type sources depend on language strings to be available)
    //CServiceBroker::GetMediaManager().LoadSources();


    CEvent event( true );


    //CServiceBroker::GetRenderSystem()->ShowSplash( "" );

    // Initialize GUI font manager to build/update fonts cache
    //! @todo Move GUIFontManager into service broker and drop the global reference
    event.Reset();
    GUIFontManager& guiFontManager = g_fontManager;
    CServiceBroker::GetJobManager()->Submit( [&guiFontManager, &event]() {
        guiFontManager.Initialize();
        //event.Set(); // TODO track down sometimes crash in notifyAll
                                             } );

    //CServiceBroker::GetRenderSystem()->ShowSplash( "" );

    // GUI depends on seek handler
    GetComponent<CApplicationPlayer>()->GetSeekHandler().Configure();


    //bool uiInitializationFinished = false;

    //if( CServiceBroker::GetGUI()->GetWindowManager().Initialized() )
    //{
    //    CServiceBroker::GetGUI()->GetWindowManager().CreateWindows();

    //    event.Reset();

    //    // Start splashscreen and load skin
    //    CServiceBroker::GetRenderSystem()->ShowSplash( "" );

    //    CServiceBroker::RegisterTextureCache( std::make_shared<CTextureCache>() );

    //    // initialize splash window after splash screen disappears
    //    // because we need a real window in the background which gets
    //    // rendered while we load the main window or enter the master lock key
    //    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow( WINDOW_SPLASH );

    //    uiInitializationFinished = true;

    //}
    //else //No GUI Created
    //{
    //    uiInitializationFinished = true;
    //}

    onInitLevel( 3, true );

    g_sysinfo.Refresh();

    CLog::Log( LOGINFO, "removing tempfiles" );
    CUtil::RemoveTempFiles();

    m_slowTimer.StartZero();

    // register action listeners
    const auto appListener = GetComponent<CApplicationActionListeners>();
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    appListener->RegisterActionListener( &appPlayer->GetSeekHandler() );
    //appListener->RegisterActionListener( &CPlayerController::GetInstance() );

    CLog::Log( LOGINFO, "initialize done" );

    // if the user interfaces has been fully initialized let everyone know
    //if( uiInitializationFinished )
    //{
    //    CGUIMessage msg( GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY );
    //    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage( msg );
    //}


    return true;
}


void CApplication::Render()
{
    CServiceBroker::GetAppMessenger()->ProcessMessages();

    // do not render if we are stopped or in background
    if( m_bStop )
        return;

    const auto appPlayer = GetComponent<CApplicationPlayer>();

    bool hasRendered = false;

    // Whether externalplayer is playing and we're unfocused
    //bool extPlayerActive = appPlayer->IsExternalPlaying() && !m_AppFocused;


    if( appPlayer->IsRenderingVideo() )
    {
        if( !CServiceBroker::GetRenderSystem()->BeginRender() )
            return;

        int winWidth{ 0 };
        int winHeight{ 0 };
        getRenderWindowSize( winWidth, winHeight );
        if( winWidth > 1 && winHeight > 1 )
        {
            CServiceBroker::GetWinSystem()->GetGfxContext().SetViewWindow( 0, 0, winWidth, winHeight );

            //CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), false);

            appPlayer->Render(true, 255);

            //m_lastRenderTime = std::chrono::steady_clock::now();
            hasRendered = true;
        }

        CServiceBroker::GetRenderSystem()->EndRender();

        //CServiceBroker::GetWinSystem()->GetGfxContext().Flip( hasRendered, false );
        INlcRender::getINlcRender().presentRender( true, false );

        //CTimeUtils::UpdateFrameTime( hasRendered );
    }
}

bool CApplication::OnAction( const CAction& action )
{
    // special case for switching between GUI & fullscreen mode.
    if( action.GetID() == ACTION_SHOW_GUI )
    { // Switch to fullscreen mode if we can

    }

    const auto appPlayer = GetComponent<CApplicationPlayer>();

    if( action.GetID() == ACTION_TOGGLE_FULLSCREEN )
    {
        //CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
        //appPlayer->TriggerUpdateResolution();
        return true;
    }

    //if( action.IsMouse() )
    //    CServiceBroker::GetInputManager().SetMouseActive( true );


    // The action PLAYPAUSE behaves as ACTION_PAUSE if we are currently
    // playing or ACTION_PLAYER_PLAY if we are seeking (FF/RW) or not playing.
    if( action.GetID() == ACTION_PLAYER_PLAYPAUSE )
    {

        if( appPlayer->IsPlaying() && appPlayer->GetPlaySpeed() == 1  )
        {
            onPlayPause( true );
            return OnAction( CAction( ACTION_PAUSE ) );
        }
        else
        {
            onPlayPause( false );
            return OnAction( CAction( ACTION_PLAYER_PLAY ) );
        }
    }


    // handle extra global presses

    // notify action listeners
    if( GetComponent<CApplicationActionListeners>()->NotifyActionListeners( action ) )
        return true;

    // screenshot : take a screenshot :)
    if( action.GetID() == ACTION_TAKE_SCREENSHOT )
    {
        //CScreenShot::TakeScreenshot();
        return true;
    }
    // Display HDR : toggle HDR on/off
    if( action.GetID() == ACTION_HDR_TOGGLE )
    {
        //// Only enables manual HDR toggle if no video is playing or auto HDR switch is disabled
        //if( appPlayer->IsPlayingVideo() &&
        //    CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        //        CServiceBroker::GetWinSystem()->SETTING_WINSYSTEM_IS_HDR_DISPLAY ) )
        //    return true;

        //HDR_STATUS hdrStatus = CServiceBroker::GetWinSystem()->ToggleHDR();

        //if( hdrStatus == HDR_STATUS::HDR_OFF )
        //{
        //    LogMsg( LOG_VERBOSE, "%s ToggleHDR ON", __func__ );
        //    CGUIDialogKaiToast::QueueNotification( CGUIDialogKaiToast::Info, g_localizeStrings.Get( 34220 ),
        //                                           g_localizeStrings.Get( 34221 ) );
        //}
        //else if( hdrStatus == HDR_STATUS::HDR_ON )
        //{
        //    LogMsg( LOG_VERBOSE, "%s ToggleHDR OFF", __func__ );
        //    CGUIDialogKaiToast::QueueNotification( CGUIDialogKaiToast::Info, g_localizeStrings.Get( 34220 ),
        //                                           g_localizeStrings.Get( 34222 ) );
        //}
        return true;
    }
    // Tone Mapping : switch to next tone map method
    if( action.GetID() == ACTION_CYCLE_TONEMAP_METHOD )
    {
        // Only enables tone mapping switch if display is not HDR capable or HDR is not enabled
        if( CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CServiceBroker::GetWinSystem()->SETTING_WINSYSTEM_IS_HDR_DISPLAY ) &&
            CServiceBroker::GetWinSystem()->IsHDRDisplay() )
            return true;

        if( appPlayer->IsPlayingVideo() )
        {
            CVideoSettings vs = appPlayer->GetVideoSettings();
            vs.m_ToneMapMethod = static_cast<ETONEMAPMETHOD>(static_cast<int>(vs.m_ToneMapMethod) + 1);
            if( vs.m_ToneMapMethod >= VS_TONEMAPMETHOD_MAX )
                vs.m_ToneMapMethod =
                static_cast<ETONEMAPMETHOD>(static_cast<int>(VS_TONEMAPMETHOD_OFF) + 1);

            appPlayer->SetVideoSettings( vs );

            int code = 0;
            switch( vs.m_ToneMapMethod )
            {
            case VS_TONEMAPMETHOD_REINHARD:
                code = 36555;
                break;
            case VS_TONEMAPMETHOD_ACES:
                code = 36557;
                break;
            case VS_TONEMAPMETHOD_HABLE:
                code = 36558;
                break;
            default:
                throw std::logic_error( "Tonemapping method not found. Did you forget to add a mapping?" );
            }
            //CGUIDialogKaiToast::QueueNotification( CGUIDialogKaiToast::Info, g_localizeStrings.Get( 34224 ),
            //                                       g_localizeStrings.Get( code ), 1000, false, 500 );
        }
        return true;
    }
    // built in functions : execute the built-in
    if( action.GetID() == ACTION_BUILT_IN_FUNCTION )
    {

        return true;
    }


    // show info : Shows the current video or song information
    if( action.GetID() == ACTION_SHOW_INFO )
    {
        //CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().ToggleShowInfo();
        return true;
    }

    if( action.GetID() == ACTION_SET_RATING && appPlayer->IsPlayingAudio() )
    {
        //int userrating = MUSIC_UTILS::ShowSelectRatingDialog( m_itemCurrentFile->GetMusicInfoTag()->GetUserrating() );
        //if( userrating < 0 ) // Nothing selected, so user rating unchanged
        //    return true;
        //userrating = std::min( userrating, 10 );
        //if( userrating != m_itemCurrentFile->GetMusicInfoTag()->GetUserrating() )
        //{
        //    m_itemCurrentFile->GetMusicInfoTag()->SetUserrating( userrating );
        //    // Mirror changes to GUI item
        //    CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem( *m_itemCurrentFile );

        //    // Asynchronously update song userrating in music library
        //    MUSIC_UTILS::UpdateSongRatingJob( m_itemCurrentFile, userrating );

        //    // Tell all windows (e.g. playlistplayer, media windows) to update the fileitem
        //    CGUIMessage msg( GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile );
        //    CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
        //}
        return true;
    }

    else if( (action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) &&
             appPlayer->IsPlayingAudio() )
    {
        //int userrating = m_itemCurrentFile->GetMusicInfoTag()->GetUserrating();
        //bool needsUpdate( false );
        //if( userrating > 0 && action.GetID() == ACTION_DECREASE_RATING )
        //{
        //    m_itemCurrentFile->GetMusicInfoTag()->SetUserrating( userrating - 1 );
        //    needsUpdate = true;
        //}
        //else if( userrating < 10 && action.GetID() == ACTION_INCREASE_RATING )
        //{
        //    m_itemCurrentFile->GetMusicInfoTag()->SetUserrating( userrating + 1 );
        //    needsUpdate = true;
        //}
        //if( needsUpdate )
        //{
        //    // Mirror changes to current GUI item
        //    CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem( *m_itemCurrentFile );

        //    // Asynchronously update song userrating in music library
        //    MUSIC_UTILS::UpdateSongRatingJob( m_itemCurrentFile, m_itemCurrentFile->GetMusicInfoTag()->GetUserrating() );

        //    // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
        //    CGUIMessage msg( GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile );
        //    CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
        //}

        return true;
    }
    else if( (action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) &&
             appPlayer->IsPlayingVideo() )
    {
        //int rating = m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating;
        //bool needsUpdate( false );
        //if( rating > 1 && action.GetID() == ACTION_DECREASE_RATING )
        //{
        //    m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating = rating - 1;
        //    needsUpdate = true;
        //}
        //else if( rating < 10 && action.GetID() == ACTION_INCREASE_RATING )
        //{
        //    m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating = rating + 1;
        //    needsUpdate = true;
        //}
        //if( needsUpdate )
        //{
        //    // Mirror changes to GUI item
        //    CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem( *m_itemCurrentFile );

        //    CVideoDatabase db;
        //    if( db.Open() )
        //    {
        //        db.SetVideoUserRating( m_itemCurrentFile->GetVideoInfoTag()->m_iDbId,
        //                               m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating,
        //                               m_itemCurrentFile->GetVideoInfoTag()->m_type );
        //        db.Close();
        //    }
        //    // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
        //    CGUIMessage msg( GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile );
        //    CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
        //}
        return true;
    }

    // Now check with the playlist player if action can be handled.
    // In case of ACTION_PREV_ITEM, we only allow the playlist player to take it if we're less than ACTION_PREV_ITEM_THRESHOLD seconds into playback.
    if( !(action.GetID() == ACTION_PREV_ITEM && appPlayer->CanSeek() &&
           GetTime() > ACTION_PREV_ITEM_THRESHOLD) )
    {
        if( CServiceBroker::GetPlaylistPlayer().OnAction( action ) )
            return true;
    }


    //bool bNotifyPlayer = false;
    //if( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO )
    //    bNotifyPlayer = true;
    //else if( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME )
    //    bNotifyPlayer = true;

    //else if( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_DIALOG_VIDEO_OSD )
    //{
    //    switch( action.GetID() )
    //    {
    //    case ACTION_NEXT_ITEM:
    //    case ACTION_PREV_ITEM:
    //    case ACTION_CHANNEL_UP:
    //    case ACTION_CHANNEL_DOWN:
    //        bNotifyPlayer = true;
    //        break;
    //    default:
    //        break;
    //    }
    //}
    //else if( action.GetID() == ACTION_STOP )
    //    bNotifyPlayer = true;

    //if( bNotifyPlayer )
    //{
    //    if( appPlayer->OnAction( action ) )
    //        return true;
    //}

    // stop : stops playing current audio song
    if( action.GetID() == ACTION_STOP )
    {
        StopPlaying();
        return true;
    }

    // In case the playlist player nor the player didn't handle PREV_ITEM, because we are past the ACTION_PREV_ITEM_THRESHOLD secs limit.
    // If so, we just jump to the start of the track.
    if( action.GetID() == ACTION_PREV_ITEM && appPlayer->CanSeek() )
    {
        SeekTime( 0 );
        appPlayer->SetPlaySpeed( 1 );
        return true;
    }

    // forward action to graphic context and see if it can handle it
    if( CServiceBroker::GetGUI()->GetStereoscopicsManager().OnAction( action ) )
        return true;

    if( appPlayer->IsPlaying() )
    {
        // forward channel switches to the player - he knows what to do
        if( action.GetID() == ACTION_CHANNEL_UP || action.GetID() == ACTION_CHANNEL_DOWN )
        {
            appPlayer->OnAction( action );
            return true;
        }

        // pause : toggle pause action
        if( action.GetID() == ACTION_PAUSE )
        {
            appPlayer->Pause();
            // go back to normal play speed on unpause
            if( !appPlayer->IsPaused() && appPlayer->GetPlaySpeed() != 1 )
                appPlayer->SetPlaySpeed( 1 );

            //CGUIComponent* gui = CServiceBroker::GetGUI();
            //if( gui )
            //    gui->GetAudioManager().Enable( appPlayer->IsPaused() );
            return true;
        }
        // play: unpause or set playspeed back to normal
        if( action.GetID() == ACTION_PLAYER_PLAY )
        {
            // if currently paused - unpause
            if( appPlayer->IsPaused() )
                return OnAction( CAction( ACTION_PAUSE ) );
            // if we do a FF/RW then go back to normal speed
            if( appPlayer->GetPlaySpeed() != 1 )
                appPlayer->SetPlaySpeed( 1 );
            return true;
        }
        if( !appPlayer->IsPaused() )
        {
            if( action.GetID() == ACTION_PLAYER_FORWARD || action.GetID() == ACTION_PLAYER_REWIND )
            {
                float playSpeed = appPlayer->GetPlaySpeed();

                if( action.GetID() == ACTION_PLAYER_REWIND && (playSpeed == 1) ) // Enables Rewinding
                    playSpeed *= -2;
                else if( action.GetID() == ACTION_PLAYER_REWIND && playSpeed > 1 ) //goes down a notch if you're FFing
                    playSpeed /= 2;
                else if( action.GetID() == ACTION_PLAYER_FORWARD && playSpeed < 1 ) //goes up a notch if you're RWing
                    playSpeed /= 2;
                else
                    playSpeed *= 2;

                if( action.GetID() == ACTION_PLAYER_FORWARD && playSpeed == -1 ) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
                    playSpeed = 1;
                if( playSpeed > 32 || playSpeed < -32 )
                    playSpeed = 1;

                appPlayer->SetPlaySpeed( playSpeed );
                return true;
            }
            else if( (action.GetAmount() || appPlayer->GetPlaySpeed() != 1) &&
                     (action.GetID() == ACTION_ANALOG_REWIND || action.GetID() == ACTION_ANALOG_FORWARD) )
            {
                // calculate the speed based on the amount the button is held down
                int iPower = (int)(action.GetAmount() * MAX_FFWD_SPEED + 0.5f);
                // amount can be negative, for example rewind and forward share the same axis
                iPower = std::abs( iPower );
                // returns 0 -> MAX_FFWD_SPEED
                int iSpeed = 1 << iPower;
                if( iSpeed != 1 && action.GetID() == ACTION_ANALOG_REWIND )
                    iSpeed = -iSpeed;
                appPlayer->SetPlaySpeed( static_cast<float>(iSpeed) );
                if( iSpeed == 1 )
                    CLog::Log( LOGDEBUG, "Resetting playspeed" );
                return true;
            }
        }
        // allow play to unpause
        else
        {
            if( action.GetID() == ACTION_PLAYER_PLAY )
            {
                // unpause, and set the playspeed back to normal
                appPlayer->Pause();

                //CGUIComponent* gui = CServiceBroker::GetGUI();
                //if( gui )
                //    gui->GetAudioManager().Enable( appPlayer->IsPaused() );

                appPlayer->SetPlaySpeed( 1 );
                return true;
            }
        }
    }


    if( action.GetID() == ACTION_SWITCH_PLAYER )
    {
        const CPlayerCoreFactory& playerCoreFactory = m_ServiceManager->GetPlayerCoreFactory();

        if( appPlayer->IsPlaying() )
        {
            std::vector<std::string> players;
            CFileItem item( *m_itemCurrentFile.get() );
            playerCoreFactory.GetPlayers( item, players );
            std::string player = playerCoreFactory.SelectPlayerDialog( players );
            if( !player.empty() )
            {
                item.SetStartOffset( CUtil::ConvertSecsToMilliSecs( GetTime() ) );
                PlayFile( item, player, true );
            }
        }
        else
        {
            std::vector<std::string> players;
            playerCoreFactory.GetRemotePlayers( players );
            std::string player = playerCoreFactory.SelectPlayerDialog( players );
            if( !player.empty() )
            {
                PlayFile( CFileItem(), player, false );
            }
        }
    }



    if( action.GetID() == ACTION_MUTE )
    {
        //const auto appVolume = GetComponent<CApplicationVolumeHandling>();
        //appVolume->ToggleMute();
        //appVolume->ShowVolumeBar( &action );
        return true;
    }

    if( action.GetID() == ACTION_TOGGLE_DIGITAL_ANALOG )
    {
        //const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
        //bool passthrough = settings->GetBool( CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH );
        //settings->SetBool( CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, !passthrough );

        //if( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SETTINGS_SYSTEM )
        //{
        //    CGUIMessage msg( GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() );
        //    CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
        //}
        return true;
    }

    // Check for global volume control
    if( (action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN)) || action.GetID() == ACTION_VOLUME_SET )
    {
//        const auto appVolume = GetComponent<CApplicationVolumeHandling>();
//        if( !appPlayer->IsPassthrough() )
//        {
//            if( appVolume->IsMuted() )
//                appVolume->UnMute();
//            float volume = appVolume->GetVolumeRatio();
//            int volumesteps = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt( CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS );
//            // sanity check
//            if( volumesteps == 0 )
//                volumesteps = 90;
//
//            // Android has steps based on the max available volume level
//#if defined(TARGET_ANDROID)
//            float step = (CApplicationVolumeHandling::VOLUME_MAXIMUM -
//                           CApplicationVolumeHandling::VOLUME_MINIMUM) /
//                CXBMCApp::GetMaxSystemVolume();
//#else
//            float step = (CApplicationVolumeHandling::VOLUME_MAXIMUM -
//                           CApplicationVolumeHandling::VOLUME_MINIMUM) /
//                volumesteps;
//
//            if( action.GetRepeat() )
//                step *= action.GetRepeat() * 50; // 50 fps
//#endif
//            if( action.GetID() == ACTION_VOLUME_UP )
//                volume += action.GetAmount() * action.GetAmount() * step;
//            else if( action.GetID() == ACTION_VOLUME_DOWN )
//                volume -= action.GetAmount() * action.GetAmount() * step;
//            else
//                volume = action.GetAmount() * step;
//            if( volume != appVolume->GetVolumeRatio() )
//                appVolume->SetVolume( volume, false );
//        }
//        // show visual feedback of volume or passthrough indicator
//        appVolume->ShowVolumeBar( &action );
        return true;
    }

    if( action.GetID() == ACTION_GUIPROFILE_BEGIN )
    {
        //CGUIControlProfiler::Instance().SetOutputFile( CSpecialProtocol::TranslatePath( "special://home/guiprofiler.xml" ) );
        //CGUIControlProfiler::Instance().Start();
        return true;
    }
    if( action.GetID() == ACTION_SHOW_PLAYLIST )
    {
        //const PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
        //if( playlistId == PLAYLIST::TYPE_VIDEO &&
        //    CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_VIDEO_PLAYLIST )
        //{
        //    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow( WINDOW_VIDEO_PLAYLIST );
        //}
        //else if( playlistId == PLAYLIST::TYPE_MUSIC &&
        //         CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() !=
        //         WINDOW_MUSIC_PLAYLIST )
        //{
        //    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow( WINDOW_MUSIC_PLAYLIST );
        //}
        return true;
    }
    return false;
}

int CApplication::GetMessageMask()
{
    return TMSG_MASK_APPLICATION;
}

void CApplication::OnApplicationMessage( ThreadMessage* pMsg )
{
    uint32_t msg = pMsg->dwMessage;
    if( msg == TMSG_SYSTEM_POWERDOWN )
    {
        return; // no shutdown
    }

    switch( msg )
    {
    case TMSG_POWERDOWN:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_POWERDOWN" );
        break;

    case TMSG_QUIT:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_QUIT" );
        Stop( EXITCODE_QUIT );
        break;

    case TMSG_SHUTDOWN:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_SHUTDOWN" );
        break;

    case TMSG_RENDERER_FLUSH:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_RENDERER_FLUSH" );
        GetComponent<CApplicationPlayer>()->FlushRenderer();
        break;

    case TMSG_HIBERNATE:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_HIBERNATE" );
        break;

    case TMSG_SUSPEND:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_SUSPEND" );
        break;

    case TMSG_RESTART:
    case TMSG_RESET:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_RESTART" );
        //if( Stop( EXITCODE_REBOOT ) )
            //CServiceBroker::GetPowerManager().Reboot();
        break;

    case TMSG_RESTARTAPP:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_RESTARTAPP" );
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
        Stop( EXITCODE_RESTARTAPP );
#endif
        break;

    case TMSG_INHIBITIDLESHUTDOWN:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_INHIBITIDLESHUTDOWN" );
        //GetComponent<CApplicationPowerHandling>()->InhibitIdleShutdown( pMsg->param1 != 0 );
        break;

    case TMSG_INHIBITSCREENSAVER:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_INHIBITSCREENSAVER" );
        //GetComponent<CApplicationPowerHandling>()->InhibitScreenSaver( pMsg->param1 != 0 );
        break;

    case TMSG_ACTIVATESCREENSAVER:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_ACTIVATESCREENSAVER" );
        //GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
        break;

    case TMSG_RESETSCREENSAVER:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_RESETSCREENSAVER" );
        //GetComponent<CApplicationPowerHandling>()->m_bResetScreenSaver = true;
        break;

    case TMSG_VOLUME_SHOW:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_VOLUME_SHOW" );
        //CAction action( pMsg->param1 );
        //GetComponent<CApplicationVolumeHandling>()->ShowVolumeBar( &action );
    }
    break;

#ifdef TARGET_ANDROID
    case TMSG_DISPLAY_SETUP:
        // We might come from a refresh rate switch destroying the native window; use the context resolution
        *static_cast<bool*>(pMsg->lpVoid) = InitWindow( CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution() );
        break;

    case TMSG_DISPLAY_DESTROY:
        *static_cast<bool*>(pMsg->lpVoid) = CServiceBroker::GetWinSystem()->DestroyWindow();
        break;
#endif

    case TMSG_START_ANDROID_ACTIVITY:
        break;

    case TMSG_NETWORKMESSAGE:
        //m_ServiceManager->GetNetwork().NetworkMessage( static_cast<CNetworkBase::EMESSAGE>(pMsg->param1),
        //                                               pMsg->param2 );
        break;

    case TMSG_SETLANGUAGE:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_NETWORKMESSAGE" );
        SetLanguage( pMsg->strParam );
        break;


    case TMSG_SWITCHTOFULLSCREEN:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_SWITCHTOFULLSCREEN" );
        CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo( true );
        break;
    }
    case TMSG_VIDEORESIZE:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_VIDEORESIZE" );
        XBMC_Event newEvent = {};
        newEvent.type = XBMC_VIDEORESIZE;
        newEvent.resize.w = pMsg->param1;
        newEvent.resize.h = pMsg->param2;
        OnEvent( newEvent );
    }
    break;

    case TMSG_SETVIDEORESOLUTION:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_SETVIDEORESOLUTION" );
        CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution( static_cast<RESOLUTION>(pMsg->param1), pMsg->param2 == 1 );
        break;

    case TMSG_TOGGLEFULLSCREEN:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_TOGGLEFULLSCREEN" );
        CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
        GetComponent<CApplicationPlayer>()->TriggerUpdateResolution();
        break;

    case TMSG_MOVETOSCREEN:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_MOVETOSCREEN" );
        CServiceBroker::GetWinSystem()->MoveToScreen( static_cast<int>(pMsg->param1) );
        break;

    case TMSG_MINIMIZE:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_MINIMIZE" );
        CServiceBroker::GetWinSystem()->Minimize();
        break;

    case TMSG_EXECUTE_OS:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_EXECUTE_OS" );
        // Suspend AE temporarily so exclusive or hog-mode sinks
        // don't block external player's access to audio device
        IAE* audioengine;
        audioengine = CServiceBroker::GetActiveAE();
        if( audioengine )
        {
            if( !audioengine->Suspend() )
            {
                CLog::Log( LOGINFO, "%s: Failed to suspend AudioEngine before launching external program",
                           __FUNCTION__ );
            }
        }
#if defined(TARGET_DARWIN)
        CLog::Log( LOGINFO, "ExecWait is not implemented on this platform" );
#elif defined(TARGET_POSIX)
        CUtil::RunCommandLine( pMsg->strParam, (pMsg->param1 == 1) );
#elif defined(TARGET_WINDOWS)
        CWIN32Util::XBMCShellExecute( pMsg->strParam.c_str(), (pMsg->param1 == 1) );
#endif
        // Resume AE processing of XBMC native audio
        if( audioengine )
        {
            if( !audioengine->Resume() )
            {
                CLog::Log( LOGFATAL, "%s: Failed to restart AudioEngine after return from external player",
                           __FUNCTION__ );
            }
        }
        break;

    case TMSG_EXECUTE_SCRIPT:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_EXECUTE_SCRIPT" );
        break;

    case TMSG_EXECUTE_BUILT_IN:
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_EXECUTE_BUILT_IN" );
        break;

    case TMSG_PICTURE_SHOW:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_PICTURE_SHOW" );

        // stop playing file
        if( GetComponent<CApplicationPlayer>()->IsPlayingVideo() )
            StopPlaying();

    }
    break;

    case TMSG_PICTURE_SLIDESHOW:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_PICTURE_SLIDESHOW" );
    }
    break;

    case TMSG_LOADPROFILE:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_LOADPROFILE" );
        //const int profile = pMsg->param1;
        //if( profile >= 0 )
        //    CServiceBroker::GetSettingsComponent()->GetProfileManager()->LoadProfile( static_cast<unsigned int>(profile) );
    }

    break;

    case TMSG_EVENT:
    {
        if( pMsg->lpVoid )
        {
            LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_EVENT" );
            XBMC_Event* event = static_cast<XBMC_Event*>(pMsg->lpVoid);
            OnEvent( *event );
            delete event;
        }
    }
    break;

    case TMSG_UPDATE_PLAYER_ITEM:
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CApplication::OnApplicationMessage TMSG_UPDATE_PLAYER_ITEM" );
        std::unique_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
        if( item )
        {
            m_itemCurrentFile->UpdateInfo( *item );
            //CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem( *m_itemCurrentFile );
        }
    }
    break;

    case TMSG_RENDERER_UNINIT:
        CLog::Log( LOGERROR, "%s: TMSG_RENDERER_UNINIT rxed", __func__ );
        break;

    default:
        CLog::Log( LOGERROR, "%s: Unhandled threadmessage sent, 0x%X", __FUNCTION__, msg );
        break;
    }
}

void CApplication::LockFrameMoveGuard()
{
    ++m_WaitingExternalCalls;
    m_frameMoveGuard.lock();
    ++m_ProcessedExternalCalls;
    CServiceBroker::GetWinSystem()->GetGfxContext().lock();
};

void CApplication::UnlockFrameMoveGuard()
{
    --m_WaitingExternalCalls;
    CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
    m_frameMoveGuard.unlock();
};

void CApplication::FrameMove( bool processEvents, bool processGUI )
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    bool renderGUI = true; // GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
    if( processEvents )
    {
        // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
        float frameTime = m_frameTime.GetElapsedSeconds();
        m_frameTime.StartZero();
        // never set a frametime less than 2 fps to avoid problems when debugging and on breaks
        if( frameTime > 0.5f )
            frameTime = 0.5f;

        if( processGUI && renderGUI )
        {
            //std::unique_lock<CCriticalSection> lock( CServiceBroker::GetWinSystem()->GetGfxContext() );
            //// check if there are notifications to display
            //CGUIDialogKaiToast* toast = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKaiToast>( WINDOW_DIALOG_KAI_TOAST );
            //if( toast && toast->DoWork() )
            //{
            //    if( !toast->IsDialogRunning() )
            //    {
            //        toast->Open();
            //    }
            //}
        }

        HandlePortEvents();


        // Open the door for external calls e.g python exactly here.
        // Window size can be between 2 and 10ms and depends on number of continuous requests
        if( m_WaitingExternalCalls )
        {
            CSingleExit ex( CServiceBroker::GetWinSystem()->GetGfxContext() );
            m_frameMoveGuard.unlock();

            // Calculate a window size between 2 and 10ms, 4 continuous requests let the window grow by 1ms
            // When not playing video we allow it to increase to 80ms
            unsigned int max_sleep = 10;
            if( !appPlayer->IsPlayingVideo() || appPlayer->IsPausedPlayback() )
                max_sleep = 80;
            unsigned int sleepTime = std::max( static_cast<unsigned int>(2), std::min( m_ProcessedExternalCalls >> 2, max_sleep ) );
            KODI::TIME::Sleep( std::chrono::milliseconds( sleepTime ) );
            m_frameMoveGuard.lock();
            m_ProcessedExternalDecay = 5;
        }
        if( m_ProcessedExternalDecay && --m_ProcessedExternalDecay == 0 )
            m_ProcessedExternalCalls = 0;
    }

    if( processGUI && renderGUI )
    {
        m_skipGuiRender = false;

        /*! @todo look into the possibility to use this for GBM
        int fps = 0;

        // This code reduces rendering fps of the GUI layer when playing videos in fullscreen mode
        // it makes only sense on architectures with multiple layers
        if (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() && !m_appPlayer.IsPausedPlayback() && m_appPlayer.IsRenderingVideoLayer())
          fps = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE);

        auto now = std::chrono::steady_clock::now();

        auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastRenderTime).count();
        if (fps > 0 && frameTime * fps < 1000)
          m_skipGuiRender = true;
        */

        //if( CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiSmartRedraw && m_guiRefreshTimer.IsTimePast() )
        //{
        //    CServiceBroker::GetGUI()->GetWindowManager().SendMessage( GUI_MSG_REFRESH_TIMER, 0, 0 );
        //    m_guiRefreshTimer.Set( 500ms );
        //}

        //if( !m_bStop )
        //{
        //    if( !m_skipGuiRender )
        //        CServiceBroker::GetGUI()->GetWindowManager().Process( CTimeUtils::GetFrameTime() );
        //}
        //CServiceBroker::GetGUI()->GetWindowManager().FrameMove();
    }

    appPlayer->FrameMove();

    // this will go away when render systems gets its own thread
    CServiceBroker::GetWinSystem()->DriveRenderLoop();
}


void CApplication::ResetCurrentItem()
{
    m_itemCurrentFile->Reset();
    //if( m_pGUI )
    //    m_pGUI->GetInfoManager().ResetCurrentItem();
}

int CApplication::Run()
{
    CLog::Log( LOGINFO, "Running the application..." );

    std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
    std::chrono::milliseconds frameTime;
    const unsigned int noRenderFrameTime = 15; // Simulates ~66fps
    onPlayerRunning( true );
    // Run the app
    while( !m_bStop && !VxIsAppShuttingDown()  )
    {
        // Animate and render a frame

        lastFrameTime = std::chrono::steady_clock::now();
        Process();

        bool renderGUI = true; // GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
        if( !m_bStop )
        {
            FrameMove( true, renderGUI );
        }

        if( renderGUI && !m_bStop )
        {
            if( getIsPlayingVideo() )
            {
                Render();
            }
            else
            {
                KODI::TIME::Sleep( std::chrono::milliseconds( 20 ) );
            }           
        }
        else if( !renderGUI )
        {
            //auto now = std::chrono::steady_clock::now();
            //frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);
            //if( frameTime.count() < noRenderFrameTime )
            //    KODI::TIME::Sleep( std::chrono::milliseconds( noRenderFrameTime - frameTime.count() ) );
        }
    }

    onPlayerRunning( false );

    Cleanup();

    CLog::Log( LOGINFO, "Exiting the application..." );
    return m_ExitCode;
}

bool CApplication::Cleanup()
{
    try
    {
        ResetCurrentItem();
        StopPlaying();

        if( m_ServiceManager )
            m_ServiceManager->DeinitStageThree();


        CServiceBroker::UnregisterTextureCache();


        CRenderSystemBase* renderSystem = CServiceBroker::GetRenderSystem();
        if( renderSystem )
            renderSystem->DestroyRenderSystem();

        CWinSystemBase* winSystem = CServiceBroker::GetWinSystem();
        if( winSystem )
            winSystem->DestroyWindow();

        //if( m_pGUI )
        //    m_pGUI->GetWindowManager().DestroyWindows();

        CLog::Log( LOGINFO, "unload sections" );

        //  Shutdown as much as possible of the
        //  application, to reduce the leaks dumped
        //  to the vc output window before calling
        //  _CrtDumpMemoryLeaks(). Most of the leaks
        //  shown are no real leaks, as parts of the app
        //  are still allocated.

        g_localizeStrings.Clear();
        g_LangCodeExpander.Clear();
        g_charsetConverter.clear();
        g_directoryCache.Clear();

        //CServiceBroker::GetPlaylistPlayer().Clear();

        if( m_ServiceManager )
            m_ServiceManager->DeinitStageTwo();

#ifdef TARGET_POSIX
        CXHandle::DumpObjectTracker();
#endif
#ifdef _CRTDBG_MAP_ALLOC
        _CrtDumpMemoryLeaks();
        while( 1 ); // execution ends
#endif

        if( m_pGUI )
        {
            m_pGUI->Deinit();
            m_pGUI.reset();
        }

        if( winSystem )
        {
            winSystem->DestroyWindowSystem();
            CServiceBroker::UnregisterWinSystem();
            winSystem = nullptr;
            m_pWinSystem.reset();
        }

        // Cleanup was called more than once on exit during my tests
        if( m_ServiceManager )
        {
            m_ServiceManager->DeinitStageOne();
            m_ServiceManager.reset();
        }


        CServiceBroker::UnregisterAppMessenger();

        CServiceBroker::UnregisterAnnouncementManager();
        m_pAnnouncementManager->Deinitialize();
        m_pAnnouncementManager.reset();

        CServiceBroker::UnregisterJobManager();
        CServiceBroker::UnregisterCPUInfo();

        //UnregisterSettings();

        m_bInitializing = true;

        return true;
    }
    catch( ... )
    {
        CLog::Log( LOGERROR, "Exception in CApplication::Cleanup()" );
        return false;
    }
}

bool CApplication::Stop( int exitCode )
{
    CLog::Log( LOGINFO, "Stopping the application..." );

    bool success = true;

    CLog::Log( LOGINFO, "Stopping player" );
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    appPlayer->ClosePlayer();

    {
        // close inbound port
        CServiceBroker::UnregisterAppPort();
        XbmcThreads::EndTime<> timer( 1000ms );
        while( m_pAppPort.use_count() > 1 )
        {
            KODI::TIME::Sleep( 100ms );
            if( timer.IsTimePast() )
            {
                CLog::Log( LOGERROR, "CApplication::Stop - CAppPort still in use, app may crash" );
                break;
            }
        }
        m_pAppPort.reset();
    }

    try
    {
        m_frameMoveGuard.unlock();

        CVariant vExitCode( CVariant::VariantTypeObject );
        vExitCode["exitcode"] = exitCode;
        CServiceBroker::GetAnnouncementManager()->Announce( ANNOUNCEMENT::System, "OnQuit", vExitCode );



        g_alarmClock.StopThread();



        m_bStop = true;
        // Add this here to keep the same ordering behaviour for now
        // Needs cleaning up
        CServiceBroker::GetAppMessenger()->Stop();
        m_AppFocused = false;
        m_ExitCode = exitCode;
        CLog::Log( LOGINFO, "Stopping all" );

        // cancel any jobs from the jobmanager
        CServiceBroker::GetJobManager()->CancelJobs();


        //if( CVideoLibraryQueue::GetInstance().IsRunning() )
        //    CVideoLibraryQueue::GetInstance().CancelAllJobs();

        if( CServiceBroker::GetAppMessenger() )
        {
            CServiceBroker::GetAppMessenger()->Cleanup();
        }


#if defined(TARGET_DARWIN_OSX)
        if( XBMCHelper::GetInstance().IsAlwaysOn() == false )
            XBMCHelper::GetInstance().Stop();
#endif


        // unregister action listeners
        const auto appListener = GetComponent<CApplicationActionListeners>();
        appListener->UnregisterActionListener( &GetComponent<CApplicationPlayer>()->GetSeekHandler() );
        //appListener->UnregisterActionListener( &CPlayerController::GetInstance() );

        //CGUIComponent* gui = CServiceBroker::GetGUI();
        //if( gui )
        //    gui->GetAudioManager().DeInitialize();

        // shutdown the AudioEngine
        CServiceBroker::UnregisterAE();
        m_pActiveAE->Shutdown();
        m_pActiveAE.reset();

        CLog::Log( LOGINFO, "Application stopped" );
    }
    catch( ... )
    {
        CLog::Log( LOGERROR, "Exception in CApplication::Stop()" );
        success = false;
    }

    //cleanup_emu_environ();

    KODI::TIME::Sleep( 200ms );

    return success;
}

namespace
{
    class CCreateAndLoadPlayList : public IRunnable
    {
    public:
        CCreateAndLoadPlayList( CFileItem& item, std::unique_ptr<PLAYLIST::CPlayList>& playlist )
            : m_item( item ), m_playlist( playlist )
        {
        }

        void Run() override
        {
            const std::unique_ptr<PLAYLIST::CPlayList> playlist( PLAYLIST::CPlayListFactory::Create( m_item ) );
            if( playlist )
            {
                if( playlist->Load( m_item.GetPath() ) )
                    *m_playlist = *playlist;
            }
        }

    private:
        CFileItem& m_item;
        std::unique_ptr<PLAYLIST::CPlayList>& m_playlist;
    };
} // namespace

bool CApplication::PlayMedia( CFileItem& item, const std::string& player, PLAYLIST::Id playlistId )
{
    //nothing special just play
    return PlayFile( item, player, false );
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
// return value: same with PlayFile()
bool CApplication::PlayStack( CFileItem& item, bool bRestart )
{
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();
    //if( !stackHelper->InitializeStack( item ) )
    //    return false;

    //int startoffset = stackHelper->InitializeStackStartPartAndOffset( item );

    //CFileItem selectedStackPart = stackHelper->GetCurrentStackPartFileItem();
    //selectedStackPart.SetStartOffset( startoffset );

    //if( item.HasProperty( "savedplayerstate" ) )
    //{
    //    selectedStackPart.SetProperty( "savedplayerstate", item.GetProperty( "savedplayerstate" ) ); // pass on to part
    //    item.ClearProperty( "savedplayerstate" );
    //}

    //return PlayFile( selectedStackPart, "", true );
    return false;
}

bool CApplication::PlayFile( CFileItem item, const std::string& player, bool bRestart )
{
    // Ensure the MIME type has been retrieved for http:// and shout:// streams
    if( item.GetMimeType().empty() )
        item.FillInMimeType();

    const auto appPlayer = GetComponent<CApplicationPlayer>();

    if( !bRestart )
    {
        // bRestart will be true when called from PlayStack(), skipping this block
        appPlayer->SetPlaySpeed( 1 );

        m_nextPlaylistItem = -1;
        //stackHelper->Clear();

        if( item.IsVideo() )
            CUtil::ClearSubtitles();
    }

    if( item.IsPlayList() )
        return false;

    // if we have a stacked set of files, we need to setup our stack routines for
    // "seamless" seeking and total time of the movie etc.
    // will recall with restart set to true
    if( item.IsStack() )
        return PlayStack( item, bRestart );

    CPlayerOptions options;

    if( item.HasProperty( "StartPercent" ) )
    {
        options.startpercent = item.GetProperty( "StartPercent" ).asDouble();
        item.SetStartOffset( 0 );
    }

    options.starttime = CUtil::ConvertMilliSecsToSecs( item.GetStartOffset() );

    if( bRestart )
    {
        // have to be set here due to playstack using this for starting the file
        //if( item.HasVideoInfoTag() )
        //    options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
    }
 
    options.fullscreen = true;
    bool openFileResult = appPlayer->OpenFile( item, options, m_ServiceManager->GetPlayerCoreFactory(), player, *this );
    if( !openFileResult )
    {
        if( item.isVirtualStream() )
        {
            LogModule( eLogStreams, LOG_VERBOSE, "CApplication::%s open stream %s failed", __func__, item.getFileName().c_str() );
        }
        else
        {
            LogModule( eLogPlayerNlc, LOG_VERBOSE, "CApplication::%s open local file %s failed", __func__, item.getFileName().c_str() );
        }
    }

    onPlayFile( openFileResult );
    return openFileResult;
}

void CApplication::PlaybackCleanup()
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();

    if( !appPlayer->IsPlaying() )
    {
        CGUIComponent* gui = CServiceBroker::GetGUI();
        appPlayer->OpenNext( m_ServiceManager->GetPlayerCoreFactory() );
    }

    if( !appPlayer->IsPlaying() )
    {
        appPlayer->ResetPlayer();
    }
}

bool CApplication::IsPlayingFullScreenVideo() const
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    return appPlayer->IsPlayingVideo() &&
        CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo();
}

bool CApplication::IsFullScreen()
{
    return false;
}

void CApplication::StopPlaying()
{
    if( GetComponent<CApplicationPlayer>() && GetComponent<CApplicationPlayer>()->IsPlaying() )
    {

        GetComponent<CApplicationPlayer>()->ClosePlayer();

        onStopPlaying();
    }
}

bool CApplication::OnMessage( CGUIMessage& message )
{
    switch( message.GetGuiMessage() )
    {
    case GUI_MSG_NOTIFY_ALL:
    {
        if( message.GetParam1() == GUI_MSG_REMOVED_MEDIA )
        {
            // Update general playlist: Remove DVD playlist items
            int nRemoved = CServiceBroker::GetPlaylistPlayer().RemoveDVDItems();
            if( nRemoved > 0 )
            {
                //CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
                //CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
            }
            // stop the file if it's on dvd (will set the resume point etc)
            if( m_itemCurrentFile->IsOnDVD() )
                StopPlaying();
        }
        else if( message.GetParam1() == GUI_MSG_UI_READY )
        {
            // remove splash window
           // CServiceBroker::GetGUI()->GetWindowManager().Delete( WINDOW_SPLASH );



            m_bInitializing = false;

        }
        else if( message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem() )
        {
            CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
            if( m_itemCurrentFile->IsSamePath( item.get() ) )
            {
                m_itemCurrentFile->UpdateInfo( *item );
                //CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem( *item );
            }
        }
    }
    break;

    case GUI_MSG_PLAYBACK_STARTED:
    {
#ifdef TARGET_DARWIN_EMBEDDED
        // @TODO move this away to platform code
        CDarwinUtils::SetScheduling( GetComponent<CApplicationPlayer>()->IsPlayingVideo() );
#endif
        PLAYLIST::CPlayList playList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
            CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() );

        // Update our infoManager with the new details etc.
        if( m_nextPlaylistItem >= 0 )
        {
            // playing an item which is not in the list - player might be stopped already
            // so do nothing
            if( playList.size() <= m_nextPlaylistItem )
                return true;

            // we've started a previously queued item
            CFileItemPtr item = playList[m_nextPlaylistItem];
            // update the playlist manager
            int currentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
            int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
            //CGUIMessage msg( GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist(), param, item );
            //CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage( msg );
            CServiceBroker::GetPlaylistPlayer().SetCurrentSong( m_nextPlaylistItem );
            m_itemCurrentFile.reset( new CFileItem( *item ) );
        }
        //CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem( *m_itemCurrentFile );
        //g_partyModeManager.OnSongChange( true );


        CVariant param;
        param["player"]["speed"] = 1;
        param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();

        CServiceBroker::GetAnnouncementManager()->Announce( ANNOUNCEMENT::Player, "OnPlay",
                                                            m_itemCurrentFile, param );

        onPlayStarted();
        return true;
    }
    break;

    case GUI_MSG_QUEUE_NEXT_ITEM:
    {
        // Check to see if our playlist player has a new item for us,
        // and if so, we check whether our current player wants the file
        int iNext = CServiceBroker::GetPlaylistPlayer().GetNextSong();
        PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
            CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() );
        if( iNext < 0 || iNext >= playlist.size() )
        {
            GetComponent<CApplicationPlayer>()->OnNothingToQueueNotify();
            return true; // nothing to do
        }

        // ok, grab the next song
        CFileItem file( *playlist[iNext] );


        // Don't queue if next media type is different from current one
        bool bNothingToQueue = false;

        const auto appPlayer = GetComponent<CApplicationPlayer>();
        if( !file.IsVideo() && appPlayer->IsPlayingVideo() )
            bNothingToQueue = true;
        else if( (!file.IsAudio() || file.IsVideo()) && appPlayer->IsPlayingAudio() )
            bNothingToQueue = true;

        if( bNothingToQueue )
        {
            appPlayer->OnNothingToQueueNotify();
            return true;
        }



        // ok - send the file to the player, if it accepts it
        if( appPlayer->QueueNextFile( file ) )
        {
            // player accepted the next file
            m_nextPlaylistItem = iNext;
        }
        else
        {
            /* Player didn't accept next file: *ALWAYS* advance playlist in this case so the player can
                queue the next (if it wants to) and it doesn't keep looping on this song */
            CServiceBroker::GetPlaylistPlayer().SetCurrentSong( iNext );
        }

        return true;
    }
    break;


    case GUI_MSG_PLAYBACK_STOPPED:
        m_playerEvent.Set();
        ResetCurrentItem();
        PlaybackCleanup();

        onPlaybackStopped();
        return true;

    case GUI_MSG_PLAYBACK_ENDED:
    {
        m_playerEvent.Set();
        //const auto stackHelper = GetComponent<CApplicationStackHelper>();
        //if( stackHelper->IsPlayingRegularStack() && stackHelper->HasNextStackPartFileItem() )
        //{ // just play the next item in the stack
        //    PlayFile( stackHelper->SetNextStackPartCurrentFileItem(), "", true );
        //    return true;
        //}
        ResetCurrentItem();
        if( !CServiceBroker::GetPlaylistPlayer().PlayNext( 1, true ) )
            GetComponent<CApplicationPlayer>()->ClosePlayer();

        PlaybackCleanup();

#ifdef HAS_PYTHON
        CServiceBroker::GetXBPython().OnPlayBackEnded();
#endif
        onPlaybackEnded();
        return true;
    }

    case GUI_MSG_PLAYLISTPLAYER_STOPPED:
        ResetCurrentItem();
        if( GetComponent<CApplicationPlayer>()->IsPlaying() )
            StopPlaying();
        PlaybackCleanup();
        return true;

    case GUI_MSG_PLAYBACK_AVSTARTED:
        m_playerEvent.Set();

        return true;

    case GUI_MSG_PLAYBACK_AVCHANGE:

        return true;

    case GUI_MSG_PLAYBACK_ERROR:
        //HELPERS::ShowOKDialogText( CVariant{ 16026 }, CVariant{ 16027 } );
        return true;

    case GUI_MSG_PLAYLISTPLAYER_STARTED:
    case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
        return true;
    }
    break;
    case GUI_MSG_FULLSCREEN:
    { // Switch to fullscreen, if we can
        //CGUIComponent* gui = CServiceBroker::GetGUI();
        //if( gui )
        //    gui->GetWindowManager().SwitchToFullScreen();

        return true;
    }
    break;
    case GUI_MSG_EXECUTE:
        //if( message.GetNumStringParams() )
        //    return ExecuteXBMCAction( message.GetStringParam(), message.GetItem() );
        break;
    }
    return false;
}

//bool CApplication::ExecuteXBMCAction( std::string actionStr, const CGUIListItemPtr& item /* = NULL */ )
//{
//    // see if it is a user set string
//
//    //We don't know if there is unsecure information in this yet, so we
//    //postpone any logging
//    const std::string in_actionStr( actionStr );
//    if( item )
//        actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetItemLabel( actionStr, item.get() );
//    else
//        actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetLabel( actionStr, INFO::DEFAULT_CONTEXT );
//
//    // user has asked for something to be executed
//    if( CBuiltins::GetInstance().HasCommand( actionStr ) )
//    {
//        if( !CBuiltins::GetInstance().IsSystemPowerdownCommand( actionStr )
//#if HAVE_ADDONS
//            || CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown() )
//#else
//            )
//#endif // HAVE_ADDONS
//        CBuiltins::GetInstance().Execute( actionStr );
//    }
//    else
//    {
//        // try translating the action from our ButtonTranslator
//        unsigned int actionID;
//        if( CActionTranslator::TranslateString( actionStr, actionID ) )
//        {
//            OnAction( CAction( actionID ) );
//            return true;
//        }
//        CFileItem item( actionStr, false );
//#ifdef HAS_PYTHON
//        if( item.IsPythonScript() )
//        { // a python script
//            CScriptInvocationManager::GetInstance().ExecuteAsync( item.GetPath() );
//        }
//        else
//#endif
//            if( item.IsAudio() || item.IsVideo() || item.IsGame() )
//            { // an audio or video file
//                PlayFile( item, "" );
//            }
//            else
//            {
//                //At this point we have given up to translate, so even though
//                //there may be insecure information, we log it.
//                CLog::LogF( LOGDEBUG, "Tried translating, but failed to understand %s", in_actionStr.c_str() );
//                return false;
//            }
//    }
//    return true;
//}

// inform the user that the configuration data has moved from old XBMC location
// to new Kodi location - if applicable
void CApplication::ShowAppMigrationMessage()
{
    // .kodi_migration_complete will be created from the installer/packaging
    // once an old XBMC configuration was moved to the new Kodi location
    // if this is the case show the migration info to the user once which
    // tells him to have a look into the wiki where the move of configuration
    // is further explained.
    //if( CFile::Exists( "special://home/.kodi_data_was_migrated" ) &&
    //    !CFile::Exists( "special://home/.kodi_migration_info_shown" ) )
    //{
    //    HELPERS::ShowOKDialogText( CVariant{ 24128 }, CVariant{ 24129 } );
    //    CFile tmpFile;
    //    // create the file which will prevent this dialog from appearing in the future
    //    tmpFile.OpenForWrite( "special://home/.kodi_migration_info_shown" );
    //    tmpFile.Close();
    //}
}

void CApplication::ConfigureAndEnableAddons()
{

}

void CApplication::Process()
{
    // dispatch the messages generated by python or other threads to the current window
    //CServiceBroker::GetGUI()->GetWindowManager().DispatchThreadMessages();

    // process messages which have to be send to the gui
    // (this can only be done after CServiceBroker::GetGUI()->GetWindowManager().Render())
    CServiceBroker::GetAppMessenger()->ProcessWindowMessages();

    // process messages, even if a movie is playing
    CServiceBroker::GetAppMessenger()->ProcessMessages();
    if( m_bStop ) return; //we're done, everything has been unloaded

    // update sound
    GetComponent<CApplicationPlayer>()->DoAudioWork();

    // do any processing that isn't needed on each run
    if( m_slowTimer.GetElapsedMilliseconds() > 500 )
    {
        m_slowTimer.Reset();
        ProcessSlow();
    }
}

// We get called every 500ms
void CApplication::ProcessSlow()
{


#if defined(TARGET_DARWIN_OSX) && defined(SDL_FOUND)
    // There is an issue on OS X that several system services ask the cursor to become visible
    // during their startup routines.  Given that we can't control this, we hack it in by
    // forcing the
    if( CServiceBroker::GetWinSystem()->IsFullScreen() )
    { // SDL thinks it's hidden
        Cocoa_HideMouse();
    }
#endif

    // Temporarily pause pausable jobs when viewing video/picture
    //int currentWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
    //if( CurrentFileItem().IsVideo() ||
    //    CurrentFileItem().IsPicture() ||
    //    currentWindow == WINDOW_FULLSCREEN_VIDEO ||
    //    currentWindow == WINDOW_FULLSCREEN_GAME ||
    //    currentWindow == WINDOW_SLIDESHOW )
    //{
    //    CServiceBroker::GetJobManager()->PauseJobs();
    //}
    //else
    //{
    //    CServiceBroker::GetJobManager()->UnPauseJobs();
    //}


    if( testQuitFlag() )
    {
        CLog::Log( LOGINFO, "Quitting due to POSIX signal" );
        CServiceBroker::GetAppMessenger()->PostMsg( TMSG_QUIT );
    }

    // check if we should restart the player
    CheckDelayedPlayerRestart();

    //  check if we can unload any unreferenced dlls or sections
    //const auto appPlayer = GetComponent<CApplicationPlayer>();
    //if( !appPlayer->IsPlayingVideo() )
    //    CSectionLoader::UnloadDelayed();

#if defined(TARGET_ANDROID)
    // Pass the slow loop to droid
//    CXBMCApp::Get().ProcessSlow();
#endif


    CServiceBroker::GetGUI()->GetLargeTextureManager().CleanupUnusedImages();

    CServiceBroker::GetGUI()->GetTextureManager().FreeUnusedTextures( 5000 );


#if defined(TARGET_POSIX) && defined(HAS_FILESYSTEM_SMB)
    smb.CheckIfIdle();
#endif

    CServiceBroker::GetMediaManager().ProcessEvents();
}

void CApplication::DelayedPlayerRestart()
{
    m_restartPlayerTimer.StartZero();
}

void CApplication::CheckDelayedPlayerRestart()
{
    if( m_restartPlayerTimer.GetElapsedSeconds() > 3 )
    {
        m_restartPlayerTimer.Stop();
        m_restartPlayerTimer.Reset();
        Restart( true );
    }
}

void CApplication::Restart( bool bSamePosition )
{
    // this function gets called when the user changes a setting (like noninterleaved)
    // and which means we gotta close & reopen the current playing file

    // first check if we're playing a file
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    if( !appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingAudio() )
        return;

    if( !appPlayer->HasPlayer() )
        return;

    // do we want to return to the current position in the file
    if( !bSamePosition )
    {
        // no, then just reopen the file and start at the beginning
        PlayFile( *m_itemCurrentFile, "", true );
        return;
    }

    // else get current position
    double time = GetTime();

    // get player state, needed for dvd's
    std::string state = appPlayer->GetPlayerState();

    // set the requested starttime
    m_itemCurrentFile->SetStartOffset( CUtil::ConvertSecsToMilliSecs( time ) );

    // reopen the file
    if( PlayFile( *m_itemCurrentFile, "", true ) )
        appPlayer->SetPlayerState( state );
}

const std::string& CApplication::CurrentFile()
{
    return m_itemCurrentFile->GetPath();
}

std::shared_ptr<CFileItem> CApplication::CurrentFileItemPtr()
{
    return m_itemCurrentFile;
}

CFileItem& CApplication::CurrentFileItem()
{
    return *m_itemCurrentFile;
}

const CFileItem& CApplication::CurrentUnstackedItem()
{
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    //if( stackHelper->IsPlayingISOStack() || stackHelper->IsPlayingRegularStack() )
    //    return stackHelper->GetCurrentStackPartFileItem();
    //else
        return *m_itemCurrentFile;
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
    double rc = 0.0;

    const auto appPlayer = GetComponent<CApplicationPlayer>();
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    if( appPlayer->IsPlaying() )
    {
        //if( stackHelper->IsPlayingRegularStack() )
        //    rc = stackHelper->GetStackTotalTimeMs() * 0.001;
        //else
            rc = appPlayer->GetTotalTime() * 0.001;
    }

    return rc;
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
    double rc = 0.0;

    const auto appPlayer = GetComponent<CApplicationPlayer>();
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    if( appPlayer->IsPlaying() )
    {
        //if( stackHelper->IsPlayingRegularStack() )
        //{
        //    uint64_t startOfCurrentFile = stackHelper->GetCurrentStackPartStartTimeMs();
        //    rc = (startOfCurrentFile + appPlayer->GetTime()) * 0.001;
        //}
        //else
            rc = appPlayer->GetTime() * 0.001;
    }

    return rc;
}

// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
void CApplication::SeekTime( double dTime )
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    if( appPlayer->IsPlaying() && (dTime >= 0.0) )
    {
        if( !appPlayer->CanSeek() )
            return;

        // convert to milliseconds and perform seek
        appPlayer->SeekTime( static_cast<int64_t>(dTime * 1000.0) );
    }
}

float CApplication::GetPercentage() const
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    if( appPlayer->IsPlaying() )
    {
        if( appPlayer->GetTotalTime() == 0 && appPlayer->IsPlayingAudio() &&
            m_itemCurrentFile->HasMusicInfoTag() )
        {
            const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
            if( tag.GetDuration() > 0 )
                return (float)(GetTime() / tag.GetDuration() * 100);
        }


            return appPlayer->GetPercentage();
    }
    return 0.0f;
}

float CApplication::GetCachePercentage() const
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    if( appPlayer->IsPlaying() )
    {
        // Note that the player returns a relative cache percentage and we want an absolute percentage
              return std::min( 100.0f, appPlayer->GetPercentage() + appPlayer->GetCachePercentage() );
    }
    return 0.0f;
}

void CApplication::SeekPercentage( float percent )
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    //const auto stackHelper = GetComponent<CApplicationStackHelper>();

    if( appPlayer->IsPlaying() && (percent >= 0.0f) )
    {
        if( !appPlayer->CanSeek() )
            return;
             appPlayer->SeekPercentage( percent );
    }
}

std::string CApplication::GetCurrentPlayer()
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();
    return appPlayer->GetCurrentPlayer();
}


void CApplication::UpdateCurrentPlayArt()
{
    //const auto appPlayer = GetComponent<CApplicationPlayer>();
    //if( !appPlayer->IsPlayingAudio() )
    //    return;
    ////Clear and reload the art for the currently playing item to show updated art on OSD
    //m_itemCurrentFile->ClearArt();
    //CMusicThumbLoader loader;
    //loader.LoadItem( m_itemCurrentFile.get() );
    //// Mirror changes to GUI item
    ////CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem( *m_itemCurrentFile );
}

bool CApplication::ProcessAndStartPlaylist( const std::string& strPlayList,
                                            PLAYLIST::CPlayList& playlist,
                                            PLAYLIST::Id playlistId,
                                            int track )
{
    CLog::Log( LOGDEBUG, "CApplication::ProcessAndStartPlaylist(%s, %d)", strPlayList.c_str(), playlistId );

    // initial exit conditions
    // no songs in playlist just return
    if( playlist.size() == 0 )
        return false;

    // illegal playlist
    if( playlistId == PLAYLIST::TYPE_NONE || playlistId == PLAYLIST::TYPE_PICTURE )
        return false;

    // setup correct playlist
    CServiceBroker::GetPlaylistPlayer().ClearPlaylist( playlistId );

    // if the playlist contains an internet stream, this file will be used
    // to generate a thumbnail for musicplayer.cover
    m_strPlayListFile = strPlayList;

    // add the items to the playlist player
    CServiceBroker::GetPlaylistPlayer().Add( playlistId, playlist );

    // if we have a playlist
    if( CServiceBroker::GetPlaylistPlayer().GetPlaylist( playlistId ).size() )
    {
        // start playing it
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist( playlistId );
        CServiceBroker::GetPlaylistPlayer().Reset();
        CServiceBroker::GetPlaylistPlayer().Play( track, "" );
        return true;
    }
    return false;
}

bool CApplication::GetRenderGUI() const
{
    //return GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
    return m_RenderGui;
}

bool CApplication::SetLanguage( const std::string& strLanguage )
{
    // nothing to be done if the language hasn't changed
    //if( strLanguage == CServiceBroker::GetSettingsComponent()->GetSettings()->GetString( CSettings::SETTING_LOCALE_LANGUAGE ) )
    //    return true;

    //return CServiceBroker::GetSettingsComponent()->GetSettings()->SetString( CSettings::SETTING_LOCALE_LANGUAGE, strLanguage );
    return true;
}

bool CApplication::LoadLanguage( bool reload )
{


    // set the proper audio and subtitle languages
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

    g_langInfo.SetSubtitleLanguage( settings->GetString( CSettings::SETTING_LOCALE_SUBTITLELANGUAGE ) );

    return true;
}

void CApplication::SetLoggingIn( bool switchingProfiles )
{
    //// don't save skin settings on unloading when logging into another profile
    //// because in that case we have already loaded the new profile and
    //// would therefore write the previous skin's settings into the new profile
    //// instead of into the previous one
    //GetComponent<CApplicationSkinHandling>()->m_saveSkinOnUnloading = !switchingProfiles;
}

void CApplication::PrintStartupLog()
{
    CLog::Log( LOGINFO, "-----------------------------------------------------------------------" );
    CLog::Log( LOGINFO, "Starting %s (%s). Platform: %s %s %d-bit", CSysInfo::GetAppName().c_str(),
               CSysInfo::GetVersion().c_str(), g_sysinfo.GetBuildTargetPlatformName().c_str(),
               g_sysinfo.GetBuildTargetCpuFamily().c_str(), g_sysinfo.GetXbmcBitness() );

    std::string buildType;
#if defined(_DEBUG)
    buildType = "Debug";
#elif defined(NDEBUG)
    buildType = "Release";
#else
    buildType = "Unknown";
#endif

    CLog::Log( LOGINFO, "Using %s %s x%d", buildType.c_str(), CSysInfo::GetAppName().c_str(),
               g_sysinfo.GetXbmcBitness() );
    CLog::Log( LOGINFO, "%s compiled %s by %s for %s %s %d-bit %s (%s)", CSysInfo::GetAppName().c_str(),
               CSysInfo::GetBuildDate().c_str(), g_sysinfo.GetUsedCompilerNameAndVer().c_str(),
               g_sysinfo.GetBuildTargetPlatformName().c_str(), g_sysinfo.GetBuildTargetCpuFamily().c_str(),
               g_sysinfo.GetXbmcBitness(), g_sysinfo.GetBuildTargetPlatformVersionDecoded().c_str(),
               g_sysinfo.GetBuildTargetPlatformVersion().c_str() );

    std::string deviceModel( g_sysinfo.GetModelName() );
    if( !g_sysinfo.GetManufacturerName().empty() )
        deviceModel = g_sysinfo.GetManufacturerName() + " " +
        (deviceModel.empty() ? std::string( "device" ) : deviceModel);
    if( !deviceModel.empty() )
        CLog::Log( LOGINFO, "Running on %s with %s, kernel: %s %s %d-bit version %s", deviceModel.c_str(),
                   g_sysinfo.GetOsPrettyNameWithVersion().c_str(), g_sysinfo.GetKernelName().c_str(),
                   g_sysinfo.GetKernelCpuFamily().c_str(), g_sysinfo.GetKernelBitness(),
                   g_sysinfo.GetKernelVersionFull().c_str() );
    else
        CLog::Log( LOGINFO, "Running on %s, kernel: %s %s %d-bit version %s",
                   g_sysinfo.GetOsPrettyNameWithVersion().c_str(), g_sysinfo.GetKernelName().c_str(),
                   g_sysinfo.GetKernelCpuFamily().c_str(), g_sysinfo.GetKernelBitness(),
                   g_sysinfo.GetKernelVersionFull().c_str() );

    CLog::Log( LOGINFO, "FFmpeg version/source: %s", av_version_info() );

    std::string cpuModel( CServiceBroker::GetCPUInfo()->GetCPUModel() );
    if( !cpuModel.empty() )
    {
        CLog::Log( LOGINFO, "Host CPU: %s, %d core%s available", cpuModel.c_str(),
                   CServiceBroker::GetCPUInfo()->GetCPUCount(),
                   (CServiceBroker::GetCPUInfo()->GetCPUCount() == 1) ? "" : "s" );
    }
    else
        CLog::Log( LOGINFO, "%d CPU core%s available", CServiceBroker::GetCPUInfo()->GetCPUCount(),
                   (CServiceBroker::GetCPUInfo()->GetCPUCount() == 1) ? "" : "s" );

    // Any system info logging that is unique to a platform
    //m_ServiceManager->GetPlatform().PlatformSyslog();

#if defined(__arm__) || defined(__aarch64__)
    CLog::Log( LOGINFO, "ARM Features: Neon %s",
               (CServiceBroker::GetCPUInfo()->GetCPUFeatures() & CPU_FEATURE_NEON) ? "enabled"
               : "disabled" );
#endif
    CSpecialProtocol::LogPaths();

#ifdef HAS_WEB_SERVER
    CLog::Log( LOGINFO, "Webserver extra whitelist paths: %s",
               StringUtils::Join( CCompileInfo::GetWebserverExtraWhitelist(), ", " ).c_str() );
#endif

    // Check, whether libkodi.so was reused (happens on Android, where the system does not unload
    // the lib on activity end, but keeps it loaded (as long as there is enough memory) and reuses
    // it on next activity start.
    static bool firstRun = true;

    CLog::Log( LOGINFO, "The executable running is: %s%s", CUtil::ResolveExecutablePath().c_str(),
               firstRun ? "" : " [reused]" );

    firstRun = false;

    std::string hostname( "[unknown]" );
    //m_ServiceManager->GetNetwork().GetHostName( hostname );
    CLog::Log( LOGINFO, "Local hostname: %s", hostname.c_str() );
    std::string mediaPlayerLogName{ "mediaplayernlc" };
    CLog::Log( LOGINFO, "Media Player Log File is located: %s.log",
               CSpecialProtocol::TranslatePath( "special://logpath/" + mediaPlayerLogName ).c_str() );
    CRegExp::LogCheckUtf8Support();
    CLog::Log( LOGINFO, "-----------------------------------------------------------------------" );
}

void CApplication::CloseNetworkShares()
{
    CLog::Log( LOGDEBUG, "CApplication::CloseNetworkShares: Closing all network shares" );
}

bool CApplication::SwitchToFullScreen( bool fullScreen )
{
    const auto appPlayer = GetComponent<CApplicationPlayer>();

    appPlayer->TriggerUpdateResolution();
    return true;
}

bool CApplication::IsCurrentThread() const
{
    return CThread::IsCurrentThread( m_threadID );
}

void CApplication::PlayPauseButtonClicked( void )
{
  const auto appPlayer = GetComponent<CApplicationPlayer>();
  if( appPlayer )
  {
      appPlayer->Pause();
  }
}
