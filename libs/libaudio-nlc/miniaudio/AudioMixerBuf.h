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

#include "AudioDefs.h"

#include <deque>
#include <vector>
#include <cstdint>
#include <mutex>

// Assumes mono 16-bit PCM samples at AUDIO_DEVICE_SAMPLE_RATE sample rate, and that the caller will write exactly AUDIO_SAMPLES_PER_FRAME samples at a time
class AudioMixerBuf 
{
public:
    static constexpr int SAMPLES_PER_60MS = AUDIO_SAMPLES_PER_FRAME;
    static constexpr size_t MAX_FRAMES = 3;
    static constexpr size_t MAX_SAMPLES = MAX_FRAMES * SAMPLES_PER_60MS;

    AudioMixerBuf() = default;
    ~AudioMixerBuf() = default;

    //============================================================================
    void clear( void )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
    }

    //============================================================================
    int getMaxSamples( void )
    {
        return MAX_SAMPLES;
    }

    //============================================================================
    int getSampleCnt( void )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        int sampleCount = m_queue.size() * SAMPLES_PER_60MS;
        return sampleCount;
    }

    //============================================================================
    int freeSpaceSampleCount( void )
    {
        return MAX_SAMPLES - getSampleCnt();
    }   

    //============================================================================
    int writeSamples( const int16_t* data ) 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if( m_queue.size() >= MAX_FRAMES ) 
        {
            m_queue.pop_front();
        }

        // Emplace directly to avoid extra copies
        m_queue.emplace_back( data, data + SAMPLES_PER_60MS );   
        return SAMPLES_PER_60MS;
    }

    //============================================================================
    bool getFrame(int16_t* outData) 
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if (m_queue.empty()) return false;

        auto& frame = m_queue.front();
        if (frame.empty()) 
        {
            std::fill_n(outData, SAMPLES_PER_60MS, 0);
        } 
        else 
        {
            std::copy(frame.begin(), frame.end(), outData);
        }
        
        m_queue.pop_front();
        return true;
    }

private:
    std::mutex m_mutex;
    std::deque<std::vector<int16_t>> m_queue;
};
