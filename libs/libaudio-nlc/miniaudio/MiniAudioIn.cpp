//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioMgr.h"
#include "MiniAudioIn.h"
#include "AudioMgr.h"
#include "AudioUtils.h"

#include <GuiInterface/IToGui.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include <math.h>

//============================================================================
MiniAudioIn::MiniAudioIn( AudioMgr& mgr )
: MiniAudioInDevice( mgr )
{
    memset( m_MicSilence, 0, sizeof( m_MicSilence ) );

    // m_MicInBitrate.setLogMessagePrefix( "Mic in " );
    // m_MicOutBitrate.setLogMessagePrefix( "Mic out " );
}

//============================================================================
bool MiniAudioIn::initAudioIn( VxAudioFormat& audioFormat, int deviceIndex )
{
    m_AudioFormat = audioFormat;
    soundInDeviceChanged( deviceIndex );
    audioFormat = m_AudioFormat;
    return m_initialized;
}

//============================================================================
int MiniAudioIn::getHardwareDelayMs( void )
{
    if( !m_initialized )
    {
        return 0;
    }

    float hardwareLatency = m_MaDevice.capture.internalPeriodSizeInFrames * 1000.0f / m_AudioFormat.sampleRate();
    return static_cast<int>(hardwareLatency);
}

//============================================================================
bool MiniAudioIn::soundInDeviceChanged( int deviceIndex )
{
    int deviceCount = m_AudioIoMgr.getAudioInDeviceCount();
    if( !deviceCount )
    {
        IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrNoMicDevices, "" );
        return false;
    }

    if( deviceIndex >= deviceCount )
    {
        IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrMicDeviceOutOfRange, "" );
        deviceIndex = 0;
    }

    stopAudioInHardware();
    m_AudioFormat.setSampleRate( AUDIO_DEVICE_SAMPLE_RATE );
    m_AudioFormat.setChannelCount( AUDIO_CHANNELS );

    int preferredDeviceIndex = deviceIndex;
    int actualRate = 0;
    m_initialized = initializeAudioInDevice( preferredDeviceIndex, m_AudioFormat.sampleRate(), actualRate );
    if( m_initialized )
    {
        if( actualRate != m_AudioFormat.sampleRate() )
        {
            m_AudioFormat.setSampleRate( actualRate );
        }

        if( preferredDeviceIndex != deviceIndex )
        {
            m_AudioIoMgr.setSoundInDeviceIndex( deviceIndex );
        }

        setDivideSamplesCount( m_AudioFormat.sampleRate() / AUDIO_DEVICE_SAMPLE_RATE );

        if( m_AudioIoMgr.getIsMicrophoneWanted() )
        {
            startAudioInHardware();
        }
    }
    else
    {
        IToGui::getIToGui().toGuiAppPopupErr( eAppPopupErrMicDeviceFailedToInitialize, "" );
    }

    return m_initialized;
}

//============================================================================
bool MiniAudioIn::startAudioInHardware( void )
{
    if( !m_AudioInDeviceIsStarted )
    {
        m_AudioInDeviceIsStarted = startAudioInDevice();
        if( m_AudioInDeviceIsStarted )
        {
            m_AudioIoMgr.setIsMicrophoneRunning( true );
        }
        else
        {
            LogMsg( LOG_ERROR, "MiniAudioIn::startAudioInHardware failed " );
        }
    }

    return m_AudioInDeviceIsStarted;
}

//============================================================================
void MiniAudioIn::stopAudioInHardware( void )
{
    if( m_AudioInDeviceIsStarted )
    {
        m_AudioInDeviceIsStarted = false;
        stopAudioInDevice();
        m_AudioIoMgr.setIsMicrophoneRunning( false );
    }
}

//============================================================================
void MiniAudioIn::wantMicrophoneInputHardware( bool enableInput )
{
    m_MicInputEnabled = enableInput;

    if( enableInput )
    {
        m_AudioInDeviceIsStarted = startAudioInHardware();
        m_AudioIoMgr.setIsMicrophoneRunning( m_AudioInDeviceIsStarted );
    }
    else
    {
        stopAudioInHardware();
        m_AudioInDeviceIsStarted = false;
        m_AudioIoMgr.setIsMicrophoneRunning( false );
    }
}

//============================================================================
int MiniAudioIn::callbackAudioWrite( int16_t* pcmData, int sampleCnt )
{
    // LogMsg( LOG_VERBOSE, "MiniAudioIn::callbackAudioWrite %d ", sampleCnt );

    if( VxIsAppShuttingDown() )
    {
        // do not attempt anything while being destroyed
        return 0;
    }

    if( !pcmData )
    {
        LogMsg( LOG_ERROR, "MiniAudioIn::callbackAudioWrite has null pcmData." );
        return sampleCnt;
    }

    if( !sampleCnt )
    {
        LogMsg( LOG_ERROR, "MiniAudioIn::writeData has 0 sampleCnt." );
        return sampleCnt;
    }

    m_AudioIoMgr.callbackAudioDeviceWrite( pcmData, sampleCnt );
    return sampleCnt;
}
