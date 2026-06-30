#pragma once

//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QWidget>

#include <GuiInterface/IDefs.h>
#include "ToGuiHardwareControlInterface.h"

#include <QObject>
#include <QTimer>

class AppCommon;
class AudioFrameAecBuffer;
class AudioCallbackSpaceAvailable;
class AudioDelayTestCallback;
class AudioMgr;
class GuiAudioLevelCallback;
class GuiAudioMgr;

class GuiAudioMgr : public QObject, public ToGuiHardwareControlInterface
{
    Q_OBJECT
public:

    GuiAudioMgr( AppCommon& app );
    ~GuiAudioMgr() = default;

    void                        audioIoSystemStartup( void );
    void                        audioIoSystemShutdown( void );

    bool                        isAudioInitialized( void );

    bool                        getIsMicrophoneAvailable( void );
    int                         getWantMicrophoneCount( void );

    bool                        getIsMicrophoneRunning( void );
    void                        setIsMicrophoneMuted( bool micMuted );
    bool                        getIsMicrophoneMuted( void );

    bool                        getIsSpeakerAvailable( void );
    int                         getWantSpeakerCount( void );

    bool                        getIsSpeakerRunning( void );
    void                        setIsSpeakerMuted( bool speakerMuted );
    bool                        getIsSpeakerMuted( void );

    int                         toGuiModuleAudioFrame( EMediaModule mediaModule, int16_t * pu16PcmData, int pcmDataLenInBytes );
    int                         toGuiPlayerNlcAudio( EMediaModule mediaModule, float* audioDataFloat, int audioDataLenInBytes );
    float                       toGuiGetAudioDelaySeconds( EMediaModule mediaModule );
    float                       toGuiGetAudioCacheFreeSpaceBytes( EMediaModule mediaModule );
    float                       toGuiGetAudioCacheMaxSeconds( EMediaModule mediaModule );

    void                        toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput );
    void                        toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput );

    void				        toGuiUpdateWantMicrophoneCount( int wantMicCnt );
    void				        toGuiUpdateWantSpeakerCount( int wantSpeakerCnt );

    void                        wantAudioOutSpaceAvailableCallback( AudioCallbackSpaceAvailable* callback, bool want );

    // NLC player related functions
    void				        setPlayerNlcActive( bool isActive );
    bool						getPlayerNlcActive( void );
    void                        setPlayerNlcAcceptInput( bool acceptInput );
    bool                        getPlayerNlcAcceptInput( void );
    void                        clearPlayerNlcBuffers( void );

    // Visualization related functions
    void                        wantAudioInVisualization( bool wanted );
    void                        wantAudioOutVisualization( bool wanted );

    AudioFrameAecBuffer&        getAudioInRawWaveformBuffer( void );
    AudioFrameAecBuffer&        getAudioAecProcessedWaveformBuffer( void );
    AudioFrameAecBuffer&        getSpeakerOutWaveformBuffer( void );

    // Audio Device related functions
    std::vector<std::string>&	getAudioInDevices( void );
	std::vector<std::string>&   getAudioOutDevices( void );

    bool                        setAudioInDevice( std::string deviceDescription );
    bool                        setAudioOutDevice( std::string deviceDescription );

    // Microphone level callback related functions
    int                         getAudioInPeakAmplitude( void );
    void                        wantMicrophoneLevelCallbacks( GuiAudioLevelCallback* client, bool enable );

    // Configure
    void                        setNoAecLoopbackEnable( bool enable ); 
    void                        setWithAecLoopbackEnable( bool enable ); 
    void                        setAgcEnabled( bool enable );
    void                        setNoiseSuppressionEnabled( bool enable );

    // Audio test related functions
    void                        wantAudioDelayTestCallbacks( AudioDelayTestCallback* client, bool enable ); // optional
    bool                        runEchoDelayTest( void );
    void                        setEchoDelayParam( int delayMs );

    void                        setEnableSpeakerTestTone( int enableTestTone );
    void                        playTestFile( std::string testFile );  

signals:
    void                        signalUpdateWantMicrophoneCount( int wantMicCnt );
    void                        signalUpdateSpeakerOutputCount( int wantSpeakerCnt );
    
protected slots:
    void                        slotAudioPeekTimeout( void );

    void                        slotUpdateWantMicrophoneCount( int wantMicCnt );
    void                        slotUpdateWantSpeakerCount( int wantSpeakerCnt );

protected:

	void 				        callbackToGuiWantMicrophoneRecording( bool wantMicInput ) override {};
	void 				        callbackToGuiWantSpeakerOutput( bool wantSpeakerOutput ) override {};


    AppCommon&                  m_MyApp;
    AudioMgr&                   m_AudioMgr;

    // microphone input peak amplitude for visualization
    QTimer*                     m_AudioLevelPeekTimer{ nullptr };
    std::vector<GuiAudioLevelCallback*> m_AudioLevelClientList;

};