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

#include <CoreLib/VxAudioFormat.h>

#include <vector>

class AudioTestToneGenerator
{

public:
    AudioTestToneGenerator() = default;
    AudioTestToneGenerator( const VxAudioFormat& format, int64_t durationUs, int toneHz );
    ~AudioTestToneGenerator() = default;

    void                        setAudioFormat( VxAudioFormat& audioFormat );
    VxAudioFormat               getAudioFormat( void )                      { return m_AudioFormat;  }
    bool                        isFormatSet( void )                         { return m_AudioFormat.isValid(); }

    void                        readToneSamples( int16_t* pcmData, int sampleCnt );

    int64_t                     size() const                                { return m_buffer.size(); }

    int16_t                     peekNextSample( void );

private:
    void                        generateData( const VxAudioFormat &format, int64_t durationUs, int toneHz );
    int64_t                     readData( char* data, int64_t len );

    int64_t                     m_pos = 0;
    std::vector<int16_t>        m_buffer;
    VxAudioFormat               m_AudioFormat;
};

