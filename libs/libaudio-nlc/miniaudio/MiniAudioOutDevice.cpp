//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "MiniAudioOutDevice.h"
#include "AudioMgr.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include <GuiInterface/IToGui.h>

namespace
{
    void MiniAudioOutCallback( ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount )
    {
        if(VxIsAppShuttingDown())
        {
            return;
        }

        MiniAudioOutDevice* miniAudioDevice = (MiniAudioOutDevice*)pDevice->pUserData;
        vx_assert( miniAudioDevice != nullptr );

        miniAudioDevice->callbackAudioRead( (int16_t*)pOutput, frameCount );

        (void)pInput;
    }
}

//============================================================================
MiniAudioOutDevice::MiniAudioOutDevice( AudioMgr& maMgr )
    : m_AudioIoMgr( maMgr )
{
}

//============================================================================
bool MiniAudioOutDevice::initializeAudioOutDevice( int& deviceIndex, int preferredRate, int& retActualRate )
{
    if( m_DeviceActive )
    {
        stopAudioOutDevice();
    }

    retActualRate = 0;
    // first try with preferredRate
    if( initalizeDevice( deviceIndex, preferredRate ) )
    {
        LogMsg( LOG_VERBOSE, "MiniAudioOutDevice::initializeAudioOutDevice SUCCESS index %d rate %d", deviceIndex, preferredRate );
        retActualRate = preferredRate;
        m_AudioDeviceIndex = deviceIndex;
        return true;
    }
    else
    {
        LogMsg( LOG_VERBOSE, "MiniAudioOutDevice::initializeAudioOutDevice FAIL index %d rate %d", deviceIndex, preferredRate );
    }

    // most devices can at least run at 48000 .. this seems to be android's default
    if( preferredRate != 48000 && initalizeDevice( deviceIndex, 48000 ) )
    {
        retActualRate = 48000;
        m_AudioDeviceIndex = deviceIndex;
        LogMsg( LOG_VERBOSE, "MiniAudioOutDevice::initializeAudioOutDevice SUCCESS index %d rate %d", deviceIndex, retActualRate );
        return true;
    }

    LogMsg( LOG_ERROR, "MiniAudioOutDevice::initializeAudioOutDevice FAILED index %d", deviceIndex );
    return false;
}

//============================================================================
bool MiniAudioOutDevice::startAudioOutDevice( void )
{
    if( !m_DeviceActive )
    {
        ma_result result = ma_device_start( &m_MaDevice );
        if( result != MA_SUCCESS )
        {
            ma_device_uninit( &m_MaDevice );
            LogMsg( LOG_VERBOSE, "MiniAudioOutDevice::startAudioOut Failed to start device." );
            m_DeviceActive = false;
        }
        else
        {
            m_DeviceActive = true;
        }
    }

    return m_DeviceActive;
}

//============================================================================
void MiniAudioOutDevice::stopAudioOutDevice( void )
{
    if( m_DeviceActive )
    {
        m_DeviceActive = false;
        ma_device_stop( &m_MaDevice );
    }
}

//============================================================================
bool MiniAudioOutDevice::initalizeDevice( int deviceIndex, int sampleRate )
{
    if( m_DeviceActive )
    {
        stopAudioOutDevice();
    }

    if( m_DeviceAvailable )
    {
        m_DeviceAvailable = false;
        ma_device_uninit( &m_MaDevice );
    }
    
    m_MaDeviceConfig = ma_device_config_init( ma_device_type_playback );
    m_MaDeviceConfig.playback.pDeviceID = m_AudioIoMgr.getAudioOutDeviceId( deviceIndex );
    m_MaDeviceConfig.playback.format = ma_format_s16;
    m_MaDeviceConfig.playback.channels = 1;
    //m_MaDeviceConfig.playback.shareMode = ma_share_mode_shared;
    m_MaDeviceConfig.sampleRate = sampleRate;
    m_MaDeviceConfig.dataCallback = MiniAudioOutCallback;
    m_MaDeviceConfig.pUserData = this;

    m_MaDeviceConfig.noPreSilencedOutputBuffer = true;
    //m_MaDeviceConfig.performanceProfile = ma_performance_profile_conservative;

    m_MaDeviceConfig.periodSizeInFrames = ECHO_FRAME_SIZE_10MS;

    //ma_result result = ma_device_init( &m_MaContext, &m_MaDeviceConfig, &m_MaDevice );
    ma_result result = ma_device_init( NULL, &m_MaDeviceConfig, &m_MaDevice );
    if( result != MA_SUCCESS ) 
    {
        LogMsg( LOG_VERBOSE, "MiniAudioOutDevice::startAudioOut Failed to initialize playback device." );
        m_DeviceAvailable = false;
        return false;
    }

    // start and stop device just to make sure it works
    result = ma_device_start( &m_MaDevice );
    if( result != MA_SUCCESS ) 
    {
        ma_device_uninit( &m_MaDevice );
        LogMsg( LOG_VERBOSE, "MiniAudioOutDevice::startAudioOut Failed to start device." );
        m_DeviceAvailable = false;
        return false;
    }

    ma_device_stop( &m_MaDevice );

    m_DeviceAvailable = true;
    return true;
}
