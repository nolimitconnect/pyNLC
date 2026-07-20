#pragma once
//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletPlayerBase.h"

#include "GuiPlayerCallback.h"

#include <AssetBase/AssetPlaySession.h>
#include <GuiInterface/IMediaPlayerCallback.h>

#include <QElapsedTimer>

class PlayControlWidget;
class RenderGlWidget;
class QMediaPlayer;
class QProgressDialog;
class QPushButton;
class QSlider;
class VxPushButton;

class AppletPlayerNlcBase : public AppletPlayerBase, public GuiPlayerCallback, public IMediaPlayerCallback
{
	Q_OBJECT
public:
	AppletPlayerNlcBase( const char* ObjName, AppCommon& app, QWidget* parent );
	virtual ~AppletPlayerNlcBase();

	virtual EMediaModule		getMediaModule( void ) { return m_MediaModule; }

	virtual RenderGlWidget*		getRenderConsumer( void ) = 0;
	virtual QSlider*			getPlayPosSlider( void ) = 0;
	virtual QPushButton*		getReplayButton( void ) = 0;
	virtual VxPushButton*		getPlayPauseButton( void ) = 0;
	virtual PlayControlWidget*	getPlayControlWidget( void ) = 0;

	virtual void				onMediaPlayerNlcReady( bool isReady );

    void						toGuiClientAssetAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 ) override;

    void						callbackGuiPlayMotionVideoFrame( VxGUID& feedOnlineId, QImage& vidFrame, int motion0To100000 ) override {};

	void						fromMediaPlayerInitLevel( int level, bool success ) override;
	void						fromMediaPlayerIsRunning( bool isRunning ) override;

	void						fromMediaPlayerPlayFile( VxGUID& feedId, bool fileOpened ) override;

	void						fromMediaPlayerPlaybackStarted( VxGUID& feedId ) override;
	void						fromMediaPlayerPlaybackStopped( VxGUID& feedId ) override;
	void						fromMediaPlayerPlaybackEnded( VxGUID& feedId ) override;

	void						fromMediaPlayerPlayPause( VxGUID& feedId, bool isPaused ) override;

	void						fromMediaPlayerCanSeek( VxGUID& feedId, bool canSeek, bool canPause ) override;
	void						fromMediaPlayerUpdatePlayPosition( VxGUID& feedId, int pos0to100000 ) override;

	void						setIsPlaying( bool isPlaying )	{ m_IsPlaying = isPlaying; }
	bool						isPlaying( void )				{ return m_IsPlaying; }

	void						resetPlayerControls( void );

	void						updateLastPlayedFile( void );

	virtual bool				playMedia( AssetPlaySession& assetPlaySession, bool useExternalPlayer ) override;

signals:
	void						signalInternalInitLevel( int level, bool success );
	void						signalInternalPlayerNlcIsRunning( bool isRunning );

	void						signalInternalPlayFile( VxGUID feedId, bool fileOpened );
	void						signalInternalPlayStarted( VxGUID feedId );

	void						signalInternalPlaybackStopped( VxGUID feedId );
	void						signalInternalPlaybackEnded( VxGUID feedId );
	void						signalInternalPlayPause( VxGUID feedId, bool isPaused );
	void						signalInternalCanSeek( VxGUID feedId, bool canSeek, bool canPause );
	void						signalInternalUpdatePlayPosition( VxGUID feedId, int pos0to100000 );

protected slots:
	void						slotInternalInitLevel( int level, bool success );
	void						slotInternalPlayerNlcIsRunning( bool isRunning );

	void						slotInternalPlayFile( VxGUID feedId, bool fileOpened );
	void						slotInternalPlayStarted( VxGUID feedId );

	void						slotInternalPlaybackStopped( VxGUID feedId );
	void						slotInternalPlaybackEnded( VxGUID feedId );
	void						slotInternalPlayPause( VxGUID feedId, bool isPaused );
	void						slotInternalCanSeek( VxGUID feedId, bool canSeek, bool canPause );
	void						slotInternalUpdatePlayPosition( VxGUID feedId, int pos0to100000 );

	void						slotPlayButtonClicked( void );
	void						slotSliderPressed( void );
	void						slotSliderReleased( void );

	void						slotPlayProgress( int pos0to100000 );
	void						slotPlayEnd( void );

	void                        slotReplayButtonClick( void );

	void						slotPlayPauseButtonClick( void );
	void						slotStopButtonClick( void );
	void						slotSliderChanged( int sliderPos );
	void						slotUpdateControlsTimeout( void );

	void                        slotLeftMouseButtonClick( void );

protected:
	void						initAppletPlayerNlcBase( void );

	void						onAppletInitialized( void ); // must be called after derived class applet ui is initialized so can connect widgets to slots

	void						showEvent( QShowEvent* ev ) override;
	void						hideEvent( QHideEvent* ev ) override;
	void						resizeEvent( QResizeEvent* ev ) override;
	void						closeEvent( QCloseEvent* ev ) override;

	virtual bool				playAsset( VxGUID& sessionId, AssetBaseInfo& assetInfo, int pos0to100000 );
	virtual bool				playStream( VxGUID& sessionId, AssetBaseInfo& assetInfo, int pos0to100000 );
	bool						playMediaFile( VxGUID& sessionId, std::string fileStr, int pos0to100000, bool isStream );

	void						setReadyForCallbacks( bool isReady );
	virtual void				updateGuiPlayControls( bool isPlaying );

	void						startMediaPlay( int startPos );
    virtual void				stopMediaIfPlaying( void );

	bool						waitForPlayerThread( void );

	virtual void				onPlayStarted( VxGUID& feedId );
	virtual void				onPlaybackStopped( VxGUID& feedId );
	virtual void				onPlaybackEnded( VxGUID& feedId );
	virtual void				onPlayPause( VxGUID& feedId, bool isPaused );
	virtual void				onCanSeek( VxGUID& feedId, bool canSeek, bool canPause );
	virtual void				onUpdatePlayPosition( VxGUID& feedId, int pos0to100000 );

    void                        onBackButtonClicked( void ) override;
    void                        onActivityFinish( void ) override;

    void                        onAboutToDestroyApplet( void );

	void						setVisible( bool visible ) override;

	void						setIsStreaming( bool isStreaming ) { m_IsStreaming = isStreaming; }
	bool						getIsStreaming( void ) { return m_IsStreaming; }

private:

	//=== vars ===//
	EMediaModule					m_MediaModule{ eMediaModulePlayerNlc };
	bool						m_ActivityCallbacksEnabled{ false };
	bool						m_IsPlaying{ false };
	bool						m_SliderIsPressed{ false };

	QElapsedTimer				m_ElapsedTimer;

	bool						m_LastPlayedIsStream{ false };
	bool						m_LastPlayedIsFile{ false };
	std::string					m_LastPlayedMediaName;
	std::string					m_LastPlayedMediaFile;

	std::vector<AssetPlaySession> m_PlayAssetQue;
	VxGUID						m_SessionId;
	bool						m_IsStreaming{ false };

    bool						m_AboutToDestroyHasBeenCalled{ false };
};


