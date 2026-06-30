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

#include "miniaudio.h"

#include <inttypes.h>

class AudioMgr;
class MiniAudioOutDevice
{
public:
	MiniAudioOutDevice( AudioMgr& maMgr );
	~MiniAudioOutDevice() = default;

	virtual bool				initializeAudioOutDevice( int& deviceIndex, int preferredRate, int& retActualRate );

	virtual bool				startAudioOutDevice( void );
	virtual void				stopAudioOutDevice( void );

	virtual int					callbackAudioRead( int16_t* pcmData, int lenBytes ) = 0;

protected:
	bool						initalizeDevice( int deviceIndex, int sampleRate );

	AudioMgr&					m_AudioIoMgr;

	ma_context					m_MaContext;
	ma_device_info*				m_MaSpeakerDeviceInfos{ nullptr };
	ma_uint32					m_MaSpeakerDeviceCount{ 0 };
	ma_device_config			m_MaDeviceConfig;
	ma_device					m_MaDevice;

	bool						m_DeviceAvailable{ false };
	bool						m_DeviceActive{ false };
	int                         m_AudioDeviceIndex{ 0 };
};

