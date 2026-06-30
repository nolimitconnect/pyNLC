#pragma once
//============================================================================
// Copyright (C) 2022 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioDefs.h"

#include <inttypes.h>

#include <vector>

class AudioSampleBuf
{
public:
	AudioSampleBuf();
	AudioSampleBuf( const AudioSampleBuf& rhs );
	~AudioSampleBuf() = default;

	AudioSampleBuf& operator = ( const AudioSampleBuf& rhs );

	void						clear( void )							{ m_SampleCnt = 0; }
	bool						empty( void )							{ return 0 == m_SampleCnt; }
	int							freeSpaceSampleCount( void )			{ return m_MaxSamples - m_SampleCnt; }

	void						setMaxSamples( int maxSamples ); // also resizes m_PcmData
	int							getMaxSamples( void )					{ return m_MaxSamples; }

	void						setSampleCnt( int sampleCnt )
	{
		if( sampleCnt < 0 )
		{
			m_SampleCnt = 0;
		}
		else
		{
			const int vectorMaxSamples = static_cast<int>( m_PcmData.size() );
			if( sampleCnt > m_MaxSamples )
			{
				sampleCnt = m_MaxSamples;
			}

			if( sampleCnt > vectorMaxSamples )
			{
				sampleCnt = vectorMaxSamples;
			}

			m_SampleCnt = sampleCnt;
		}
	}
	int							getSampleCnt( void )					{ return m_SampleCnt; }

	int16_t*					getSampleBuffer( bool atCurIdx = false )
	{
		int sampleIdx = atCurIdx ? m_SampleCnt : 0;
		if( sampleIdx < 0 )
		{
			sampleIdx = 0;
		}

		const int vectorMaxSamples = static_cast<int>( m_PcmData.size() );
		if( sampleIdx > vectorMaxSamples )
		{
			sampleIdx = vectorMaxSamples;
		}

		return m_PcmData.data() + sampleIdx;
	}

	virtual int					writeSamples( int16_t* samplesBuf, int sampleCnt );
	void						samplesWereWritten( int sampleCnt );

	virtual int					readSamples( int16_t* srcSamplesBuf, int sampleCnt );
	virtual void				samplesWereRead( int samplesRead ); // move remainder samples to begining of buffer and decrement sample count

	int16_t						getLastSample( void );

	int							getAudioDurationMs( int sampleRate = ECHO_SAMPLE_RATE ); // get the samples duration in ms

	void						truncateSamples( int sampleCnt ); // if contains more samples than this then limit to sampleCnt

private:
	std::vector<int16_t>		m_PcmData;

	int							m_MaxSamples{ AUDIO_SAMPLES_PER_FRAME };
	int							m_SampleCnt{ 0 };
};
