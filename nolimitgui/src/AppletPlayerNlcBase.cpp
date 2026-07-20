//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletPlayerNlcBase.h"

#include "AppCommon.h"
#include "GuiHelpers.h"
#include "GuiPluginMgr.h"
#include "GuiPlayerMgr.h"
#include "HomeWindow.h"
#include "PlayControlWidget.h"
#include "MediaPlayerNlc.h"
#include "RenderGlWidget.h"
#include "GuiAudioMgr.h"
#include "VxPushButton.h"

#include <P2PEngine/P2PEngine.h>
#include <VirtStream/VirtStreamMgr.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>

#include <QApplication>
#include <QMessageBox>
#include <QSlider>
#include <QTimer>

namespace
{
	bool isCallbackForCurrentAsset( AssetBaseInfo& assetInfo, VxGUID& sessionId, VxGUID& feedId )
	{
		if( !feedId.isValid() )
		{
			return true;
		}

		if( assetInfo.isStream() )
		{
			return !sessionId.isValid() || ( feedId == sessionId );
		}

		VxGUID assetId = assetInfo.getAssetUniqueId();
		return !assetId.isValid() || ( feedId == assetId );
	}
}

//============================================================================
AppletPlayerNlcBase::AppletPlayerNlcBase( const char* ObjName, AppCommon& app, QWidget* parent )
: AppletPlayerBase( ObjName, app, parent )
{
	initAppletPlayerNlcBase();
}

//============================================================================
void AppletPlayerNlcBase::initAppletPlayerNlcBase( void )
{
	connect( this, SIGNAL(signalInternalInitLevel(int,bool)), this, SLOT(slotInternalInitLevel(int,bool)), Qt::QueuedConnection );
	connect( this, SIGNAL(signalInternalPlayerNlcIsRunning(bool)), this, SLOT(slotInternalPlayerNlcIsRunning(bool)), Qt::QueuedConnection );

	connect( this, SIGNAL(signalInternalPlayFile(VxGUID,bool)), this, SLOT(slotInternalPlayFile(VxGUID,bool)), Qt::QueuedConnection );
	connect( this, SIGNAL(signalInternalPlayStarted(VxGUID)), this, SLOT(slotInternalPlayStarted(VxGUID)), Qt::QueuedConnection );

	connect( this, SIGNAL(signalInternalPlaybackStopped(VxGUID)), this, SLOT(slotInternalPlaybackStopped(VxGUID)), Qt::QueuedConnection );
	connect( this, SIGNAL(signalInternalPlaybackEnded(VxGUID)), this, SLOT(slotInternalPlaybackEnded(VxGUID)), Qt::QueuedConnection );
	connect( this, SIGNAL(signalInternalPlayPause(VxGUID,bool)), this, SLOT(slotInternalPlayPause(VxGUID,bool)), Qt::QueuedConnection );
	connect( this, SIGNAL(signalInternalCanSeek(VxGUID,bool,bool)), this, SLOT(slotInternalCanSeek(VxGUID,bool,bool)), Qt::QueuedConnection );
	connect( this, SIGNAL(signalInternalUpdatePlayPosition(VxGUID,int)), this, SLOT(slotInternalUpdatePlayPosition(VxGUID,int)), Qt::QueuedConnection );

	IMediaPlayerRequests::getNlcPlayer().wantMediaPlayerCallback( this, true );
}

//============================================================================
AppletPlayerNlcBase::~AppletPlayerNlcBase()
{
	stopMediaIfPlaying();
	IMediaPlayerRequests::getNlcPlayer().wantMediaPlayerCallback( this, false );
	m_MyApp.getAudioMgr().setPlayerNlcActive( false );
	m_MyApp.activityStateChange( this, false );
}

//============================================================================
void AppletPlayerNlcBase::showEvent( QShowEvent* showEvent )
{
	AppletBase::showEvent( showEvent );
	wantActivityCallbacks( true );
}

//============================================================================
void AppletPlayerNlcBase::hideEvent( QHideEvent* hideEvent )
{
	wantActivityCallbacks( false );
	AppletBase::hideEvent( hideEvent );
}

//============================================================================
void AppletPlayerNlcBase::resizeEvent( QResizeEvent* ev )
{
	AppletBase::resizeEvent( ev );
}

//============================================================================
void AppletPlayerNlcBase::closeEvent( QCloseEvent* ev )
{
	AppletBase::closeEvent( ev );
	onAboutToDestroyApplet();
}

//============================================================================
void AppletPlayerNlcBase::onAppletInitialized( void )
{
	getPlayPosSlider()->setRange(0, 100000);

	connect( getPlayPosSlider(), SIGNAL(sliderPressed()), this, SLOT(slotSliderPressed()) );
	connect( getPlayPosSlider(), SIGNAL(sliderReleased()), this, SLOT(slotSliderReleased()) );

	connect( getReplayButton(), SIGNAL(clicked()), this, SLOT(slotReplayButtonClick()) );

	connect( this, SIGNAL(signalPlayProgress(int)), this, SLOT(slotPlayProgress(int)) );
	connect( this, SIGNAL(signalPlayEnd()), this, SLOT(slotPlayEnd()) );

	m_MyApp.activityStateChange( this, true );
	m_MyApp.getAudioMgr().setPlayerNlcActive( true );
}

//============================================================================
void AppletPlayerNlcBase::setReadyForCallbacks( bool isReady )
{
	if( m_ActivityCallbacksEnabled != isReady )
	{
		m_ActivityCallbacksEnabled = isReady;
		wantActivityCallbacks( isReady );
	}
}

//============================================================================
bool AppletPlayerNlcBase::playMedia( AssetPlaySession& assetPlaySession, bool useExternalPlayer )
{
	if( !IMediaPlayerRequests::getNlcPlayer().fromGuiIsModuleRunning( eMediaModulePlayerNlc ) )
	{
		// player not ready so queue for play when ready
		m_PlayAssetQue.emplace_back( assetPlaySession );
		startBusySpinner( this );
		return true;
	}

	return playAsset( assetPlaySession.getSessionId(), assetPlaySession, assetPlaySession.getPlayPosition() );
}

//============================================================================
bool AppletPlayerNlcBase::playAsset( VxGUID& sessionId, AssetBaseInfo& assetInfo, int pos0to100000 )
{
	stopMediaIfPlaying();
	AppletPlayerBase::setAssetInfo( assetInfo );
	if( assetInfo.isStream() )
	{
		m_SessionId = sessionId;
		return playStream( sessionId, assetInfo, pos0to100000 );
	}

	return IMediaPlayerRequests::getNlcPlayer().fromGuiPlayMedia( assetInfo, pos0to100000 );
}

//============================================================================
bool AppletPlayerNlcBase::playStream( VxGUID& sessionId, AssetBaseInfo& assetInfo, int pos0to100000 )
{
	if( sessionId.isValid() )
	{
		m_SessionId = sessionId;
	}

	bool canPlay = GetVirtStreamMgr().fromGuiPlayStream( assetInfo, sessionId, pos0to100000 );
	if( canPlay )
	{
		canPlay &= IMediaPlayerRequests::getNlcPlayer().fromGuiPlayStream( assetInfo, sessionId, pos0to100000 );
	}
	else
	{
		LogModule( eLogStreams, LOG_VERBOSE, "%s VirtStreamMgr says cannot play", __func__ );
	}
	
	setIsStreaming( canPlay );
	return canPlay;
}

//============================================================================
bool AppletPlayerNlcBase::playMediaFile( VxGUID& sessionId, std::string mediaFile, int pos0to100000, bool isStream )
{
	if( !waitForPlayerThread() )
	{
		return false;
	}

	if( VxFileUtil::fileExists( mediaFile.c_str() ) )
    {
		return AppletPlayerBase::playFile( mediaFile.c_str(), pos0to100000, isStream, false );
	}
    else
    {
		QString fileName( mediaFile.c_str() );
        QMessageBox::information( this, QObject::tr("File does not exist"), fileName, QMessageBox::Ok );
		return false;
    }
}

//============================================================================
bool AppletPlayerNlcBase::waitForPlayerThread( void )
{
	m_ElapsedTimer.start();
	while( !IMediaPlayerRequests::getNlcPlayer().fromGuiIsModuleRunning( eMediaModulePlayerNlc ) )
	{
		GuiHelpers::processQtEvents( 100 );
		if( m_ElapsedTimer.elapsed() > 6000 )
		{
			LogMsg( LOG_ERROR, "Media Player Failed To Start" );
			return false;
		}
	}

	return true;
}

//============================================================================
void AppletPlayerNlcBase::toGuiClientAssetAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 )
{
	AppletPlayerBase::toGuiClientAssetAction( assetAction, assetId, pos0to100000 );
	switch( assetAction )
	{
	case eAssetActionPlayProgress:
		if( false == m_SliderIsPressed )
		{
			updateGuiPlayControls( true );
			getPlayPosSlider()->setValue(pos0to100000);
		}

		break;

	case eAssetActionPlayEnd:
		if( false == m_SliderIsPressed )
		{
			updateGuiPlayControls( false );
		}

		break;

	default:
		break;
	}
}

//============================================================================
void AppletPlayerNlcBase::slotSliderPressed( void )
{
	m_SliderIsPressed = true;
}

//============================================================================
void AppletPlayerNlcBase::slotSliderReleased( void )
{
	m_SliderIsPressed = false;
}

//============================================================================
void AppletPlayerNlcBase::slotPlayButtonClicked( void )
{
	if( m_IsPlaying )
	{
		stopMediaIfPlaying();
	}
	else
	{
		startMediaPlay( 0 );
	}
}

//========================================================================
void AppletPlayerNlcBase::startMediaPlay( int startPos )
{
	// Start every play from a clean player-audio cache state.
	m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
	m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( true );
	bool playStarted = m_Engine.fromGuiAssetAction( eAssetActionPlayBegin, m_AssetInfo, startPos );
	updateGuiPlayControls( playStarted );
	if( false == playStarted )
	{
		m_MyApp.toGuiStatusMessage( "Video Play FAILED TO Begin" );
	}
}

//========================================================================
void AppletPlayerNlcBase::updateGuiPlayControls( bool isPlaying )
{
	if( m_IsPlaying != isPlaying )
	{
		m_IsPlaying = isPlaying;
		if( m_IsPlaying )
		{
			setReadyForCallbacks( true );
		}
	}
}

//============================================================================
void AppletPlayerNlcBase::stopMediaIfPlaying( void )
{
	if( m_IsPlaying )
	{
		// Stop accepting decoder callbacks immediately so queue state cannot repopulate during stop.
		m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( false );
		m_MyApp.getAudioMgr().clearPlayerNlcBuffers();

		m_MyApp.toGuiStatusMessage( "" );
		m_Engine.fromGuiAssetAction( eAssetActionPlayEnd, m_AssetInfo, 0 );

		stopBusySpinner();
	}

	updateGuiPlayControls( false );
	if( getIsStreaming() )
	{
		setIsStreaming( false );
		GetVirtStreamMgr().onPlaybackStopped( m_SessionId );
	}

	IMediaPlayerRequests::getNlcPlayer().fromGuiStopButtonClicked();
	m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( false );
	m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
}

//============================================================================
void AppletPlayerNlcBase::slotPlayProgress( int pos0to100000 )
{
	if( m_IsPlaying && (false == m_SliderIsPressed) )
	{
		getPlayPosSlider()->setValue(pos0to100000);
	}
}

//============================================================================
void AppletPlayerNlcBase::slotPlayEnd( void )
{
	updateGuiPlayControls( false );
}

//============================================================================
void AppletPlayerNlcBase::slotReplayButtonClick( void )
{
	if( m_LastPlayedIsFile )
	{
		if( VxFileUtil::fileExists( m_LastPlayedMediaFile.c_str() ) )
		{
			startBusySpinner(getPlayControlWidget());
			playFile( m_LastPlayedMediaFile.c_str(), 0, false, false );
		}
		else
		{
			QMessageBox::information( this, QObject::tr( "File does not exist" ), QString(m_LastPlayedMediaFile.c_str()), QMessageBox::Ok );
		}
	}
	else if( m_LastPlayedIsStream && m_AssetInfo.isStream() )
	{
		VxGUID replaySessionId;
		replaySessionId.initializeWithNewVxGUID();
		startBusySpinner( getPlayControlWidget() );
		playAsset( replaySessionId, m_AssetInfo, 0 );
	}
}

//============================================================================
void AppletPlayerNlcBase::slotLeftMouseButtonClick( void )
{
	PlayControlWidget* playControl = getPlayControlWidget();
	if( playControl )
	{
		playControl->setVisible( !playControl->isVisible() );
	}
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerInitLevel( int level, bool success )
{
	emit signalInternalInitLevel( level, success );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalInitLevel( int level, bool success )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::slotInternalInitLevel level %d sucess ? %d", level, success );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerIsRunning( bool isRunning )
{
	emit signalInternalPlayerNlcIsRunning( isRunning );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalPlayerNlcIsRunning( bool isRunning )
{
	LogModule( eLogPlayerNlc, LOG_DEBUG, "%s %d", __func__, isRunning );
    if( isRunning )
    {
        getRenderConsumer()->showAppIcon();
    }

    onMediaPlayerNlcReady( isRunning );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerPlayFile( VxGUID& feedId, bool fileOpened )
{
	emit signalInternalPlayFile( feedId, fileOpened );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalPlayFile( VxGUID feedId, bool fileOpened )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s file opened ? %d", __func__, fileOpened );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerPlaybackStarted( VxGUID& feedId )
{
	emit signalInternalPlayStarted( feedId );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalPlayStarted( VxGUID feedId )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s", __func__ );
	onPlayStarted( feedId );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerPlaybackStopped( VxGUID& feedId )
{
	emit signalInternalPlaybackStopped( feedId );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalPlaybackStopped( VxGUID feedId )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s", __func__ );
	onPlaybackStopped( feedId );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerPlaybackEnded( VxGUID& feedId )
{
	emit signalInternalPlaybackEnded( feedId );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalPlaybackEnded( VxGUID feedId )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s", __func__ );
	onPlaybackEnded( feedId );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerPlayPause( VxGUID& feedId, bool isPaused )
{
	emit signalInternalPlayPause( feedId, isPaused );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalPlayPause( VxGUID feedId, bool isPaused )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s", __func__ );
	onPlayPause( feedId, isPaused );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerCanSeek( VxGUID& feedId, bool canSeek, bool canPause )
{
	emit signalInternalCanSeek( feedId, canSeek, canPause );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalCanSeek( VxGUID feedId, bool canSeek, bool canPause )
{
	onCanSeek( feedId, canSeek, canPause );
}

//============================================================================
void AppletPlayerNlcBase::fromMediaPlayerUpdatePlayPosition( VxGUID& feedId, int pos0to100000 )
{
	emit signalInternalUpdatePlayPosition( feedId, pos0to100000 );
}

//============================================================================
void AppletPlayerNlcBase::slotInternalUpdatePlayPosition( VxGUID feedId, int pos0to100000 )
{
	onUpdatePlayPosition( feedId, pos0to100000 );
}

//============================================================================
void AppletPlayerNlcBase::onBackButtonClicked( void )
{
	stopMediaIfPlaying();

#if defined(TARGET_OS_ANDROID)
	// Android player widget teardown can blank the app surface after control returns
	// to the previous page. Keep the applet alive and reusable instead of deleting it.
	IMediaPlayerRequests::getNlcPlayer().fromGuiStopModule( eMediaModulePlayerNlc );
	onMediaPlayerNlcReady( false );
	setReadyForCallbacks( false );
	stopBusySpinner();
	resetPlayerControls();
	setIsPlaying( false );
	setIsStreaming( false );
	m_PlayAssetQue.clear();
	m_SessionId = VxGUID::nullVxGUID();
	m_AssetInfo = AssetInfo();
	getRenderConsumer()->showAppIcon();
	getPlayControlWidget()->setVisible( false );
	hide();
	return;
#endif // defined(TARGET_OS_ANDROID)

	AppletPlayerBase::onBackButtonClicked();
}

//============================================================================
void AppletPlayerNlcBase::onActivityFinish( void )
{
    stopMediaIfPlaying();
    onAboutToDestroyApplet();
    AppletPlayerBase::onActivityFinish();
}

//============================================================================
void AppletPlayerNlcBase::onPlayStarted( VxGUID& feedId )
{
	if( !isCallbackForCurrentAsset( m_AssetInfo, m_SessionId, feedId ) )
	{
		return;
	}

	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s id %s", __func__, feedId.toHexString().c_str() );
	stopBusySpinner();
	updateGuiPlayControls( true );
	resetPlayerControls();
	setIsPlaying( true );
	updateLastPlayedFile();
}

//============================================================================
void AppletPlayerNlcBase::onPlaybackStopped( VxGUID& feedId )
{
	if( !isCallbackForCurrentAsset( m_AssetInfo, m_SessionId, feedId ) )
	{
		return;
	}

	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s id %s", __func__, feedId.toHexString().c_str() );
	if( getIsStreaming() )
	{
		setIsStreaming( false );
		GetVirtStreamMgr().onPlaybackStopped( feedId );
	}

	stopBusySpinner();
	updateGuiPlayControls( false );
	resetPlayerControls();
	setIsPlaying( false );
	m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
	m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( false );
	// Keep the render widget from staying on a blank frame after manual stop.
	getRenderConsumer()->showAppIcon();
}

//============================================================================
void AppletPlayerNlcBase::onPlaybackEnded( VxGUID& feedId )
{
	if( !isCallbackForCurrentAsset( m_AssetInfo, m_SessionId, feedId ) )
	{
		return;
	}

	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s id %s", __func__, feedId.toHexString().c_str() );
	if( getIsStreaming() )
	{
		setIsStreaming( false );
		GetVirtStreamMgr().onPlaybackEnded( feedId );
	}

	stopBusySpinner();
	updateGuiPlayControls( false );
	resetPlayerControls();
	setIsPlaying( false );
	m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
	m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( false );
	getRenderConsumer()->showAppIcon();
}

//============================================================================
void AppletPlayerNlcBase::onCanSeek( VxGUID& feedId, bool canSeek, bool canPause )
{
	if( !isCallbackForCurrentAsset( m_AssetInfo, m_SessionId, feedId ) )
	{
		return;
	}

	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s is canSeek ? %d canPause ? %d", __func__, canSeek, canPause );
	stopBusySpinner();
	getPlayPauseButton()->setEnabled( canPause );
	getPlayPosSlider()->setEnabled( canSeek );
}

//============================================================================
void AppletPlayerNlcBase::onPlayPause( VxGUID& feedId, bool isPaused )
{
	if( !isCallbackForCurrentAsset( m_AssetInfo, m_SessionId, feedId ) )
	{
		return;
	}

	LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s is paused ? %d", __func__, isPaused );
	stopBusySpinner();
	if( isPaused )
	{
		// Flush immediately when paused so resume starts from a clean state.
		m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( false );
		m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
	}
	else
	{
		// Resume with a fresh queue; stale buffered frames can otherwise block startup.
		m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
		m_MyApp.getAudioMgr().setPlayerNlcAcceptInput( true );
	}

	if( isPaused )
	{ 
		getPlayPauseButton()->setIcons( eMyIconPauseNormal );
	}
	else
	{
		getPlayPauseButton()->setIcons( eMyIconPlayNormal );
	}
}

//============================================================================
void AppletPlayerNlcBase::slotPlayPauseButtonClick( void )
{
	if( isPlaying() )
	{
		IMediaPlayerRequests::getNlcPlayer().fromGuiPlayPauseButtonClicked();
	}
	else
	{
		slotReplayButtonClick();
	}
}

//============================================================================
void AppletPlayerNlcBase::slotStopButtonClick( void )
{
    stopMediaIfPlaying();
}

//============================================================================
void AppletPlayerNlcBase::slotSliderChanged( int sliderPos )
{
	m_MyApp.getAudioMgr().clearPlayerNlcBuffers();
	IMediaPlayerRequests::getNlcPlayer().fromGuiMediaPlayerSeek( sliderPos );
}

//============================================================================
void AppletPlayerNlcBase::slotUpdateControlsTimeout( void )
{
	if( m_IsPlaying )
	{
		IMediaPlayerRequests::getNlcPlayer().fromGuiUpdatePlayPosition();
	}
}

//============================================================================
void AppletPlayerNlcBase::onUpdatePlayPosition( VxGUID& feedId, int pos0to100000 )
{
	if( !isCallbackForCurrentAsset( m_AssetInfo, m_SessionId, feedId ) )
	{
		return;
	}

	slotPlayProgress( pos0to100000 );
}

//============================================================================
void AppletPlayerNlcBase::resetPlayerControls( void )
{
	getPlayPosSlider()->setValue( 0 );
	getPlayPauseButton()->setIcons( eMyIconPlayNormal );
	getPlayPauseButton()->setEnabled( true );
}

//============================================================================
void AppletPlayerNlcBase::updateLastPlayedFile( void )
{
	if( m_AssetInfo.isStream() )
	{
		m_LastPlayedIsStream = true;
		m_LastPlayedIsFile = false;
		m_LastPlayedMediaName = m_AssetInfo.getAssetName();
		m_LastPlayedMediaFile.clear();
	}
	else if( m_AssetInfo.isValidFile() )
	{
		m_LastPlayedIsStream = false;
		m_LastPlayedIsFile = true;
		m_LastPlayedMediaName = m_AssetInfo.getAssetName();
		m_LastPlayedMediaFile = m_AssetInfo.getAssetNameAndPath();
	}
	else
	{
		m_LastPlayedIsStream = false;
		m_LastPlayedIsFile = false;
	}
}

//============================================================================
void AppletPlayerNlcBase::onAboutToDestroyApplet( void )
{
    if( m_AboutToDestroyHasBeenCalled )
    {
        return;
    }
	m_AboutToDestroyHasBeenCalled = true;
	// Ensure stream and playback teardown runs before stopping the player module.
	// This prevents shutdown from waiting on active virtual-stream reads.
	stopMediaIfPlaying();
    getRenderConsumer()->aboutToDestroy();
	IMediaPlayerRequests::getNlcPlayer().fromGuiStopModule( eMediaModulePlayerNlc );

#if defined(TARGET_OS_ANDROID)
	// Android may leave a stale black surface after external player GL teardown.
	// Force the main window surface to rebind on the next event-loop turn.
	HomeWindow* homeWindow = &m_MyApp.getHomeWindow();
	QTimer::singleShot( 0, [homeWindow]()
	{
		if( homeWindow )
		{
			homeWindow->hide();
			homeWindow->show();
			homeWindow->activateWindow();
			homeWindow->raise();
			homeWindow->update();
		}

		const auto topLevelWidgets = QApplication::topLevelWidgets();
		for( QWidget* widget : topLevelWidgets )
		{
			if( widget )
			{
				widget->update();
			}
		}
	} );
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
void AppletPlayerNlcBase::setVisible( bool visible )
{
	AppletPlayerBase::setVisible( visible );
}

//============================================================================
void AppletPlayerNlcBase::onMediaPlayerNlcReady( bool isReady )
{
	if( isReady )
	{
		stopBusySpinner();
		if( m_PlayAssetQue.size() )
		{
			bool playResult = playAsset( m_PlayAssetQue.at( 0 ).getSessionId(), m_PlayAssetQue.at( 0 ), m_PlayAssetQue.at( 0 ).getPlayPosition() );
			m_PlayAssetQue.clear();
			if( !playResult )
			{
				LogModule( eLogPlayerNlc, LOG_VERBOSE, "AppletPlayerNlcBase::%s play failed", __func__ );
			}
		}
	}
}
