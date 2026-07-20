
#include "MediaPlayerNlc.h"

#include "ApplicationPlayer.h"

#include "NlcUrl.h"
#include "ServiceBroker.h"
#include "ServiceManager.h"

#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"

#include "settings/DisplaySettings.h"
#include "settings/SettingsComponent.h"
#include "settings/Settings.h"
#include "settings/lib/SettingsManager.h"

#include "windowing/WinSystem.h"
#include "windowing/GraphicContext.h"

#include <GuiInterface/IToGui.h>
#include <GuiInterface/OsInterface/OsInterface.h>
#include <GuiInterface/IMediaPlayerCallback.h>
#include <GuiInterface/IMediaPlayerRequests.h>

#include <CoreLib/VxDebug.h>

#include <algorithm>


//============================================================================
MediaPlayerNlc& GetNlcPlayerInstance()
{
    static MediaPlayerNlc g_NlcPlayer;
    return g_NlcPlayer;
}


//============================================================================
MediaPlayerNlc::MediaPlayerNlc()
	: CApplication()
{
}

//============================================================================
IMediaPlayerRequests& MediaPlayerNlc::getIMediaPlayerRequests( void )
{
	return *this;
}

//============================================================================
void MediaPlayerNlc::wantMediaPlayerCallback( IMediaPlayerCallback* client, bool wantCallback )
{
    if( !client )
    {
		LogMsg( LOG_ERROR, "MediaPlayerNlc::wantConnectIdListCallback null client" );
		vx_assert( false );
        return;
    }

    lockClientList();
    bool foundClient{ false };
    for( auto iter = m_MediaPlayerCallbackClients.begin(); iter != m_MediaPlayerCallbackClients.end(); ++iter )
    {
        if( *iter == client )
        {
            foundClient = true;
            if( !wantCallback )
            {
                m_MediaPlayerCallbackClients.erase( iter );
            }

            break;
        }
    }

    if( !foundClient && wantCallback )
    {
        m_MediaPlayerCallbackClients.emplace_back( client );
    }

    unlockClientList();
}

//============================================================================
enum EMediaModule MediaPlayerNlc::getMediaModule( void )
{ 
	return EMediaModule::eMediaModulePlayerNlc; 
}

//============================================================================
bool MediaPlayerNlc::testQuitFlag()
{
    // BRJ TODO implement
    return false;
}

//============================================================================
/// NOTE: called from RenderPlayerNlcThread::run()
bool MediaPlayerNlc::fromThreadStartModule( EMediaModule mediaModule )
{
	if( eMediaModulePlayerNlc == mediaModule && !m_ModuleIsRunning )
	{
		if( !m_ModuleIsInitialized )
		{
			m_ModuleIsInitialized = true;
			getOsInterface().doStartup();
		}

		m_ModuleIsRunning = true;
		m_ModuleStopCalled = false;
		getOsInterface().doRun( mediaModule ); // will stay in this function until kodi is shutdown
		m_ModuleIsRunning = false;
	}

	return true;
}

//============================================================================
bool MediaPlayerNlc::fromGuiStopModule( EMediaModule mediaModule )
{
	if( eMediaModulePlayerNlc == mediaModule && m_ModuleIsRunning )
	{
        StopPlaying();
		if( !m_ModuleStopCalled )
		{
			m_ModuleStopCalled = true;
			m_ModuleIsInitialized = false;
			if( !m_bStop && CServiceBroker::GetAppMessenger() )
			{
				CServiceBroker::GetAppMessenger()->PostMsg( TMSG_QUIT );
			}           
		}
	}

	return m_ModuleIsRunning;
}

//============================================================================
bool MediaPlayerNlc::fromGuiIsModuleRunning( EMediaModule mediaModule )
{
	return m_ServiceManager && m_ServiceManager->GetInitLevel() >= 3;
}

//============================================================================
bool MediaPlayerNlc::assureInitialized( void )
{
	if( !m_ServiceManager.get() )
	{
		// has been shutdown.. restart
		LogModule( eLogPlayerNlc, LOG_ERROR, "media player could not be initialized" );
		return false;
	}

    if( !m_ServiceManager->HasPlayerFactory() )
    {
        LogModule( eLogPlayerNlc, LOG_ERROR, "media player has no player factory init level %d",
                  m_ServiceManager->GetInitLevel() );
        return false;
    }

    return true;
}

//============================================================================
bool MediaPlayerNlc::fromGuiPlayMedia( AssetBaseInfo& assetInfo, int pos0to100000 )
{
	fromGuiMediaPlayerAction( eMediaPlayerActionPlayStop );
    bool result{ false };
    if( !assureInitialized() )
    {
        LogModule( eLogPlayerNlc, LOG_ERROR, "%s media player not initialized", __func__ );
        return false;
    }

	if( assetInfo.isStream() || assetInfo.isValidFile() )
	{
		m_AssetInfo = assetInfo;
		if( !assetInfo.isStream() || !m_FeedId.isValid() )
		{
			m_FeedId = m_AssetInfo.getAssetUniqueId();
		}

        m_FileItem = CFileItem( assetInfo );
        m_FileItem.setIsVirtualStream( assetInfo.isStream() );
		if( pos0to100000 )
		{
			std::string kodiPercent = std::to_string( 100000.0f / (float)pos0to100000 );
			m_FileItem.SetProperty( "StartPercent", kodiPercent.c_str() );
		}

        // because android file name has a content provider path instead of file path,
        // always set the meta data
        if( assetInfo.isVideoAsset() )
        {
            std::string metadata = "video/";
            metadata += assetInfo.getFileExtension();
            m_FileItem.SetMimeType(metadata);
        }

        if( assetInfo.isAudioAsset() )
        {
            std::string metadata = "audio/";
            metadata += assetInfo.getFileExtension();
            m_FileItem.SetMimeType(metadata);
		}

		EnableLogTimer( true );

		if( m_AssetInfo.isVideoAsset() )
		{
			result = playVideoFile( pos0to100000 );
		}
		else if( m_AssetInfo.isAudioAsset() )
		{
			result = playAudioFile( pos0to100000 );
		}
	}
	else
	{
		LogModule( eLogPlayerNlc, LOG_ERROR, "%s assetInfo invalid", __func__ );
	}

	return result;
}

//============================================================================
bool MediaPlayerNlc::fromGuiPlayStream( AssetBaseInfo& assetInfo, VxGUID lclSessionId, int pos0to100000 )
{
	LogModule( eLogStreams, LOG_VERBOSE, "MediaPlayerNlc::%s file %s", __func__, assetInfo.getFileName().c_str() );
	m_FeedId = lclSessionId;
	return fromGuiPlayMedia( assetInfo, pos0to100000 );
}

//============================================================================
bool MediaPlayerNlc::fromGuiMediaPlayerAction( EMediaPlayerAction playerAction )
{
	bool result{ false };
	if( eMediaPlayerActionPlayStop == playerAction )
	{
		if( m_ModuleIsRunning )
		{
			StopPlaying();
		}
		
		return true;
	}

	assureInitialized();


	return result;
}

//============================================================================
bool MediaPlayerNlc::playAudioFile( int position0to100000 )
{
    assureInitialized();
	const std::string audioPlayerName( "audiodefaultplayer" );
	bool result = PlayFile( m_FileItem, audioPlayerName, false );
	if( result )
	{
		setIsPlayingMedia( true );
	}

	return result;
}

//============================================================================
bool MediaPlayerNlc::playVideoFile( int position0to100000 )
{
    assureInitialized();
	const std::string videoPlayerName( "videodefaultplayer" );
	bool result = PlayFile( m_FileItem, videoPlayerName, false );
	if( result )
	{
		setIsPlayingVideo( true );
		setIsPlayingMedia( true );
	}
    else
    {
        onPlaybackError();
	}

	return result;
}

//============================================================================
void MediaPlayerNlc::onInitLevel( int level, bool success )
{
	if( level == 3 && success )
	{
		initVideoSettings();
	}

	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerInitLevel( level, success );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onPlayerRunning( bool isRunning )
{
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerIsRunning( isRunning );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onPlayFile( bool fileOpened )
{
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerPlayFile( m_FeedId, fileOpened );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onPlayStarted( void )
{
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerPlaybackStarted( m_FeedId );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onStopPlaying( void )
{
	setIsPlayingMedia( false );
	setIsPlayingVideo( false );
}

//============================================================================
void MediaPlayerNlc::onPlaybackStopped( void )
{
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerPlaybackStopped( m_FeedId );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onPlaybackEnded( void )
{
	setIsPlayingVideo( false );
	setIsPlayingMedia( false );
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerPlaybackEnded( m_FeedId );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onPlayPause( bool isPaused )
{
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerPlayPause( m_FeedId, isPaused );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onCanSeek( bool canSeek, bool canPause )
{
	lockClientList();
    for( auto& client : m_MediaPlayerCallbackClients )
    {
		client->fromMediaPlayerCanSeek( m_FeedId, canSeek, canPause );
    }

    unlockClientList();
}

//============================================================================
void MediaPlayerNlc::fromGuiPlayPauseButtonClicked( void )
{
	PlayPauseButtonClicked();
}

//============================================================================
void MediaPlayerNlc::fromGuiStopButtonClicked( void )
{
	fromGuiMediaPlayerAction( eMediaPlayerActionPlayStop );
}

//============================================================================
void MediaPlayerNlc::fromGuiGetCanSeek( void )
{
	const auto appPlayer = GetComponent<CApplicationPlayer>();
	if( appPlayer )
	{
		bool canSeek = appPlayer->CanSeek();
		bool canPause = appPlayer->CanPause();
		lockClientList();
		for( auto& client : m_MediaPlayerCallbackClients )
		{
			client->fromMediaPlayerCanSeek( m_FeedId, canSeek, canPause );
		}

		unlockClientList();
	}
}

//============================================================================
bool MediaPlayerNlc::fromGuiMediaPlayerSeek( int position0to100000 )
{
	bool result{ false };
	assureInitialized();

	float playPos = (float)(position0to100000) / 1000.0f;
	const auto appPlayer = GetComponent<CApplicationPlayer>();
	if( appPlayer )
	{
		if( appPlayer->CanSeek() )
		{
			appPlayer->SeekPercentage(playPos);
			result = true;
		}
	}

	return result;
}

//============================================================================
void MediaPlayerNlc::fromGuiUpdatePlayPosition( void )
{
	int playPos{ 0 };
	const auto appPlayer = GetComponent<CApplicationPlayer>();
	if( appPlayer && appPlayer->IsPlaying() )
	{
		float playPercent = appPlayer->GetPercentage();
		if( playPercent <= 0.0f )
		{
			const int64_t totalTime = appPlayer->GetTotalTime();
			const int64_t playTime = appPlayer->GetTime();
			if( totalTime > 0 && playTime > 0 )
			{
				playPercent = (float)playTime * 100.0f / (float)totalTime;
			}
		}

		playPercent = std::clamp( playPercent, 0.0f, 100.0f );
		playPos = (int)( playPercent * 1000.0f );
	}

	lockClientList();
	for( auto& client : m_MediaPlayerCallbackClients )
	{
		client->fromMediaPlayerUpdatePlayPosition( m_FeedId, playPos );
	}

	unlockClientList();
}

//============================================================================
void MediaPlayerNlc::onPlaybackPaused( void ) 
{
	onPlayPause( true );
}

//============================================================================
void MediaPlayerNlc::onPlaybackResumed( void )  
{
	onPlayPause( false );
}

//============================================================================
void MediaPlayerNlc::onPlaybackError( void ) 
{
	onPlaybackStopped();
}

//============================================================================
void MediaPlayerNlc::fromGuiUpdateGlWidgetSize( int width, int height ) 
{
	//if( CServiceBroker::GetWinSystem() )
	//{
	//	CServiceBroker::GetWinSystem()->GetGfxContext().ApplyWindowResize( width, height );
	//}		
}

//============================================================================
void MediaPlayerNlc::initVideoSettings( void )
{
	// setup video settings because we do not read from files
	//CDisplaySettings::GetInstance()

	auto settingsComponent = CServiceBroker::GetSettingsComponent();
	if (!settingsComponent)
		return;

	auto settings = settingsComponent->GetSettings();
	if (!settings)
		return;

	SettingPtr renderWidth = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_WINDOW_WIDTH, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( renderWidth );
    settings->SetInt( CSettings::SETTING_WINDOW_WIDTH, 320 );

	SettingPtr renderHeight = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_WINDOW_HEIGHT, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( renderHeight );
    settings->SetInt( CSettings::SETTING_WINDOW_HEIGHT, 240 );

    SettingPtr useDisplayAsClock = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( useDisplayAsClock );
    settings->SetBool( CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK, false );

	SettingPtr audioPassthrough = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( audioPassthrough );
    settings->SetBool( CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, false );

	SettingPtr stereoscopicPlayback = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( stereoscopicPlayback );
    settings->SetInt( CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE, 0 ); // should probably be 100 (ignore) instead of ask

	SettingPtr playerStretch = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_VIDEOPLAYER_STRETCH43, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( playerStretch );
    settings->SetInt( CSettings::SETTING_VIDEOPLAYER_STRETCH43, 0 );

	SettingPtr renderMethod = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( renderMethod );
    settings->SetInt( CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD, 0 );
	
	SettingPtr noofBuffers = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_VIDEOSCREEN_NOOFBUFFERS, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( noofBuffers );
    settings->SetInt( CSettings::SETTING_VIDEOSCREEN_NOOFBUFFERS, 3 );

	SettingPtr allowAspectErr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( allowAspectErr );
    settings->SetInt( CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT, 0 );

	// <setting id="subtitles.align" default="true">0</setting>
	SettingPtr settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_ALIGN, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_ALIGN, 0 );

	// <setting id="subtitles.bgcolor" default="true">0</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_BGCOLOR, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_BGCOLOR, 0 );

	// <setting id="subtitles.bgopacity" default="true">0</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_BGOPACITY, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_BGOPACITY, 0 );

	// <setting id="subtitles.charset" default="true">DEFAULT</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_SUBTITLES_CHARSET, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_SUBTITLES_CHARSET, "DEFAULT" );

	// <setting id="subtitles.color" default="true">1</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_COLOR, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_COLOR, 1 );
    
    // <setting id = "subtitles.custompath" default = "true">< / setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_SUBTITLES_CUSTOMPATH, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_SUBTITLES_CUSTOMPATH, "" );

    // <setting id="subtitles.downloadfirst" default="true">false</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_SUBTITLES_DOWNLOADFIRST, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetBool( CSettings::SETTING_SUBTITLES_DOWNLOADFIRST, false );

    // <setting id="subtitles.font" default="true">arial.ttf</setting>
	SettingPtr subsCustomPath = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_SUBTITLES_FONTNAME, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( subsCustomPath );
    settings->SetString( CSettings::SETTING_SUBTITLES_FONTNAME, "arial.ttf" );
	
    // <setting id="subtitles.height" default="true">28</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_FONTSIZE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_FONTSIZE, 28 );

	// <setting id="subtitles.overrideassfonts" default="true">false</setting> 
	settingPtr = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_SUBTITLES_OVERRIDEFONTS, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetBool( CSettings::SETTING_SUBTITLES_OVERRIDEFONTS, false );

    // <setting id="subtitles.languages" default="true">English</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_SUBTITLES_LANGUAGES, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_SUBTITLES_LANGUAGES, "English" );

	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_LOCALE_SUBTITLELANGUAGE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_LOCALE_SUBTITLELANGUAGE, "original" );
	
    // <setting id="subtitles.movie" default="true"></setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_SUBTITLES_MOVIE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetBool( CSettings::SETTING_SUBTITLES_MOVIE, true );

	// <setting id="subtitles.tv" default="true"></setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_SUBTITLES_TV, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetBool( CSettings::SETTING_SUBTITLES_TV, true );
	
    // <setting id="subtitles.parsecaptions" default="true">false</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_SUBTITLES_PARSECAPTIONS, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetBool( CSettings::SETTING_SUBTITLES_PARSECAPTIONS, false );

    // <setting id="subtitles.pauseonsearch" default="true">true</setting>
	SettingPtr parseCaptions = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_SUBTITLES_PAUSEONSEARCH, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( parseCaptions );
    settings->SetBool( CSettings::SETTING_SUBTITLES_PAUSEONSEARCH, true );

    // <setting id="subtitles.stereoscopicdepth" default="true">0</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH, 0 );

    // <setting id="subtitles.storagemode" default="true">0</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_STORAGEMODE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_STORAGEMODE, 0 );

    // <setting id="subtitles.style" default="true">1</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "integer", CSettings::SETTING_SUBTITLES_STYLE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetInt( CSettings::SETTING_SUBTITLES_STYLE, 1 );

	// <setting id="locale.audiolanguage" default="true">mediadefault</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_LOCALE_AUDIOLANGUAGE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_LOCALE_AUDIOLANGUAGE, "mediadefault" );

	// <setting id="locale.charset" default="true">DEFAULT</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_LOCALE_CHARSET, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_LOCALE_CHARSET, "DEFAULT" );

	// <setting id="locale.language" default="true">resource.language.en_gb</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_LOCALE_LANGUAGE, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_LOCALE_LANGUAGE, "resource.language.en_gb" );

	// <setting id="locale.country" default="true">USA (12h)</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "string", CSettings::SETTING_LOCALE_COUNTRY, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetString( CSettings::SETTING_LOCALE_COUNTRY, "USA (12h)" );

	// <setting id="videoplayer.preferdefaultflag" default="true">true</setting>
	settingPtr = settings->GetSettingsManager()->CreateSetting( "boolean", CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG, settings->GetSettingsManager() );
    settings->GetSettingsManager()->AddSettingInternal( settingPtr );
    settings->SetBool( CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG, true );
}

//============================================================================
void MediaPlayerNlc::fromGuiAppShutdown( void )
{
#if defined(HAVE_NLC_GUI)
	if( CServiceBroker::GetRenderSystem() )
	{
		CServiceBroker::GetRenderSystem()->DestroyRenderSystem();
	}
#endif // defined(HAVE_NLC_GUI)
}