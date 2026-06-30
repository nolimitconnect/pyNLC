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

class AudioDelayTestCallback
{
public:
    virtual void                onAudioDelayTestStarted( void ) = 0;

    virtual void                onAudioDelayTestProgress( int delayMs, bool delayValueValid ) = 0;

    virtual void                onAudioDelayTestFinished( int delayAverageMs, bool delayValueValid ) = 0;
};
