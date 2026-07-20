//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioUtils.h"
#include "AudioDefs.h"

#include <CoreLib/VxAudioFormat.h>

#include <CoreLib/IsBigEndianCpu.h>
#include <CoreLib/VxDebug.h>

#include <cmath>
#include <memory.h>

//=============================================================================
int64_t AudioUtils::audioDurationUs( const VxAudioFormat& format, int64_t bytes )
{
    return (bytes * 1000000) /
        (format.sampleRate() * format.channelCount() * format.bytesPerSample());
}

//=============================================================================
int AudioUtils::audioDurationMs( const VxAudioFormat& format, int bytes )
{
    return (bytes * 1000) /
        (format.sampleRate() * format.channelCount() * format.bytesPerSample());
}


//=============================================================================
int AudioUtils::audioDurationMs( int sampleRate, int sampleCnt )
{
    return (sampleCnt * 1000) / sampleRate;
}

//=============================================================================
int64_t AudioUtils::audioDurationUs( int sampleRate, int sampleCnt )
{
    return (sampleCnt * 1000000) / sampleRate;
}

//=============================================================================
int64_t AudioUtils::audioLength(const VxAudioFormat &format, int64_t microSeconds)
{
    int64_t result = (format.sampleRate() * format.channelCount() * format.bytesPerSample())
        * microSeconds / 1000000;
    result -= result % (format.channelCount() * format.bytesPerSample() * 8);
    return result;
}

//=============================================================================
int AudioUtils::audioSamplesRequiredForGivenMs( const VxAudioFormat& format, int64_t milliSeconds )
{
    return (int)((format.sampleRate() * format.channelCount()) * milliSeconds / 1000);
}

//=============================================================================
int AudioUtils::audioSamplesRequiredForGivenMs( int sampleRate, int milliSeconds )
{
    return sampleRate * milliSeconds / 1000;
}

//=============================================================================
double AudioUtils::nyquistFrequency(const VxAudioFormat &format)
{
    return format.sampleRate() / 2;
}

//=============================================================================
bool AudioUtils::isPCM(const VxAudioFormat &format)
{
    return format.sampleFormat() == VxAudioFormat::Int16;
}

//=============================================================================
bool AudioUtils::isPCMS16LE(const VxAudioFormat &format)
{
    return isPCM(format) && !IsBigEndianCpu();
}

const int16_t  PCMS16MaxValue     =  32767;
const uint16_t PCMS16MaxAmplitude =  32768; // because minimum is -32768

//=============================================================================
double AudioUtils::pcmToReal(int16_t pcm)
{
    return double(pcm) / PCMS16MaxAmplitude;
}

//=============================================================================
float AudioUtils::pcmToFloat( int16_t pcm )
{
    return float(pcm) / PCMS16MaxAmplitude;
}

//=============================================================================
int16_t AudioUtils::realToPcm(double real)
{
    return real * PCMS16MaxValue;
}

//=============================================================================
int16_t AudioUtils::floatToPcm( float val )
{
    return val * PCMS16MaxValue;
}

//=============================================================================
static int16_t MixPcmSample( int a, int b ) // int16_t sample1, int16_t sample2 ) 
{
    // from stack overflow

#if 0
    // averaging algorithum
    const int32_t result( ( static_cast<int32_t>( sample1 ) + static_cast<int32_t>( sample2 ) ) / 2 );
    typedef std::numeric_limits<int16_t> Range;
    if( Range::max() < result )
        return Range::max();
    else if( Range::min() > result )
        return Range::min();
    else
        return result;
#else
    int m; // mixed result will go here
    // Make both samples unsigned (0..65535)
    a += 32768;
    b += 32768;

    // Pick the equation
    if( ( a < 32768 ) || ( b < 32768 ) ) {
        // Viktor's first equation when both sources are "quiet"
        // (i.e. less than middle of the dynamic range)
        m = a * b / 32768;
    }
    else {
        // Viktor's second equation when one or both sources are loud
        m = 2 * ( a + b ) - ( a * b ) / 32768 - 65536;
    }

    // Output is unsigned (0..65536) so convert back to signed (-32768..32767)
    if( m == 65536 ) m = 65535;
    m -= 32768;

    return (int16_t)m;
#endif // 0
}

//=============================================================================
void AudioUtils::mixPcmAudio( int16_t * pcmData, int16_t * outData, int toMixBytes )
{
    int sampleCnt = toMixBytes / 2;
    if( sampleCnt )
    {
        for( int i = 0; i < sampleCnt; i++ )
        {
            outData[i] = MixPcmSample( pcmData[ i ], outData[ i ] );
        }
    }
}

//=============================================================================
void AudioUtils::upsamplePcmAudioLerpPrev( int16_t* srcSamples, int srcSampleCnt, int upResampleMultiplier, int16_t prevFrameSample, int16_t* destSamples )
{
    int16_t firstSample = prevFrameSample;
    int16_t secondSample;
    float sampleUpMult = (float)upResampleMultiplier;
    float sampleStep;
    int iDestIdx = 0;
    for( int i = 0; i < srcSampleCnt; i++ )
    {
        secondSample = srcSamples[ i ];
        if( secondSample >= firstSample )
        {
            // ramp up
            sampleStep = ((secondSample - firstSample) / sampleUpMult);
        }
        else
        {
            // ramp down
            sampleStep = -((firstSample - secondSample) / sampleUpMult);
        }

        if( 0.0f == sampleStep )
        {
            for( int j = 0; j < upResampleMultiplier; ++j )
            {
                destSamples[ iDestIdx ] = firstSample;
                iDestIdx++;
            }
        }
        else
        {
            float sampleOffs = sampleStep;
            for( int j = 0; j < upResampleMultiplier; ++j )
            {
                destSamples[ iDestIdx ] = (int16_t)(firstSample + sampleOffs);
                iDestIdx++;
                sampleOffs += sampleStep;
            }
        }

        firstSample = secondSample;
    }
}

//=============================================================================
void AudioUtils::upsamplePcmAudioLerpNext( int16_t* srcSamples, int srcSampleCnt, int upResampleMultiplier, int16_t nextFrameSample, int16_t* destSamples )
{
    int16_t firstSample = srcSamples[ 0 ];
    int16_t secondSample;
    float sampleUpMult = (float)upResampleMultiplier;
    float sampleStep;
    int iDestIdx = 0;
    for( int i = 1; i <= srcSampleCnt; i++ )
    {
        secondSample = i == srcSampleCnt ? nextFrameSample : srcSamples[ i ];

        if( secondSample >= firstSample )
        {
            // ramp up
            sampleStep = ((secondSample - firstSample) / sampleUpMult);
        }
        else
        {
            // ramp down
            sampleStep = -((firstSample - secondSample) / sampleUpMult);
        }

        if( 0.0f == sampleStep )
        {
            for( int j = 0; j < upResampleMultiplier; ++j )
            {
                destSamples[ iDestIdx ] = firstSample;
                iDestIdx++;
            }
        }
        else
        {
            float sampleOffs = sampleStep;
            for( int j = 0; j < upResampleMultiplier; ++j )
            {
                destSamples[ iDestIdx ] = (int16_t)(firstSample + sampleOffs);
                iDestIdx++;
                sampleOffs += sampleStep;
            }
        }

        firstSample = secondSample;
    }
}

//=============================================================================
void AudioUtils::dnsamplePcmAudio( int16_t* srcSamples, int resampledCnt, int dnResampleDivider, int16_t* destSamples )
{
    if( dnResampleDivider > 1 )
    {
        for( int i = 0; i < resampledCnt; i++ )
        {
            destSamples[ i ] = srcSamples[ i * dnResampleDivider ];
        }
    }
    else
    {
        memcpy( destSamples, srcSamples, resampledCnt * AUDIO_BYTES_PER_SAMPLE );
    }
}

//============================================================================
int AudioUtils::peakPcmAmplitude0to100( const int16_t* srcSamples, int sampleCnt )
{
    int peakValue{ 0 };
    for( int i = 0; i < sampleCnt; i++ )
    {
        if( srcSamples[ i ] > peakValue )
        {
            peakValue = srcSamples[ i ];
        }
    }

    // LogMsg( LOG_VERBOSE, "peakPcmAmplitude0to100 peak raw %d 0to100 %d", peakValue, (int)(((float)peakValue / 32768.0f) * 100) );

    if( peakValue )
    {
        return (int)(( (float)peakValue / 32768.0f ) * 100);
    }
    
    return 0;
}

//============================================================================
int AudioUtils::hasSomeSilence( int16_t* srcSamples, int datalen )
{
    int samples = datalen / 2;
    int lastSample = 0;
    int silenceSamples = 0;
    for( int i = 0; i < samples; i++ )
    {
        if( srcSamples[ i ] == 0  && lastSample == 0 )
        {
            silenceSamples++;
        }

        lastSample = srcSamples[ i ];
    }

    return silenceSamples > 4 ? silenceSamples : 0;
}


//============================================================================
int AudioUtils::countConsecutiveValues( int16_t* srcSamples, int datalen, int minConsecutiveToMatch )
{
    int samples = datalen / 2;
    int lastSample = srcSamples[ 0 ];
    int consecutiveInARow = 0;
    int consecutiveTotalCnt = 0;
    for( int i = 1; i < samples; i++ )
    {
        if( srcSamples[ i ] == lastSample )
        {
            consecutiveInARow++;
            if( consecutiveInARow >= minConsecutiveToMatch )
            {
                consecutiveTotalCnt++;
            }
        }
        else
        {
            consecutiveInARow = 0;
        }

        lastSample = srcSamples[ i ];
    }

    return consecutiveTotalCnt;
}


//============================================================================
// interperlate between samples for up sample audio to a higher sample rate
// example of lerped samples for 12 as upsample multiplier and sample 1 = 0 and sample 2 = 1200 
// (first number is index and second is the lerped value)
// 0 (0), 1 (100), 2 (200), 3 (300), 4 (400), 5 (500), 6 (600), 7 (700), 8 (800), 9 (900), 10 (1000), 11 (1100), 12 (1200)
int16_t AudioUtils::lerpPcm( int16_t samp1, int16_t samp2, float totalSteps, int thisLerpIdx )
{
    float sampleStep = samp1 >= samp2 ? ((samp2 - samp1) / totalSteps) : -((samp1 - samp2) / totalSteps);

    if( 0.0f == sampleStep )
    {
        return samp1;
    }
    else
    {
        return (int16_t)(samp1 + thisLerpIdx * sampleStep);
    }
}

