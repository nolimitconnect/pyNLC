#pragma once
//============================================================================
// Copyright (C) 2025 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <GuiInterface/IAudioDefs.h>

#include <stdint.h>
#include <string>

class VxAudioFormat
{
public:
	enum SampleFormat  
	{
		Unknown,
		UInt8,
		Int16,
		Int32,
		Float,
		MaxSampleFormats
	};

	VxAudioFormat() = default;
	VxAudioFormat( VxAudioFormat& rhs );
	VxAudioFormat( const VxAudioFormat& rhs );

	VxAudioFormat& operator = ( const VxAudioFormat& rhs );

	void						setSampleRate( int rateHz )			{ m_Rate = rateHz; }
	int							sampleRate( void ) const			{ return m_Rate; }

	void						setBytesPerSample( int byteCnt )	{ m_ChannelBytes = byteCnt; }
	int							bytesPerSample( void ) const		{ return m_ChannelBytes; };

	void						setChannelCount( int channelCnt )	{ m_ChannelCount = channelCnt; }
	int							channelCount( void ) const			{ return m_ChannelCount; }

	void						setSampleFormat( SampleFormat sampleFmt ) { m_SampleFormat = sampleFmt; }
	SampleFormat				sampleFormat( void ) const			{ return m_SampleFormat; }

	int64_t						bytesForDuration( int64_t durationUs ) const;

	bool						isValid( void ) const;

    std::string					describeFormat( void ) const;

protected:
	int							m_Rate{ AUDIO_DEVICE_SAMPLE_RATE };
	int							m_ChannelBytes{ 2 };
	int							m_ChannelCount{ 1 };
	SampleFormat				m_SampleFormat{ Int16 };
};
