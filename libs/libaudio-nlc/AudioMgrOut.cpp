//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioMgr.h"

#include "AudioDefs.h"
#include "AudioUtils.h"

#include <algorithm>

#include <GuiInterface/IToGui.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxTime.h>
#include <CoreLib/VxElapseTimer.h>

//============================================================================
void AudioMgr::callbackReadSpeakerData( int16_t* pcmData, int sampleCnt )
{
    if( !pcmData || sampleCnt <= 0 )
    {
        return;
    }

    if( m_AudioTestState != eAudioTestStateNone )
    {
        memset( pcmData, 0, sampleCnt * AUDIO_BYTES_PER_SAMPLE );
        if( m_AudioTestState == eAudioTestStateRun && !getAudioTestSentTime() )
        {
            int16_t* sampleBuf = (int16_t*)pcmData;
            // create a 480 hz square wave tone for 10 ms as a sound to be detected by microphone for delay timing test
            int maxSamplesToSet = AudioUtils::audioSamplesRequiredForGivenMs( m_AudioOutFormat, 10 );
            maxSamplesToSet = std::min( maxSamplesToSet, sampleCnt );
            int samplesCycle = (m_AudioOutFormat.sampleRate() * m_AudioOutFormat.channelCount()) / (480 * 2);
            bool sampleIsMax{ true };
            for( int i = 0; i < maxSamplesToSet; i += samplesCycle )
            {
                int16_t sampVal = sampleIsMax ? 32767 : -32768;
                for( int j = 0; j < samplesCycle && ((j + i) < maxSamplesToSet); j++ )
                {
                    sampleBuf[ i + j ] = sampVal;
                }

                sampleIsMax = !sampleIsMax;
            }

            setAudioTestSentTime( GetHighResolutionTimeMs() );
        }

        return;
    }

    // The m_SpeakerOutBuffer always contains a multiple of ECHO_FRAME_SIZE_10MS samples that are ready to be played back
    // MiniAudio will request samples and depending on the OS it might request in chunks that are not a multiple of ECHO_FRAME_SIZE_10MS, 
    // so we need to handle that case by copying only the available samples and zero filling the rest if necessary

    const size_t requestedSamples = static_cast<size_t>( sampleCnt );
    size_t samplesToCopy = 0;

    {
        std::lock_guard<std::mutex> lk(m_SpeakerOutBufferMutex);
        const size_t availableSamples = m_SpeakerOutBuffer.size();
        samplesToCopy = std::min( availableSamples, requestedSamples );

        if( samplesToCopy > 0 )
        {
            std::copy_n( m_SpeakerOutBuffer.data(), samplesToCopy, pcmData );
            m_SpeakerOutBuffer.erase( m_SpeakerOutBuffer.begin(), m_SpeakerOutBuffer.begin() + static_cast<std::vector<int16_t>::difference_type>(samplesToCopy) );
        }
    }

    if( samplesToCopy < requestedSamples )
    {
        m_SpeakerUnderflowCount++;
        std::fill( pcmData + samplesToCopy, pcmData + sampleCnt, 0 );
    }

    m_SpeakerCopiedSamples += static_cast<uint64_t>( samplesToCopy );
    m_SpeakerRequestedSamples += static_cast<uint64_t>( requestedSamples );

    if( getEnableSpeakerTestTone() )
    {
        readTestToneSamples( pcmData, sampleCnt );
    }

    if( m_SpeakersMuted )
    {
        // If muted, zero out the entire buffer to prevent any sound from being heard in the speakers
        std::fill_n( pcmData, sampleCnt, 0 );
    }

    // Must hold at least AEC_FRAME_COUNT_PER_OPUS_FRAME (6) × 10ms frames plus headroom for
    // Android scheduler latency (~33ms observed). 10 buffers = 100ms prevents overflow drops
    // that stall the AEC frame counter and delay callbackAudioOut60msSpaceAvail refills.
    constexpr size_t MAX_PENDING_AUDIO_OUT_BUFFERS = 10;

    {
        std::lock_guard<std::mutex> lk( m_PendingOutBuffersMutex );
        if( m_PendingOutBuffers.size() >= MAX_PENDING_AUDIO_OUT_BUFFERS )
        {
            m_SpeakerOverflowCount++;
            m_PendingOutBuffers.erase( m_PendingOutBuffers.begin() );
        }

        m_PendingOutBuffers.emplace_back( pcmData, pcmData + sampleCnt );
    }

    m_AudioOutWorkSemaphore.signal();

    const int nowMs = GetApplicationAliveMs();
    if( 0 == m_LastSpeakerStatsLogMs )
    {
        m_LastSpeakerStatsLogMs = nowMs;
    }
    else if( ( nowMs - m_LastSpeakerStatsLogMs ) >= 1000 )
    {
        m_LastSpeakerStatsLogMs = nowMs;
        const uint64_t underflows = m_SpeakerUnderflowCount.exchange( 0 );
        const uint64_t overflows = m_SpeakerOverflowCount.exchange( 0 );

        const uint64_t requested = m_SpeakerRequestedSamples.exchange( 0 );
        const uint64_t copied = m_SpeakerCopiedSamples.exchange( 0 );

        const uint64_t queueHighWater = m_SpeakerQueueHighWatermark.exchange( 0 );
        const uint64_t missing = ( requested > copied ) ? ( requested - copied ) : 0;
        const bool issuesDetected = ( underflows > 0 || overflows > 0 || missing > 0 );

        if( issuesDetected && !m_SpeakerIssueReported )
        {
            LogMsg( LOG_WARNING, "%s: Detected speaker issues! underflows=%llu overflows=%llu requested=%llu copied=%llu missing=%llu pendingQ_highwater=%llu",
                __func__,
                static_cast<unsigned long long>( underflows ),
                static_cast<unsigned long long>( overflows ),
                static_cast<unsigned long long>( requested ),
                static_cast<unsigned long long>( copied ),
                static_cast<unsigned long long>( missing ),
                static_cast<unsigned long long>( queueHighWater ) );

            m_SpeakerIssueReported = true;
        }
        else if( !issuesDetected )
        {
            m_SpeakerIssueReported = false;
        }

        if( m_SpeakerJitterStatsEnable )
        {
            if( m_SpeakerOutJitterHighMarkMs > 1 )
            {
                LogMsg( LOG_WARNING, "%s: Detected speaker jitter! low=%lld high=%lld",
                    __func__,
                    static_cast<unsigned long long>( m_SpeakerOutJitterLowMarkMs ),
                    static_cast<unsigned long long>( m_SpeakerOutJitterHighMarkMs ) );
                m_SpeakerOutJitterLowMarkMs = 100000000;
                m_SpeakerOutJitterHighMarkMs = 0;   

            }
        }
    }
}

//============================================================================
void AudioMgr::startAudioOutWorker( void )
{
    if( m_AudioOutWorkerRunning )
    {
        return;
    }

    m_AudioOutWorkerStopping = false;
    m_AudioOutWorkerThread = std::thread( &AudioMgr::audioOutWorkerLoop, this );
    m_AudioOutWorkerRunning = true;
}

//============================================================================
void AudioMgr::stopAudioOutWorker( void )
{
    m_AudioOutWorkerStopping = true;
    m_AudioOutWorkSemaphore.signal();

    if( m_AudioOutWorkerThread.joinable() )
    {
        m_AudioOutWorkerThread.join();
    }

    m_AudioOutWorkerRunning = false;

    {
        std::lock_guard<std::mutex> lk( m_PendingOutBuffersMutex );
        m_PendingOutBuffers.clear();
    }

    {
        std::lock_guard<std::mutex> lk( m_SpeakerOutBufferMutex );
        m_SpeakerOutBuffer.clear();
    }

    m_ResidualOutBuffer.clear();

    m_SpeakerUnderflowCount = 0;
    m_SpeakerRequestedSamples = 0;
    m_SpeakerCopiedSamples = 0;
    m_SpeakerQueueHighWatermark = 0;
    m_LastSpeakerStatsLogMs = 0;
    m_SpeakerIssueReported = false;
}

//============================================================================
void AudioMgr::audioOutWorkerLoop( void )
{
    while( true )
    {
        m_AudioOutWorkSemaphore.wait();

        std::vector<std::vector<int16_t>> pendingBuffers;
        {
            std::lock_guard<std::mutex> lk( m_PendingOutBuffersMutex );
            pendingBuffers.swap( m_PendingOutBuffers );
        }

        for( auto& buffer : pendingBuffers )
        {
            if( !buffer.empty() )
            {
                processQueuedAudioOutput( buffer.data(), static_cast<int>(buffer.size()) );
            }
        }

        if( m_AudioOutWorkerStopping )
        {
            std::lock_guard<std::mutex> lk( m_PendingOutBuffersMutex );
            if( m_PendingOutBuffers.empty() )
            {
                break;
            }
        }
    }
}

//============================================================================
void AudioMgr::processQueuedAudioOutput( const int16_t* pcmData, int sampleCnt )
{
    // Tell WebRTC AEC about the samples that were just played out so it can update its internal state 
    // and do better echo cancellation on subsequent frames
    static int lastSampleRate = 0;
    const int sampleRate = m_AudioOutFormat.sampleRate() ? m_AudioOutFormat.sampleRate()
                                                         : AUDIO_DEVICE_SAMPLE_RATE;
    if (sampleRate != lastSampleRate) {
        m_Aec.setStreamFormat(sampleRate, AUDIO_CHANNELS);
        lastSampleRate = sampleRate;
    }

    if( !pcmData || sampleCnt <= 0 )
    {
        LogMsg( LOG_ERROR, "%s: Invalid audio output buffer. pcmData=%p, sampleCnt=%d", __func__, (void*)pcmData, sampleCnt );
        return;
    }

    // 1. Append the new data to our residual buffer
    m_ResidualOutBuffer.insert( m_ResidualOutBuffer.end(), pcmData, pcmData + sampleCnt );

    // 2. Process as many 10ms frames as we can
    // Pre-allocate frame buffer once outside the loop to avoid per-iteration heap allocation.
    std::vector<int16_t> frame( ECHO_FRAME_SIZE_10MS );
    while( true )
    {
        if( m_ResidualOutBuffer.size() < ECHO_FRAME_SIZE_10MS )
            break;

        // m_ResidualOutBuffer is a std::deque: copy to contiguous frame buffer first,
        // then erase from front in O(N-erased) instead of O(N-total) as with std::vector.
        std::copy_n( m_ResidualOutBuffer.begin(), ECHO_FRAME_SIZE_10MS, frame.data() );
        m_ResidualOutBuffer.erase( m_ResidualOutBuffer.begin(), m_ResidualOutBuffer.begin() + ECHO_FRAME_SIZE_10MS );

        if( m_SpeakerJitterStatsEnable )
        {
            // callbacks to write mic data to aec should happen every 10ms
            static int64_t last10msFrameTime = 0;
            int64_t speaker10msFrameTime = GetHighResolutionTimeMs();
            if( last10msFrameTime != 0 )
            {
                const int64_t speaker10msFrameJitter = (speaker10msFrameTime - last10msFrameTime) - ECHO_MS_PER_FRAME;
                if(speaker10msFrameJitter != 0 )
                {
                    if( speaker10msFrameJitter < m_SpeakerOutJitterLowMarkMs )
                    {
                        m_SpeakerOutJitterLowMarkMs = speaker10msFrameJitter;
                    }
                    if( speaker10msFrameJitter > m_SpeakerOutJitterHighMarkMs )
                    {
                        m_SpeakerOutJitterHighMarkMs = speaker10msFrameJitter;
                    }
                }
            }
            
            last10msFrameTime = speaker10msFrameTime;
        }

        if( m_MicrophoneRunning )
        {
            m_Aec.processRender( frame.data(), ECHO_FRAME_SIZE_10MS );
        }

        if( m_VisualizeOutWanted )
        {
            m_SpeakerOutWaveformBuffer.pushFrame( frame.data(), ECHO_FRAME_SIZE_10MS, 0.0f, 0.0f, false );
        }

        m_AudioOutAecFramesProcessed++;
        if( m_AudioOutAecFramesProcessed >= AEC_FRAME_COUNT_PER_OPUS_FRAME )
        {
            callbackAudioOut60msSpaceAvail( AUDIO_SAMPLES_PER_FRAME * AUDIO_BYTES_PER_SAMPLE );
            m_AudioOutAecFramesProcessed = 0;
        }
    }

    // Any remaining samples (less than 160) stay in m_ResidualOutBuffer
    // until the next callback arrives.
}

//============================================================================
void AudioMgr::processAudioOutput10msFrame( const int16_t* pcmData, int sampleCnt )
{
    // callbacks to write mic data to aec should happen every 10ms
    static int64_t last10msFrameTime = 0;
    int64_t speaker10msFrameTime = GetHighResolutionTimeMs();
    if( last10msFrameTime != 0 )
    {
        const int64_t speaker10msFrameJitter = (speaker10msFrameTime - last10msFrameTime) - ECHO_MS_PER_FRAME;
        if(speaker10msFrameJitter != 0 )
        {
            if( speaker10msFrameJitter < m_SpeakerOutJitterLowMarkMs )
            {
                m_SpeakerOutJitterLowMarkMs = speaker10msFrameJitter;
            }
            if( speaker10msFrameJitter > m_SpeakerOutJitterHighMarkMs )
            {
                m_SpeakerOutJitterHighMarkMs = speaker10msFrameJitter;
            }
        }
    }
    
    last10msFrameTime = speaker10msFrameTime;
}
