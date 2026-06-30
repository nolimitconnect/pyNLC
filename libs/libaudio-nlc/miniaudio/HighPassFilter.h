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

#ifndef M_PI
    #define M_PI 3.14159265358979323846f
#endif

class HighPassFilter 
{
public:
    HighPassFilter(int cutoffHz = 150, int sampleRate = AUDIO_DEVICE_SAMPLE_RATE) 
    {
        // Formula for alpha in a 1st order HPF
        float rc = 1.0f / (cutoffHz * 2.0f * M_PI);
        float dt = 1.0f / sampleRate;
        m_alpha = rc / (rc + dt);
    }

    // Processes samples in-place
    void processBufferInPlace(int16_t* data, size_t sampleCount) 
    {
        for (size_t i = 0; i < sampleCount; ++i) 
        {
            float currentInput = static_cast<float>(data[i]);
            
            // HPF Formula: y[i] = alpha * (y[i-1] + x[i] - x[i-1])
            float output = m_alpha * (m_prevOutput + currentInput - m_prevInput);
            
            m_prevInput = currentInput;
            m_prevOutput = output;
            
            data[i] = static_cast<int16_t>(output);
        }
    }

private:
    float m_alpha;
    float m_prevInput = 0.0f;
    float m_prevOutput = 0.0f;
};