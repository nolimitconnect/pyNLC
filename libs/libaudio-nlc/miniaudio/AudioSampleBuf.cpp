//============================================================================
// Copyright (C) 2022 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioSampleBuf.h"
#include "AudioUtils.h"

#include <CoreLib/VxDebug.h>

#include <algorithm>
#include <cstring>

//============================================================================
AudioSampleBuf::AudioSampleBuf()
{
	m_PcmData.resize( m_MaxSamples );
}

//============================================================================
AudioSampleBuf::AudioSampleBuf( const AudioSampleBuf& rhs )
	: m_PcmData( rhs.m_PcmData )
	, m_MaxSamples( rhs.m_MaxSamples )
	, m_SampleCnt( rhs.m_SampleCnt )
{
}

//============================================================================
AudioSampleBuf& AudioSampleBuf::operator = ( const AudioSampleBuf& rhs )
{
	if( &rhs != this )
	{
		m_PcmData = rhs.m_PcmData;
		m_MaxSamples = rhs.m_MaxSamples;
		m_SampleCnt = rhs.m_SampleCnt;
	}

	return *this;
}

//============================================================================
void AudioSampleBuf::setMaxSamples( int maxSamples )
{
	m_MaxSamples = std::max( 0, maxSamples );
	m_SampleCnt = 0;
	m_PcmData.resize( m_MaxSamples );
}

//============================================================================
int AudioSampleBuf::readSamples( int16_t* retSamplesBuf, int sampleCnt )
{
	int samplesToRead = std::min( sampleCnt, m_SampleCnt );
	if( samplesToRead )
	{
		memcpy( retSamplesBuf, m_PcmData.data(), samplesToRead * AUDIO_BYTES_PER_SAMPLE);
		samplesWereRead( samplesToRead );
	}
	else
	{
		if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_ERROR, "AudioSampleBuf::readSamples ERROR samplesToRead %d m_SampleCnt %d", sampleCnt, m_SampleCnt );
	}

	return samplesToRead;
}

//============================================================================
void AudioSampleBuf::samplesWereRead( int samplesRead )
{
	if( samplesRead <= 0 )
	{
		return;
	}

	const int vectorMaxSamples = static_cast<int>( m_PcmData.size() );
	if( m_SampleCnt < 0 || m_SampleCnt > m_MaxSamples || m_SampleCnt > vectorMaxSamples )
	{
		if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_ERROR, "AudioSampleBuf::samplesWereRead clamping invalid m_SampleCnt %d max %d vector %d", m_SampleCnt, m_MaxSamples, vectorMaxSamples );
		m_SampleCnt = std::min( std::max( m_SampleCnt, 0 ), std::min( m_MaxSamples, vectorMaxSamples ) );
	}

	if( samplesRead >= m_SampleCnt )
	{
		if( samplesRead > m_SampleCnt && LogEnabled(eLogAudioIo) )
		{
			LogModule( eLogAudioIo, LOG_WARNING, "AudioSampleBuf::samplesWereRead over-read samplesRead %d m_SampleCnt %d", samplesRead, m_SampleCnt );
		}

		m_SampleCnt = 0;
	}
	else
	{
		int samplesToMove = m_SampleCnt - samplesRead;
		int16_t* dest = m_PcmData.data();
		int16_t* src = m_PcmData.data() + samplesRead;

		// src/dest regions overlap when compacting the buffer toward index 0.
		memmove( dest, src, samplesToMove * AUDIO_BYTES_PER_SAMPLE);
		m_SampleCnt = samplesToMove;
	}
}

//============================================================================
int AudioSampleBuf::writeSamples( int16_t* srcSamplesBuf, int sampleCnt )
{
	int writtenSamples = 0;
	if( sampleCnt > m_MaxSamples )
	{
		if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_ERROR, "AudioSampleBuf::writeSamples ERROR samplesToWrite %d greater than max samples %d", sampleCnt, m_MaxSamples );
		return 0;
	}
	else if( sampleCnt <= ( m_MaxSamples - m_SampleCnt ) )
	{
		// can just append
		memcpy( &m_PcmData[ m_SampleCnt ], srcSamplesBuf, sampleCnt * AUDIO_BYTES_PER_SAMPLE );
		samplesWereWritten( sampleCnt );
		writtenSamples = sampleCnt;
	}
	else
	{
		// make room for samples
		int samplesToRemove = std::abs( m_MaxSamples - ( m_SampleCnt + sampleCnt ) );
		if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_WARNING, "AudioSampleBuf::writeSamples removing %d samples to fit %d samples", samplesToRemove, sampleCnt );
		samplesWereRead( samplesToRemove );
		memcpy( &m_PcmData[ m_SampleCnt ], srcSamplesBuf, sampleCnt * AUDIO_BYTES_PER_SAMPLE );
		samplesWereWritten( sampleCnt );
		
		writtenSamples = sampleCnt;
	}

	return writtenSamples;
}

//============================================================================
void AudioSampleBuf::samplesWereWritten( int sampleCnt )		
{ 
	if( sampleCnt <= 0 )
	{
		return;
	}

	m_SampleCnt += sampleCnt;
	const int vectorMaxSamples = static_cast<int>( m_PcmData.size() );
	const int safeMaxSamples = std::min( m_MaxSamples, vectorMaxSamples );
	if( m_SampleCnt > safeMaxSamples )
	{
		if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_WARNING, "AudioSampleBuf::samplesWereWritten clamping m_SampleCnt %d to %d", m_SampleCnt, safeMaxSamples );
		m_SampleCnt = safeMaxSamples;
	}
}

//============================================================================
int16_t AudioSampleBuf::getLastSample( void )
{
	if( m_SampleCnt )
	{
		return m_PcmData[ m_SampleCnt - 1 ];
	}

	return 0;
}

//============================================================================
int AudioSampleBuf::getAudioDurationMs( int sampleRate )
{
	return AudioUtils::audioDurationMs( sampleRate, m_SampleCnt );
}

//============================================================================
void AudioSampleBuf::truncateSamples( int sampleCnt )
{
	if( m_SampleCnt > sampleCnt )
	{
		samplesWereRead( m_SampleCnt - sampleCnt );
	}
}