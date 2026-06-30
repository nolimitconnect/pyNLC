#pragma once
//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxDefs.h"

//=== VxElapseTimer Class object ===//
//there are a couple of ways to use the timer
//		any call to StartTime with any value sets the start time
//		so that calls to ElapsedTime return the Time since last called startTimer or timer was constructed,

//		The second mode is startTimer with a time till done
//		Subsequent calls to TimeDone return FALSE if the specified TimeTillDone has not been
//		reached or TRUE if the time has expired
//
//		NOTE: even after the timerDone has expired, calls to ElapsedTimer are still valid

class VxElapseTimer
{
public:
	VxElapseTimer();
    virtual ~VxElapseTimer() = default;

    double						elapsedMs( void );       // return the elapsed time in milliseconds since timer was started or created 
    double						elapsedSec( void )  { return ( elapsedMs() / 1000 ); }      // return the elapsed time in seconds since
									                                                        // startTimer was called or VxElapseTimer was constructed...it doesn't mater if the time
									                                                        // specified in the startTimer parameter has elapsed or not
    // same as start timer.. start at 0 then read elapsed time since reset or start timer
    void                        resetTimer( void )                          { startTimerMs( 0 ); }

	// just sets start time for elapsed time
    void						startTimer( void )							{ startTimerMs(0); }

    // starts a timer and sets time in milliseconds till timerDone returns true
    void						startTimerMs( int milliSec );

	// starts a timer and sets time till timerDone returns true
    void						startTimerSec( int dSec  )                  { startTimerMs( dSec * 1000 ); }

    // returns true when time specified has expired
	bool						timerDone( void )							{ return elapsedMs() > m_TimeTillDoneMs ? true : false; }

    void						waitTimeMs( int milliSec );
    void						waitTimeSec( int dSec )                     { waitTimeMs( dSec * 1000 ); }

private:
    int64_t						m_TimeTillDoneMs{0};
    int64_t						m_StartTickMs{0};
};


int64_t GetHighResolutionTimeUs( void ); //  time in usec
int64_t GetHighResolutionTimeMs( void ); //  time in millisec
int64_t GetHighResolutionTimeSeconds( void );