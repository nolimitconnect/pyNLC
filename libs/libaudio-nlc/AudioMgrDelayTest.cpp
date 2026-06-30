
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
#include "AudioUtils.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxTime.h>
#include <CoreLib/VxElapseTimer.h>

//============================================================================
void AudioMgr::readTestToneSamples( int16_t* pcmData, int sampleCnt )
{
    if( !m_ToneGenerator.isFormatSet() )
    {
        m_ToneGenerator.setAudioFormat( m_AudioOutFormat );
    }

    m_ToneGenerator.readToneSamples( pcmData, sampleCnt );
}

//============================================================================
void AudioMgr::resetMicrophoneBuffers( void )
{
    m_PendingInBuffersMutex.lock();
    m_PendingInBuffers.clear();   
    m_PendingInBuffersMutex.unlock();
}

//============================================================================
void AudioMgr::resetSpeakerBuffers( void )
{
    m_PendingOutBuffersMutex.lock();
    m_PendingOutBuffers.clear();
    m_PendingOutBuffersMutex.unlock();
}

//============================================================================
bool AudioMgr::runEchoDelayTest( void )
{
    if( m_AudioTestState != eAudioTestStateNone )
    {
        LogMsg( LOG_ERROR, "AudioMgr::runAudioTest already running" );
        return false;
    }

    m_EchoDelayCurrentInteration = 0;
    m_EchoDelayResultList.clear();

    toGuiWantMicrophoneRecording( eMediaModuleSoundDelayTest, true );
    toGuiWantSpeakerOutput( eMediaModuleSoundDelayTest, true );

    setAudioTestState( eAudioTestStateInit );
 
    m_AudioTestTimer.start(1200);
    return true;
}

//============================================================================
void AudioMgr::callbackAudioTestTimer( void )
{
    int audioPeakValue{ 0 };
    int64_t audioDetectTimeMs{ 0 };
    switch( m_AudioTestState )
    {
    case eAudioTestStateInit:
        // waited for sound to be quiet
        LogMsg( LOG_VERBOSE, "Echo Delay Test Init" );
        m_DelayTestValid = true;
        setAudioTestState( eAudioTestStateRun );
        if(m_AudioDelayTestCallback){
            m_AudioDelayTestCallback->onAudioDelayTestStarted();
        }
        break;

    case eAudioTestStateRun:
        // send sound test cherp
        LogMsg( LOG_VERBOSE, "Echo Delay Test Run" );
        setAudioTestState( eAudioTestStateResult );
        break;

    case eAudioTestStateResult:
        // get the delay time result
        LogMsg( LOG_VERBOSE, "Echo Delay Test Result" );
        audioDetectTimeMs = getAudioTestDetectTime( audioPeakValue );
        if( !handleAudioTestResult( getAudioTestSentTime(), audioDetectTimeMs, audioPeakValue ) )
        {
            LogMsg( LOG_WARNING, "Echo Delay Test Failed" );
            toGuiWantMicrophoneRecording( eMediaModuleSoundDelayTest, false );
            toGuiWantSpeakerOutput( eMediaModuleSoundDelayTest, false );
            setAudioTestState( eAudioTestStateDone );
            break;
        }

        m_EchoDelayCurrentInteration++;
        if( m_EchoDelayCurrentInteration < m_EchoDelayTestMaxInterations )
        {
            // start test again
            LogMsg( LOG_VERBOSE, "Echo Delay Test Run Again" );
            setAudioTestState( eAudioTestStateInit );
        }
        else
        {
            LogMsg( LOG_VERBOSE, "Echo Delay Test Done" );
            setAudioTestState( eAudioTestStateDone ); 
        }

        break;

    case eAudioTestStateDone:
    {
        LogMsg( LOG_VERBOSE, "Echo Delay Test Restore Mic/Speaker states and finish" );
        m_AudioTestTimer.stop();
        setAudioTestState( eAudioTestStateNone );

        toGuiWantMicrophoneRecording( eMediaModuleSoundDelayTest, false );
        toGuiWantSpeakerOutput( eMediaModuleSoundDelayTest, false );

        sendAudioDelayTestFinished();

        break;
    }

    case eAudioTestStateNone:
    default:
        break;
    }
}

//============================================================================
void AudioMgr::setAudioTestState( EAudioTestState audioTestState )
{
    m_AudioTestState = audioTestState;

    if( eAudioTestStateInit == audioTestState || eAudioTestStateRun == audioTestState )
    {
        m_DelayTestDetectList.clear();
        setAudioTestSentTime( 0 );
        resetMicrophoneBuffers();
        resetSpeakerBuffers();
    }
}

//============================================================================
bool AudioMgr::handleAudioTestResult( int64_t soundOutTimeMs, int64_t soundDetectTimeMs, int peakVal0to100 )
{
    m_DelayTestValid = false;
    int64_t timeDif = soundDetectTimeMs - soundOutTimeMs;
    if(!soundOutTimeMs || !soundDetectTimeMs)
    {
        timeDif = 0;
        LogMsg( LOG_WARNING, "Sound Delay Not Detected. Check speaker volume and that microphone is on ");
    }
    else if( timeDif < 50 )
    {
       LogMsg( LOG_WARNING, "Sound Delay too short.. probably noise ");
    }
    else if( timeDif > 900 )
    {
        LogMsg( LOG_WARNING, "Sound Delay too long.. probably mic level low ");
    }
    else
    {
        LogMsg( LOG_WARNING, "Sound Delay is %lld ms", timeDif );
        m_DelayTestValid = true;
    }

    m_EchoDelayResultList.emplace_back( (int)timeDif );

    if(m_AudioDelayTestCallback){
        m_AudioDelayTestCallback->onAudioDelayTestProgress( (int)timeDif, m_DelayTestValid );
    } 

    return m_DelayTestValid;
}


//============================================================================
void AudioMgr::audioTestDetectTestSound( const int16_t* sampleInData, int inSampleCnt )
{
    int32_t maxPeak = 0;
    int peakIndex = 0;

    for (int i = 0; i < inSampleCnt; ++i) {
        // Use absolute value to find the true peak amplitude
        int32_t currentVal = std::abs((int32_t)sampleInData[i]);

        if (currentVal > maxPeak) {
            maxPeak = currentVal;
            peakIndex = i;
        }
    }

    // maxPeak now holds the energy (0 to 32768)
    // peakIndex holds the offset within the buffer where the peak was found

    if( maxPeak > 32768 / 10 ) 
    {
        int64_t detectTimeMsFromStartOfBuffer = AudioUtils::audioDurationMs( m_AudioInFormat, peakIndex * 2 ); // *2 since sampleInData is int16_t which is 2 bytes
        int64_t detectTimeMs = GetHighResolutionTimeMs() - AudioUtils::audioDurationMs( m_AudioInFormat, inSampleCnt * 2 ) + detectTimeMsFromStartOfBuffer;

        LogMsg( LOG_VERBOSE, "AudioMgr::%s detected pulse at delay %lld ms with peak value %d at %lld ms into buffer", __func__, 
                getAudioTestSentTime() - detectTimeMs, maxPeak, detectTimeMsFromStartOfBuffer );

        m_DelayTestDetectList.emplace_back( std::make_pair( detectTimeMs, maxPeak ) );
    }
}

//============================================================================
int64_t AudioMgr::getAudioTestDetectTime( int& peakValue )
{
    int64_t detectTime = 0;
    peakValue = 0;

    for( auto& pair : m_DelayTestDetectList )
    {
        if( pair.second > peakValue )
        {
            peakValue = pair.second;
            detectTime = pair.first;
        }
    }

    return detectTime;
}

//============================================================================
void AudioMgr::sendAudioDelayTestFinished( void )
{
    bool resultsValid = true;
    int averageDelay = 0;
    for( auto delayMs : m_EchoDelayResultList )
    {
        if( delayMs < 40 || delayMs > 500 )
        {
            resultsValid = false;
            break;
        }

        averageDelay += delayMs;
    }

    if(m_EchoDelayResultList.size())
    {
        averageDelay = averageDelay / m_EchoDelayResultList.size();
    } 
    else 
    {
        resultsValid = false;
    }
    
    if(m_AudioDelayTestCallback){
        m_AudioDelayTestCallback->onAudioDelayTestFinished( averageDelay, resultsValid );
    } 
}