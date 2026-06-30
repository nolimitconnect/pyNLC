#pragma once
//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <vector>
#include <string>

#include "miniaudio.h"

class MiniAudioDevices
{
public:
	MiniAudioDevices() = default;
	~MiniAudioDevices() = default;

	bool						startupMiniAudio( void );
	void						shutdownMiniAudio( void );

	bool                        getIsMicrophoneAvailable( void )	{ return m_MaMicDeviceCount != 0; }
	bool                        getIsSpeakerAvailable( void )	    { return m_MaSpeakerDeviceCount != 0; }

	std::vector<std::string>&	getAudioInDevices( void )			{ return m_MicDeviceDescriptions; }
	std::vector<std::string>&   getAudioOutDevices( void )			{ return m_SpeakerDeviceDescriptions; }

	int							getAudioInDeviceCount( void )		{ return (int)m_MicDeviceDescriptions.size(); }
	int							getAudioOutDeviceCount( void )		{ return (int)m_SpeakerDeviceDescriptions.size(); }

	std::string					getAudioInDeviceDesc( int deviceIndex );
	std::string					getAudioOutDeviceDesc( int deviceIndex );

	ma_device_id*				getAudioInDeviceId( int deviceIdx );
	ma_device_id*				getAudioOutDeviceId( int deviceIdx );

    virtual void                onAudioDevicesInitialized( bool hasDevices ) {};

    int                         findAudioInDeviceIndexByDescription( const std::string& deviceDescription );
    int                         findAudioOutDeviceIndexByDescription( const std::string& deviceDescription );

protected:
	ma_context					m_MaContext;
	ma_device_info*				m_MaSpeakerDeviceInfos{ nullptr };
	ma_uint32					m_MaSpeakerDeviceCount{ 0 };
	ma_device_info*				m_MaMicDeviceInfos{ nullptr };
	ma_uint32					m_MaMicDeviceCount{ 0 };

	std::vector<std::string>	m_SpeakerDeviceDescriptions;
	std::vector<std::string>	m_MicDeviceDescriptions;
};

