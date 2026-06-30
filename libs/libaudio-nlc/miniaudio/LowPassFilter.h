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

#include <cmath>
#include <vector>
#include <cstdint>

#ifndef M_PI
    #define M_PI 3.14159265358979323846f
#endif

class LowPassFilter {
public:
    LowPassFilter(int cutoffHz = 8000, int sampleRate = AUDIO_DEVICE_SAMPLE_RATE) 
    {
        // Calculate coefficients for a simple 1st-order IIR filter
        // Higher order (Butterworth) is better, but this is extremely fast
        float rc = 1.0f / (cutoffHz * 2.0f * M_PI);
        float dt = 1.0f / sampleRate;
        m_alpha = dt / (rc + dt);
    }

    // Processes samples in-place
    void processBufferInPlace(int16_t* data, size_t sampleCount) 
    {
        for (size_t i = 0; i < sampleCount; ++i) 
        {
            // Formula: y[i] = y[i-1] + alpha * (x[i] - y[i-1])
            float currentSample = static_cast<float>(data[i]);
            m_previousOutput = m_previousOutput + m_alpha * (currentSample - m_previousOutput);
            data[i] = static_cast<int16_t>(m_previousOutput);
        }
    }

private:
    float m_alpha;
    float m_previousOutput = 0.0f;
};
