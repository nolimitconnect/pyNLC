//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "MiniAudioOut.h"
#include "AudioMgr.h"

#include <GuiInterface/IToGui.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxElapseTimer.h>

#include <math.h>

//============================================================================
MiniAudioOut::MiniAudioOut( AudioMgr& mgr )
: MiniAudioOutDevice( mgr )
{
}

//============================================================================
bool MiniAudioOut::initAudioOut( VxAudioFormat& audioFormat, int deviceIndex )
{
    m_AudioFormat = audioFormat;
    soundOutDeviceChanged( deviceIndex );
    audioFormat = m_AudioFormat;
    return m_initialized;
}

//============================================================================
int MiniAudioOut::getHardwareDelayMs( void )
{
    if( !m_initialized )
    {
        return 0;
    }

    float hardwareLatency = m_MaDevice.playback.internalPeriodSizeInFrames * 1000.0f / m_AudioFormat.sampleRate();
    return static_cast<int>(hardwareLatency);
}

//============================================================================
bool MiniAudioOut::soundOutDeviceChanged( int deviceIndex )
{
    int deviceCount = m_AudioIoMgr.getAudioOutDeviceCount();
    if( !deviceCount )
    {
       IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrNoMicDevices, "" );
        return false;
    }

    if( deviceIndex >= deviceCount )
    {
        IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrSpeakerDeviceOutOfRange, "" );
        deviceIndex = 0;
    }

    stopAudioOutHardware();
    if( !m_AudioFormat.sampleRate() )
    {
        IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrSpeakerDeviceInvaidFormat, "" );
        deviceIndex = 0;
        m_AudioFormat.setSampleRate( AUDIO_DEVICE_SAMPLE_RATE );
        m_AudioFormat.setChannelCount( AUDIO_CHANNELS );
    }

    int preferredDeviceIndex = deviceIndex;
    int actualRate = 0;
    m_initialized = initializeAudioOutDevice( preferredDeviceIndex, m_AudioFormat.sampleRate(), actualRate );
    if( m_initialized )
    {
        m_AudioDeviceIndex = preferredDeviceIndex;
        if( actualRate != m_AudioFormat.sampleRate() )
        {
            m_AudioFormat.setSampleRate( actualRate );
        }

        if( preferredDeviceIndex != deviceIndex )
        {
            m_AudioIoMgr.setSoundInDeviceIndex( deviceIndex );
        }

        setUpsampleMultiplier( m_AudioFormat.sampleRate() / AUDIO_DEVICE_SAMPLE_RATE );

        if( m_AudioIoMgr.getIsSpeakerWanted() )
        {
            startAudioOutHardware();
        }
    }
    else
    {
       IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrSpeakerDeviceFailedToInitialize, "" );
    }

    return m_initialized;
}

//============================================================================
void MiniAudioOut::wantSpeakerOutputHardware( bool enableOutput )
{
    m_SpeakerOutputEnabled = enableOutput;

    if( enableOutput )
    {
        if( !m_AudioOutDeviceIsStarted )
        {
            m_AudioIoMgr.setNeedAudioOutDeviceStop( false );
            m_AudioOutDeviceIsStarted = startAudioOutHardware();
            if( m_AudioOutDeviceIsStarted )
            {
                m_AudioIoMgr.setIsSpeakerRunning( true );
            }
            else
            {
                LogMsg( LOG_DEBUG, "MiniAudioOut::wantSpeakerOutputHardware failed to start audio out device" );
            }
        }
    }
    else
    {
        m_AudioIoMgr.setNeedAudioOutDeviceStop( true );
    }
}

//============================================================================
bool MiniAudioOut::startAudioOutHardware( void )
{
    if( !m_AudioOutDeviceIsStarted )
    {
        m_AudioOutDeviceIsStarted = startAudioOutDevice();
        if( m_AudioOutDeviceIsStarted )
        {
            m_AudioIoMgr.setIsSpeakerRunning( true );
        }
        else
        {
            LogMsg( LOG_DEBUG, "MiniAudioOut::startAudioOutHardware failed " );
        }
    } 
    
    return m_AudioOutDeviceIsStarted;
}

//============================================================================
void MiniAudioOut::stopAudioOutHardware( void )
{
    if( m_AudioOutDeviceIsStarted )
    {
        m_AudioOutDeviceIsStarted = false;
        stopAudioOutDevice();
        m_AudioIoMgr.speakerDeviceEnabled( false );
    }
}

//============================================================================
int MiniAudioOut::callbackAudioRead( int16_t* pcmData, int sampleCnt )
{
    if( VxIsAppShuttingDown() )
    {
        // do not attempt anything while being destroyed
        return 0;
    }

    if( !pcmData || !sampleCnt )
    {
        LogMsg( LOG_ERROR, "MiniAudioOut::callbackAudioRead has null pcmData or sampleCnt." );
        return sampleCnt;
    }

    m_AudioIoMgr.callbackReadSpeakerData( pcmData, sampleCnt );

    return sampleCnt;
}
