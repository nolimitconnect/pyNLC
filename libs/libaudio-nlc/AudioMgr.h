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

#if defined(USE_AEC2) && USE_AEC2
#include <libwebrtc-aec2/apm/WebRtcAec.h>
#else
#include <libwebrtc-aec/apm/WebRtcAec.h>
#endif

#include <librnnoise/RNNoise.h>

#include "AudioDelayTestCallback.h"
#include "AudioMixerMgr.h"

#include "miniaudio/AudioFrameAecBuffer.h"
#include "miniaudio/AudioTestToneGenerator.h"

#include "miniaudio/HighPassFilter.h"
#include "miniaudio/LowPassFilter.h"

#include "miniaudio/MiniAudioDevices.h"
#include "miniaudio/MiniAudioIn.h"
#include "miniaudio/MiniAudioOut.h"

#include "TestFileWav.h"

#include <CoreLib/AudioCallbackSpaceAvailable.h>
#include <CoreLib/VxMutex.h>
#include <CoreLib/VxSemaphore.h>
#include <CoreLib/VxTimedCallback.h>

#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

class TestFileWavMgr;

class AudioMgr : public MiniAudioDevices, public AudioMixerMgr
{
public:
    static const int AEC_FRAME_COUNT_PER_OPUS_FRAME = 6; // opus consumes 60 ms frames
    static const int MAX_SPEAKER_OUT_BUFFER_SAMPLES = ECHO_FRAME_SIZE_10MS * 50;

    AudioMgr();
    ~AudioMgr();

    static AudioMgr&            getInstance( void )                         { static AudioMgr instance; return instance; }

    void                        audioIoSystemStartup( void );
    void                        audioIoSystemShutdown( void );

    bool                        isAudioInitialized( void )                  { return m_AudioIoInitialized;  }

    AudioFrameAecBuffer&        getAudioInRawWaveformBuffer( void )         { return m_AudioInRawWaveformBuffer; }
    AudioFrameAecBuffer&        getAudioAecProcessedWaveformBuffer( void )  { return m_AudioAecProcessedWaveformBuffer; }
    AudioFrameAecBuffer&        getSpeakerOutWaveformBuffer( void )         { return m_SpeakerOutWaveformBuffer; }

    MiniAudioIn&                getAudioInIo( void )                        { return m_AudioInIo; }
    MiniAudioOut&               getAudioOutIo( void )                       { return m_AudioOutIo; }

    void                        setIsMicrophoneMuted( bool muteMic )        { m_MicrophoneMuted = muteMic; };
    bool                        getIsMicrophoneMuted( void )                { return m_MicrophoneMuted; };

    void                        setIsSpeakerMuted( bool muteSpeaker )       { m_SpeakersMuted = muteSpeaker; };
    bool                        getIsSpeakerMuted( void )                   { return m_SpeakersMuted; };

    void                        setNoAecLoopbackEnable( bool enable )       { m_NoAecLoopbackEnabled = enable; }
    bool                        getNoAecLoopbackEnable( void )              { return m_NoAecLoopbackEnabled; };

    void                        setWithAecLoopbackEnable( bool enable )       { m_WithAecLoopbackEnabled = enable; }
    bool                        getWithAecLoopbackEnable( void )              { return m_WithAecLoopbackEnabled; };

    bool                        setAudioInDevice( std::string deviceDescription ); // finds device by description and sets it as the audio input device
    bool                        setAudioOutDevice( std::string deviceDescription ); // finds device by description and sets it as the audio output device

    bool                        setSoundInDeviceIndex( int sndInDeviceIndex ) { m_SndInDeviceIndex = sndInDeviceIndex; updateHardwareTotalLatencyMs(); return true; };
    bool                        getSoundInDeviceIndex( int& retDeviceIndex ){ retDeviceIndex = m_SndInDeviceIndex; return true; };

    bool                        setSoundOutDeviceIndex( int sndOutDeviceIndex ) { m_SndOutDeviceIndex = sndOutDeviceIndex; updateHardwareTotalLatencyMs(); return true; };
    bool                        getSoundOutDeviceIndex( int& retDeviceIndex ){ retDeviceIndex = m_SndOutDeviceIndex; return true; };

    bool                        soundInDeviceChanged( int deviceIndex )     { m_SndInDeviceIndex = deviceIndex; updateHardwareTotalLatencyMs(); return true; };
    bool                        soundOutDeviceChanged( int deviceIndex )    { m_SndOutDeviceIndex = deviceIndex; updateHardwareTotalLatencyMs(); return true; };

    void                        setIsMicrophoneWanted( bool wanted ){ m_MicrophoneWanted = wanted; };
    bool                        getIsMicrophoneWanted( void ) { return m_MicrophoneWanted; }

    void                        setIsMicrophoneRunning( bool running ){ m_MicrophoneRunning = running; };
    bool                        getIsMicrophoneRunning( void ) { return m_MicrophoneRunning; }

    void                        setIsSpeakerWanted( bool wanted ){ m_SpeakerWanted = wanted; };
    bool                        getIsSpeakerWanted( void ) { return m_SpeakerWanted; }

    void                        setIsSpeakerRunning( bool running ){ m_SpeakerRunning = running; };
    bool                        getIsSpeakerRunning( void ) { return m_SpeakerRunning; }

    void                        setIsPlayingTestFile( bool isPlaying ){ m_IsPlayingTestFile = isPlaying; };
    bool                        getIsPlayingTestFile( void ) { return m_IsPlayingTestFile; }

    void                        setNeedAudioOutDeviceStop( bool enabled ){ };
    bool                        speakerDeviceEnabled( bool enabled ){ return true; };

    void                        callbackAudioDeviceWrite( int16_t* pcmData, int sampleCnt );
    void                        callbackReadSpeakerData( int16_t* pcmData, int sampleCnt );

    void                        callbackAecProcessedAudio( int16_t* pcmData, int sampleCnt );

    void                        sendToSpeakerOutput( int16_t* pcmData, int sampleCnt );

    // for title bar
	int							getWantMicrophoneCount( void );
	int							getWantSpeakerCount( void );
 
    //=== IAudioRequests begin ===//
    // return true if any microphone device is available to be enabled
    bool				        toGuiIsMicrophoneDeviceAvailable( void ) override;
    // enable disable microphone data callback
    void				        toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput ) override;
    void                        onMicrophonePermissionResult( bool granted );
    // enable disable sound out
    void				        toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput ) override;
    //=== IAudioRequests end ===//
    void                        wantAudioOutSpaceAvailableCallback( AudioCallbackSpaceAvailable* callback, bool want );

	// Test Tone functions
    void                        setEnableSpeakerTestTone( int enableTestTone )  { m_SpeakerTestToneEnable = enableTestTone; }
    bool                        getEnableSpeakerTestTone( void )                { return m_SpeakerTestToneEnable; }
    void                        readTestToneSamples( int16_t* pcmData, int sampleCnt );

    // Audio delay test related functions
    bool                        runEchoDelayTest( void );
    void                        setEchoDelayParam( int delayMs );
    int                         getEchoDelayParam( void )                   { return m_EchoDelayMs; }

    int                         getEchoHardwareTotalLatencyMs( void )       { return m_EchoHardwareTotalLatencyMs; }
    int                         getSpeakerHardwareLatencyMs( void )         { return m_SpeakerHardwareLatencyMs; }

    void                        setAudioTestState( EAudioTestState audioTestState ); // public so can be called by UI
    EAudioTestState             getAudioTestState( void ) { return m_AudioTestState; }

    // peak microphone input level for visualization
    void                        setEnablePeekMicAmplitude( bool enable )    { m_EnableAudioInPeakAmplitude = enable; }
    void                        setAudioInPeakAmplitude( int peakValue )    { m_AudioInPeakAmplitude = peakValue; }
    int                         getAudioInPeakAmplitude( void )             { return m_AudioInPeakAmplitude; }  // get peak value 0 - 100

	// Audio waveform visualization
    void                        wantAudioInVisualization( bool wanted ) { if(wanted)m_VisualizeInWanted++; else if(m_VisualizeInWanted > 0) m_VisualizeInWanted--; };
    void                        wantAudioOutVisualization( bool wanted ) { if(wanted)m_VisualizeOutWanted++; else if(m_VisualizeOutWanted > 0) m_VisualizeOutWanted--; };

    bool                        getIsAudioInVisualizationWanted( void ) { return m_VisualizeInWanted > 0; }
    bool                        getIsAudioOutVisualizationWanted( void ) { return m_VisualizeOutWanted > 0; }

	// test file playback
    void                        playTestFile( std::string testFile );  
    void                        playTestFile( TestFileWav& testFile );  

	// Echo Cancel options
    void                        setAgcEnabled( bool enabled )             { m_Aec.setAgcEnabled( enabled ); }
    bool                        getAgcEnabled( void )                     { return m_Aec.isAgcEnabled(); }

    void                        setNoiseSuppressionEnabled( bool enabled ) { m_EnableNoiseSuppression = enabled; }
    bool                        getNoiseSuppressionEnabled( void )         { return m_EnableNoiseSuppression; }

    void                        wantAudioDelayTestCallbacks( AudioDelayTestCallback* client, bool enable ); // optional


protected:
	bool                        playFromTestFile( void );

    void                        startAudioInWorker( void );
    void                        stopAudioInWorker( void );
    void                        audioInWorkerLoop( void );
    void                        processQueuedAudioInput( int16_t* pcmData, int sampleCnt );
    void                        applyMicInLimiters( int16_t* pcmData, int sampleCnt );

    virtual void				callbackAudioIn60msFrameAvail( const int16_t* pcmData, int sampleCnt );

    void                        startAudioOutWorker( void );
    void                        stopAudioOutWorker( void );
    void                        audioOutWorkerLoop( void );
    void                        processQueuedAudioOutput( const int16_t* pcmData, int sampleCnt );
    void                        processAudioOutput10msFrame( const int16_t* pcmData, int sampleCnt );


    // Audio delay test related functions
    void                        setAudioTestSentTime( int64_t sentTime )        { m_AudioTestSentTimeMs = sentTime; }
    int64_t                     getAudioTestSentTime( void )                    { return m_AudioTestSentTimeMs; }
    void                        audioTestDetectTestSound( const int16_t* sampleInData, int inSampleCnt );
    int64_t                     getAudioTestDetectTime( int& peakValue );
    bool                        handleAudioTestResult( int64_t soundOutTimeMs, int64_t soundDetectTimeMs, int peakVal0to100 );

    void                        resetMicrophoneBuffers( void );
    void                        resetSpeakerBuffers( void );

    int                         updateHardwareTotalLatencyMs( void );

    int                         getAudioOutPipelineQueuedSampleCnt( void );
    void                        requestDeferredAudioOutDisable( void );
    void                        cancelDeferredAudioOutDisable( void );
    void                        completeAudioOutDisable( void );

    // internal
    void                        writeMixerAudioToSpeakerHardware( int16_t* pcmData, int sampleCount ) override;
    int                         getSpeakerHardwareBufferedSampleCnt( void ) override;
    int                         getSpeakerHardwareFreeSpaceSampleCnt( void ) override;
    void                        callbackAudioOut60msSpaceAvail( int freeSpaceLenBytes ) override;
    bool                        isModuleOutputWanted( EMediaModule mediaModule ) override;

    void                        callbackAudioTestTimer( void );
    void                        callbackAudioOutDisablePoll( void );

    void                        enableAudioIn( bool enable );   
    void                        enableAudioOut( bool enable );

    void                        sendAudioDelayTestFinished( void );

	//=== variables ===//	
    bool                        m_AudioIoInitialized{false};

    bool                        m_MicrophoneMuted{false};
    bool                        m_SpeakersMuted{false};

    bool                        m_NoAecLoopbackEnabled{false};
    bool                        m_WithAecLoopbackEnabled{false};

    bool                        m_MicrophoneWanted{false};
    bool                        m_SpeakerWanted{false};

    bool                        m_MicrophoneRunning{false};
    bool                        m_SpeakerRunning{false};

    VxAudioFormat               m_AudioOutFormat;
    MiniAudioOut                m_AudioOutIo;

    VxAudioFormat               m_AudioInFormat;
    MiniAudioIn                 m_AudioInIo;

    int                         m_SndOutDeviceIndex{0};
    int                         m_SndInDeviceIndex{0};

    WebRtcAec                   m_Aec;

    std::vector<std::vector<int16_t>> m_PendingInBuffers;
	std::mutex                  m_PendingInBuffersMutex;

	std::deque<int16_t>         m_ResidualInBuffer;  // deque for O(1) front erasure vs vector O(N)
    std::mutex                  m_ResidualInBufferMutex;

    std::thread                 m_AudioInWorkerThread;
    VxSemaphore                 m_AudioInWorkSemaphore;
    std::atomic<bool>           m_AudioInWorkerRunning{false};
    std::atomic<bool>           m_AudioInWorkerStopping{false};

    bool                        m_EnableAudioInPeakAmplitude{false};
    std::atomic<int>            m_AudioInPeakAmplitude{0};
    int                         m_AccumPeekAudioInAmplitude{0};
    bool                        m_CollectPeekAudioInAmplitude{false};

    // keep track of how many frames we've processed and call callbackAudioIn60msFrameAvail when enough audio for an opus frame is available
    int                         m_AudioInAecFramesProcessed{0};
    std::vector<int16_t>        m_OpusFrameBuffer;

    // VAD Probability: Map the 0.0–1.0 probability from WebRTC's VAD to a QProgressBar (0–100 range).
    float                       m_CurrentVadProb{0}; // probability user is not speaking
    // Echo Return Loss (ERL): Get this from WebRTC APM stats. If ERL is high, the AEC is working hard.
    float                       m_CurrentErl{0};
    // Input Level: Calculate the RMS (Root Mean Square) of the 10ms buffer.
    float                       m_CurrentRms{0};

    // audio callback list
    std::vector<AudioCallbackSpaceAvailable*> m_AudioOutSpaceAvailableClientList;
    std::mutex                  m_AudioOutSpaceAvailableClientListMutex;

    // input debugging buffers and stats
    bool                        m_MicJitterStatsEnable{ false };
    int64_t                     m_MicInJitterLowMarkMs{0};
    int64_t                     m_MicInJitterHighMarkMs{0};
    int                         m_LastMicStatsLogMs{0};

    bool                        m_AudioInPerfStatsEnable{ false }; // set to true to enable logging of AEC and RNNoise processing time for input audio frames
    double                      m_AecCaptureTotalMs{0.0};
    double                      m_RNNoiseTotalMs{0.0};
    int64_t                     m_AudioInPerfWindowStartMs{0};

    std::vector<int16_t>        m_SpeakerOutBuffer;
    std::mutex                  m_SpeakerOutBufferMutex;
    
    std::vector<std::vector<int16_t>> m_PendingOutBuffers;
	std::mutex                  m_PendingOutBuffersMutex;

	std::deque<int16_t>         m_ResidualOutBuffer; // deque for O(1) front erasure; only m_AudioOutWorkerThread accesses this

    std::thread                 m_AudioOutWorkerThread;
    VxSemaphore                 m_AudioOutWorkSemaphore;
    std::atomic<bool>           m_AudioOutWorkerRunning{false};
    std::atomic<bool>           m_AudioOutWorkerStopping{false};

    // keep track of how many frames we've processed and call callbackAudioOut60msSpaceAvail when space for opus frame is available
    int                         m_AudioOutAecFramesProcessed{0};

    // Speaker output stats for debugging and optimization
    std::atomic<uint64_t>       m_SpeakerUnderflowCount{0};
    std::atomic<uint64_t>       m_SpeakerOverflowCount{0};
    std::atomic<uint64_t>       m_SpeakerRequestedSamples{0};
    std::atomic<uint64_t>       m_SpeakerCopiedSamples{0};
    std::atomic<uint64_t>       m_SpeakerQueueHighWatermark{0};
    int                         m_LastSpeakerStatsLogMs{0};
    bool                        m_SpeakerIssueReported{ false };

    bool                        m_SpeakerJitterStatsEnable{ false };
    int64_t                     m_SpeakerOutJitterLowMarkMs{0};
    int64_t                     m_SpeakerOutJitterHighMarkMs{0};

    // For testing and debugging purposes, ability to play a test WAV file instead of live microphone input
    TestFileWav                 m_TestFile;
    int                         m_TestFileIndex{ 0 };
    bool                        m_IsPlayingTestFile{ false };

    // Visualization reference counting to allow multiple parts of the app to request visualization without interfering with each other
    int                         m_VisualizeInWanted{0};
    int                         m_VisualizeOutWanted{0};
    
    AudioFrameAecBuffer         m_AudioInRawWaveformBuffer;
    AudioFrameAecBuffer         m_AudioAecProcessedWaveformBuffer;
    AudioFrameAecBuffer         m_SpeakerOutWaveformBuffer;

    // Audio delay test state and data
    VxTimedCallback             m_AudioTestTimer;
    EAudioTestState             m_AudioTestState{ eAudioTestStateNone };
    std::vector<std::pair<int64_t, int>> m_DelayTestDetectList; // pairs of time and peak value detected
    bool                        m_DelayTestValid{ false };
    std::vector<int>            m_EchoDelayResultList;
    int                         m_EchoDelayMs{ 0 };

    int64_t                     m_AudioTestSentTimeMs{0};
    bool                        m_AudioTestMicEnable{ false };
    bool                        m_AudioTestSpeakerEnable{ false };
    int                         m_EchoDelayTestMaxInterations{ 3 };
    int                         m_EchoDelayCurrentInteration{ 0 }; 
    
    int                         m_EchoHardwareTotalLatencyMs{ 20 };
    int                         m_SpeakerHardwareLatencyMs{ 10 };

    // test tone variables
    bool                        m_SpeakerTestToneEnable{ false };
    AudioTestToneGenerator      m_ToneGenerator;

    // keep track of which modules want microphone and speaker output 
    std::vector<EMediaModule>   m_WantSpeakerList;
    VxMutex                     m_WantSpeakerMutex;
    std::atomic<int>            m_WantSpeakerCnt{0};

    std::vector<EMediaModule>   m_WantMicList;
    VxMutex                     m_WantMicMutex;
    std::atomic<int>            m_WantMicCnt{0};

    std::atomic<bool>           m_EnableAudioIn{0};
    std::atomic<bool>           m_EnableAudioOut{0};

    // Delay stopping audio-out until queued data has drained to hardware.
    VxTimedCallback             m_AudioOutDisableTimer;
    std::atomic<bool>           m_AudioOutDisablePending{false};
    std::atomic<bool>           m_PlayerNlcSpeakerDisablePending{false};

    RNNoise                     m_RNNoise;
    std::atomic<bool>           m_EnableNoiseSuppression{0};

    LowPassFilter               m_MicInLowPassFilter;

    TestFileWavMgr&             m_TestFileWavMgr;

    AudioDelayTestCallback*     m_AudioDelayTestCallback{ nullptr };
};

