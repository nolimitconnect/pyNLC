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
class MiniAudioInDevice
{
public:
    MiniAudioInDevice( AudioMgr& maMgr );
	~MiniAudioInDevice() = default;

	virtual bool				initializeAudioInDevice( int& deviceIndex, int preferredRate, int& retActualRate );

	virtual bool				startAudioInDevice( void );
	virtual void				stopAudioInDevice( void );

	virtual int					callbackAudioWrite( int16_t* pcmData, int lenBytes ) = 0;

protected:
	bool						initalizeDevice( int deviceIndex, int sampleRate );

    AudioMgr&					m_AudioIoMgr;

	ma_context					m_MaContext;
	ma_device_info*				m_MaMicDeviceInfos{ nullptr };
	ma_uint32					m_MaMicDeviceCount{ 0 };
	ma_device_config			m_MaDeviceConfig;
	ma_device					m_MaDevice;

	bool						m_DeviceAvailable{ false };
	bool						m_DeviceActive{ false };
	int                         m_AudioDeviceIndex{ 0 };
};

