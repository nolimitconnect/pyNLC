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

#include "AudioSampleBuf.h"
#include "AudioDefs.h"

#include <CoreLib/VxAudioFormat.h>

#include "MiniAudioOutDevice.h"

class AppCommon;
class AudioMgr;
class QTimer;

class MiniAudioOut : public MiniAudioOutDevice
{
public:
    explicit MiniAudioOut( AudioMgr& mgr );
    ~MiniAudioOut() = default;

    bool                        initAudioOut( VxAudioFormat& audioFormat, int deviceIndex );
    bool                        soundOutDeviceChanged( int deviceIndex );

    void						audioOutShutdown( void ) { stopAudioOutHardware(); }

    bool                        startAudioOutHardware( void );
    void                        stopAudioOutHardware( void );

    void                        wantSpeakerOutputHardware( bool enableOutput );
    bool                        isSpeakerOutputWanted( void )                   { return m_SpeakerOutputEnabled; }

    void                        setUpsampleMultiplier( int upSampleMult )       { m_UpsampleMutiplier = upSampleMult; }
    int                         getUpsampleMultiplier( void )                   { return m_UpsampleMutiplier; }

    AudioSampleBuf&             getSpeakerEchoSamples( int64_t& tailOfSpeakerSamplestimeMs ) { tailOfSpeakerSamplestimeMs = m_EndOfEchoBufferTimestamp; return m_EchoFarBuffer; }

    virtual int				    callbackAudioRead( int16_t* pcmData, int maxlen ) override;

    int                         getHardwareDelayMs( void );

protected:
    bool                        m_initialized{ false };
    bool                        m_SpeakerOutputEnabled{ false };

    VxAudioFormat                m_AudioFormat;
    
    int64_t                      m_ProccessedMs = 0;

    int                         m_UpsampleMutiplier{ 0 };

    int16_t                     m_PrevLastsample{ 0 };
    int                         m_PrevLerpedSamplesCnt{ 0 };

    int64_t                     m_EndOfEchoBufferTimestamp{ 0 };
    AudioSampleBuf              m_EchoFarBuffer;

    bool                        m_AudioOutDeviceIsStarted{ false };
};
