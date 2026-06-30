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

#include "AudioDefs.h"

#include <CoreLib/VxAudioFormat.h>

#include "MiniAudioInDevice.h"

class AppCommon;
class AudioMgr;
class P2PEngine;

class MiniAudioIn : public MiniAudioInDevice
{
public:
    explicit MiniAudioIn( AudioMgr& mgr );
    ~MiniAudioIn() = default;

    bool                        initAudioIn( VxAudioFormat& audioFormat, int deviceIndex );
    bool                        soundInDeviceChanged( int deviceIndex );

    void						audioInShutdown( void )             { stopAudioInHardware(); }

    bool                        startAudioInHardware( void );
    void                        stopAudioInHardware( void );

    void                        wantMicrophoneInputHardware( bool enableInput );
    bool                        isMicrophoneInputWanted( void )     { return m_MicInputEnabled; }

    void                        setMicrophoneVolume( float volume ) { m_MicrophoneVolume = volume; }
 
    void                        setMicrophoneVolume( int volume0to100 );

    char *                      getMicSilence()                     { return m_MicSilence; }

    void                        setDivideSamplesCount( int cnt )    { m_DivideCnt = cnt; }
    int                         getDivideSamplesCount( void )       { return m_DivideCnt; }

    int                         getMicWriteDurationUs( void )       { return m_MicWriteDurationUs; }

    int                         getHardwareDelayMs( void );

protected:

    virtual int				    callbackAudioWrite( int16_t* pcmData, int lenBytes ) override;

    bool                        m_initialized{ false };

    bool                        m_WantAudioIn{ false };
    bool                        m_AudioInDeviceIsStarted{ false };
    VxAudioFormat               m_AudioFormat;

    char                        m_MicSilence[ AUDIO_BUF_SIZE ];
    int                         m_DivideCnt{ 1 };
    float                       m_MicrophoneVolume{ 100.0f };

    int                         m_MicWriteDurationUs{ 0 };

    bool                        m_MicInputEnabled{ false }; 
};
