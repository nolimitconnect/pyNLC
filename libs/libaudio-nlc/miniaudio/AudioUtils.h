
#pragma once
//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <CoreLib/VxDebug.h>

#include <stdint.h>
#include <string>

class VxAudioFormat;

namespace AudioUtils
{
    int64_t                     audioDurationUs(const VxAudioFormat &format, int64_t bytes);
    int64_t                     audioDurationUs( int sampleRate, int sampleCnt );

    int                         audioDurationMs( const VxAudioFormat& format, int bytes );
    int                         audioDurationMs( int sampleRate, int sampleCnt );

    int64_t                     audioLength(const VxAudioFormat &format, int64_t microSeconds);
    int                         audioSamplesRequiredForGivenMs( const VxAudioFormat& format, int64_t milliSeconds );
    int                         audioSamplesRequiredForGivenMs( int sampleRate, int milliSeconds );

    double                      nyquistFrequency(const VxAudioFormat &format);

    // Scale PCM value to [-1.0, 1.0]
    double                      pcmToReal( int16_t pcm);
    float                       pcmToFloat( int16_t pcm );

    // Scale real value in [-1.0, 1.0] to PCM
    int16_t                     realToPcm( double real );
    int16_t                     floatToPcm( float val );

    // Check whether the audio format is PCM
    bool                        isPCM(const VxAudioFormat &format);

    // Check whether the audio format is signed, little-endian, 16-bit PCM
    bool                        isPCMS16LE(const VxAudioFormat &format);

    void                        mixPcmAudio( int16_t * pcmData,  int16_t * outData, int toMixBytes );

    //  contract pcm Audio data to disired number of samples using a divider.. typically PCM 48000Hz Stereo Channel to PCM 16000Hz Mono Channel 
    void                        dnsamplePcmAudio( int16_t* srcSamples, int resampledCnt, int dnResampleDivider, int16_t* destSamples );

    //  expand pcm Audio data to desired number of samples using a multiplier.. typically PCM 16000Hz Mono Channel to PCM 48000Hz Stereo Channel
    void                        upsamplePcmAudioLerpPrev( int16_t* srcSamples, int srcSampleCnt, int upResampleMultiplier, int16_t prevFrameSample, int16_t* destSamples );
    //  expand pcm Audio data to desired number of samples using a multiplier.. typically PCM 16000Hz Mono Channel to PCM 48000Hz Stereo Channel
    void                        upsamplePcmAudioLerpNext( int16_t* srcSamples, int srcSampleCnt, int upResampleMultiplier, int16_t nextFrameSample, int16_t* destSamples );

    // get peak amplitude of pcm audio (returns 0-100)
    int                         peakPcmAmplitude0to100( const int16_t* srcSamples, int sampleCnt );

    int                         hasSomeSilence( int16_t* srcSamples, int datalen );
    int                         countConsecutiveValues( int16_t* srcSamples, int datalen, int minConsecutiveToMatch );

    // interperlate between samples for up sample audio to a higher sample rate
    int16_t                     lerpPcm( int16_t samp1, int16_t samp2, float totalSteps, int thisLerpIdx );

} // namespace AudioUtils
