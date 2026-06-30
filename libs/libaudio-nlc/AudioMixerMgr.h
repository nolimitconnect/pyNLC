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

#include "miniaudio/AudioMixerBuf.h"
#include "miniaudio/AudioSampleBuf.h"

#include <GuiInterface/IAudioInterface.h>
#include <GuiInterface/IAudioDefs.h>

#include <vector>
#include <mutex>
#include <map>


class AudioMixerMgr : public IAudioRequests
{
public:
    const int PLAYER_MAX_QUEUE_SIZE = 80; // 80 x 60ms frames with one-frame headroom => effective max cache is 4.74s
    const int PLAYER_STARTUP_PRIME_FRAMES = 2; // prebuffer ~120ms so playback starts sooner while larger queue absorbs burst decode
    
    // return true if any microphone device is available to be enabled
    bool				        toGuiIsMicrophoneDeviceAvailable( void ) override{ return false; }; // this is handled by AudioMgr
    // enable disable microphone data callback
    void				        toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput ) override{}; // this is handled by AudioMgr
    // enable disable sound out
    void				        toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput ) override{};  // this is handled by AudioMgr

    // add audio data to play.. assumes pcm mono 16000 Hz of mixer buffer length
    int				            toGuiModuleAudioFrame( EMediaModule mediaModule, int16_t* pu16PcmData, int pcmDataLenInBytes ) override;
    // add audio data to play.. assumes 20ms of float 2 channel 48000 Hz
    int				            toGuiPlayerNlcAudio( EMediaModule mediaModule, float* audioDataFloat, int audioDataLenInBytes ) override;

    float                       toGuiGetAudioDelaySeconds( EMediaModule mediaModule ) override;

    float                       toGuiGetAudioCacheFreeSpaceBytes( EMediaModule mediaModule ) override;

    float                       toGuiGetAudioCacheMaxSeconds( EMediaModule mediaModule ) override;
    //=== IAudioRequests end ===//
    
    virtual void                writeMixerAudioToSpeakerHardware( int16_t* pcmData, int sampleCount ) = 0;
    virtual int                 getSpeakerHardwareBufferedSampleCnt( void ) = 0;
    virtual int                 getSpeakerHardwareFreeSpaceSampleCnt( void ) = 0;
    virtual bool                isModuleOutputWanted( EMediaModule mediaModule ) = 0;

    virtual void                callbackAudioOut60msSpaceAvail( int freeSpaceLenBytes );
    virtual void				fromGuiAudioOutSpaceAvaiThreaded( int freeSpaceLenBytes );

    float                       calculateMsOfSamples( int sampleCount );

	// player-nlc
  	virtual void				setPlayerNlcActive( bool isActive );
    bool						getPlayerNlcActive( void ) { return m_PlayerNlcActive; }
    void                        setPlayerNlcAcceptInput( bool acceptInput );
    bool                        getPlayerNlcAcceptInput( void ) { return m_PlayerNlcAcceptInput; }
    void                        clearPlayerNlcBuffers( void );

protected:
    void                        lockPlayerCache( void )                             { m_PlayerCacheMutex.lock(); }
    void                        unlockPlayerCache( void )                           { m_PlayerCacheMutex.unlock(); }

	// mixer
    void                        lockModuleMixerBuffer( void )                       { m_ModuleMixerMutex.lock(); }
    void                        unlockModuleMixerBuffer( void )                     { m_ModuleMixerMutex.unlock(); }
    AudioMixerBuf&              getAudioMixerBuf( EMediaModule mediaModule );
    void                        removeAudioMixerBuf( EMediaModule mediaModule );


    //=== variables ===//

    // player-nlc
    bool                        m_PlayerNlcActive{ false };
    bool                        m_PlayerNlcAcceptInput{ true };
    int                         m_PlayerNlcAcceptInputDisableAfterMs{ 0 };
    bool                        m_PlayerNlcPrimed{ false };
    int                         m_PlayerNlcStreamStartMs{ 0 };
    AudioSampleBuf              m_PlayerCacheBuf;
    std::deque<std::vector<int16_t>> m_PlayerCacheQueue;
    std::mutex                  m_PlayerCacheMutex;

    // mixer
    std::map<EMediaModule, AudioMixerBuf> m_AppModuleToSpeakerMap;
    std::mutex                  m_ModuleMixerMutex;    
};