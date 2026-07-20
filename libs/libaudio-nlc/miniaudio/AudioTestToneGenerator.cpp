//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioTestToneGenerator.h"

#include "AudioDefs.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxMathDef.h>
#include <CoreLib/IsBigEndianCpu.h>

#include <cmath>
#include <memory.h>

//============================================================================
AudioTestToneGenerator::AudioTestToneGenerator( const VxAudioFormat& format, int64_t durationUs, int toneHz )
{
    m_AudioFormat = format;
    if( format.isValid() )
    {
        generateData( format, durationUs, toneHz );
    }
}

//============================================================================
void AudioTestToneGenerator::setAudioFormat( VxAudioFormat& audioFormat ) 
{ 
    m_AudioFormat = audioFormat; 
    if( m_AudioFormat.isValid() )
    {
        generateData( m_AudioFormat, AUDIO_MS_PER_FRAME * 2 * 1000, 200 );
    }
}

//============================================================================
void AudioTestToneGenerator::generateData( const VxAudioFormat &format, int64_t durationUs, int toneHz )
{
    int rate = format.sampleRate();
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.channelCount() * channelBytes;
    int64_t length = format.bytesForDuration( durationUs );
    vx_assert(length % sampleBytes == 0);
    vx_assert(length % static_cast<int64_t>( sizeof( int16_t ) ) == 0);

    // m_buffer stores int16_t elements; resize by element count, not byte count.
    m_buffer.resize( static_cast<size_t>( length / static_cast<int64_t>( sizeof( int16_t ) ) ) );
    unsigned char* ptr = reinterpret_cast<unsigned char*>(m_buffer.data());
    int sampleIndex = 0;

    while( length ) {
        // Produces value (-1..1)
        double phase = 2.0 * M_PI * toneHz * (double)(sampleIndex % rate) / (double)(rate);
        double x = std::sin(phase);
        if(0.0 == x){
            LogMsg( LOG_DEBUG, "AudioTestToneGenerator: x == 0 at idx %d", sampleIndex );
        }

        ++sampleIndex;
        for( int i = 0; i < format.channelCount(); ++i ) {
            switch( format.sampleFormat() ) {
            case VxAudioFormat::UInt8:
                *reinterpret_cast<int8_t*>(ptr) = static_cast<int8_t>((1.0 + x) / 2 * 255);
                break;
            case VxAudioFormat::Int16:
                *reinterpret_cast<int16_t*>(ptr) = static_cast<int16_t>(x * 32767);
                break;
            case VxAudioFormat::Int32:
                *reinterpret_cast<int32_t*>(ptr) = static_cast<int32_t>(x * std::numeric_limits<int32_t>::max());
                break;
            case VxAudioFormat::Float:
                *reinterpret_cast<float*>(ptr) = x;
                break;
            default:
                break;
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
    }
}

//============================================================================
int64_t AudioTestToneGenerator::readData( char *data, int64_t len )
{
    int64_t total = 0;
    if( !m_buffer.empty() ) 
    {
        const int64_t bufferBytes = static_cast<int64_t>( m_buffer.size() * sizeof( int16_t ) );
        while( len - total > 0 ) 
        {
            int64_t chunk = std::min( bufferBytes - m_pos, len - total );
            memcpy( data + total, reinterpret_cast<const char*>( m_buffer.data() ) + m_pos, static_cast<size_t>( chunk ) );
            m_pos = ( m_pos + chunk ) % bufferBytes;
            total += chunk;
        }
    }

    return total;
}

//============================================================================
int16_t AudioTestToneGenerator::peekNextSample( void )
{
    int16_t nextSample{ 0 };
    if( !m_buffer.empty() && m_AudioFormat.sampleFormat() == VxAudioFormat::Int16 ) 
    {
        const int64_t bufferBytes = static_cast<int64_t>( m_buffer.size() * sizeof( int16_t ) );
        if( m_pos <= bufferBytes - static_cast<int64_t>( sizeof( int16_t ) ) )
        {
            // read from current position
            nextSample = *reinterpret_cast<const int16_t*>( reinterpret_cast<const char*>( m_buffer.data() ) + m_pos );
        }
        else
        {
            // read from start position
            nextSample = *reinterpret_cast<const int16_t*>( m_buffer.data() );
        }
    }

    return nextSample;
}

//============================================================================
void AudioTestToneGenerator::readToneSamples( int16_t* pcmData, int sampleCnt )
{
    readData( (char*)pcmData, sampleCnt * AUDIO_BYTES_PER_SAMPLE );
}