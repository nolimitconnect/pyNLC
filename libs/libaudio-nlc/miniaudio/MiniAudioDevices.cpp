//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "MiniAudioDevices.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxTime.h>

//============================================================================
bool MiniAudioDevices::startupMiniAudio( void )
{
    int startTime = GetApplicationAliveMs();

    if( ma_context_init( NULL, 0, NULL, &m_MaContext ) != MA_SUCCESS ) 
    {
        LogMsg( LOG_ERROR, "MiniAudioDevices::%s Failed to initialize context.", __func__ );
        return false;
    }

    ma_result result = ma_context_get_devices( &m_MaContext, &m_MaSpeakerDeviceInfos, &m_MaSpeakerDeviceCount, &m_MaMicDeviceInfos, &m_MaMicDeviceCount );
    if( result != MA_SUCCESS ) 
    {
        LogMsg( LOG_ERROR, "MiniAudioDevices::%s Failed to retrieve device information.", __func__ );
        return false;
    }

    for( ma_uint32 iDevice = 0; iDevice < m_MaSpeakerDeviceCount; ++iDevice )
    {
        LogMsg( LOG_VERBOSE, "MiniAudioDevices::%s speaker index %u: %s", __func__, iDevice, m_MaSpeakerDeviceInfos[ iDevice ].name );
        m_SpeakerDeviceDescriptions.emplace_back( m_MaSpeakerDeviceInfos[ iDevice ].name );
    }

    for( ma_uint32 iDevice = 0; iDevice < m_MaMicDeviceCount; ++iDevice )
    {
        LogMsg( LOG_VERBOSE, "MiniAudioDevices::%s Speaker index %u: %s", __func__, iDevice, m_MaMicDeviceInfos[ iDevice ].name );
        m_MicDeviceDescriptions.emplace_back( m_MaMicDeviceInfos[ iDevice ].name );
    }

    int endTime = GetApplicationAliveMs();
    LogMsg( LOG_VERBOSE, "MiniAudioDevices::%s took %d ms at %d", __func__, endTime - startTime, endTime );

    onAudioDevicesInitialized( !m_SpeakerDeviceDescriptions.empty() );

    return !m_SpeakerDeviceDescriptions.empty();
}

//============================================================================
void MiniAudioDevices::shutdownMiniAudio( void )
{
    ma_context_uninit( &m_MaContext );
}

//============================================================================
std::string MiniAudioDevices::getAudioInDeviceDesc( int deviceIndex )
{
    if( deviceIndex >= 0 && deviceIndex < m_MicDeviceDescriptions.size() )
    {
        return m_MicDeviceDescriptions[ deviceIndex ];
    }
    else
    {
        return "Unknown sound input device";
    }
}

//============================================================================
std::string MiniAudioDevices::getAudioOutDeviceDesc( int deviceIndex )
{
    if( deviceIndex >= 0 && deviceIndex < m_SpeakerDeviceDescriptions.size() )
    {
        return m_SpeakerDeviceDescriptions[ deviceIndex ];
    }
    else
    {
        return "Unknown sound output device";
    }
}

//============================================================================
ma_device_id* MiniAudioDevices::getAudioInDeviceId( int deviceIdx )
{
    return &m_MaMicDeviceInfos[ deviceIdx ].id;
}

//============================================================================
ma_device_id* MiniAudioDevices::getAudioOutDeviceId( int deviceIdx )
{
    return &m_MaSpeakerDeviceInfos[ deviceIdx ].id;
}

//============================================================================
int MiniAudioDevices::findAudioInDeviceIndexByDescription( const std::string& deviceDescription )
{
    for( int i = 0; i < m_MicDeviceDescriptions.size(); ++i )
    {
        if( m_MicDeviceDescriptions[i] == deviceDescription )
        {
            return i;
        }
    }

    LogMsg( LOG_VERBOSE, "MiniAudioDevices::%s could not find device with description: %s", __func__, deviceDescription.c_str() );
    return -1;
}

//============================================================================
int MiniAudioDevices::findAudioOutDeviceIndexByDescription( const std::string& deviceDescription )
{
    for( int i = 0; i < m_SpeakerDeviceDescriptions.size(); ++i )
    {
        if( m_SpeakerDeviceDescriptions[i] == deviceDescription )
        {
            return i;
        }
    }

    LogMsg( LOG_VERBOSE, "MiniAudioDevices::%s could not find device with description: %s", __func__, deviceDescription.c_str() );
    return -1;
}