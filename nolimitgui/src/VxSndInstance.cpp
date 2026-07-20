//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxSndInstance.h"

#include "AppCommon.h"
#include "GuiAudioMgr.h"
#include "SoundFxMgr.h"
#include "VxResourceToRealFile.h"

#include "libwav-decoder/WavMgr.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

namespace
{
	const char* g_SndResourcePaths[] = {
			":/AppRes/Resources/snd_joshua_no.wav",			// eSndDefNone, 0
			":/AppRes/Resources/snd_joshua_no.wav",			// eSndDefIgnore,1
			":/AppRes/Resources/snd_cancel1.wav",			// eSndDefCancel,2
			":/AppRes/Resources/snd_alarm2.wav",			// eSndDefAlarmPleasant,3
			":/AppRes/Resources/snd_alarmclock.wav",		// eSndDefAlarmAnoying,4
			":/AppRes/Resources/snd_keyclick2.wav",			// eSndDefButtonClick,5
			":/AppRes/Resources/snd_choice1.wav",			// eSndDefChoice1,6
			":/AppRes/Resources/snd_choice2.wav",			// eSndDefChoice2,7
			":/AppRes/Resources/snd_morse_code.wav",		// eSndDefSending,8
			":/AppRes/Resources/snd_notify1.wav",			// eSndDefNotify1,9
			":/AppRes/Resources/snd_notify2.wav",			// eSndDefNotify2,10
			":/AppRes/Resources/snd_papershredder1.wav",	// eSndDefPaperShredder,11
			":/AppRes/Resources/snd_phone_ring1.wav",		// eSndDefPhoneRing1,12
			":/AppRes/Resources/snd_stomp_hall.wav",		// eSndDefReject,13
			":/AppRes/Resources/snd_ouhouh.wav",			// eSndDefShare,14
			":/AppRes/Resources/snd_byebye.wav",			// eSndDefByeBye,15
			":/AppRes/Resources/snd_beep_spring.wav",		// eSndDefMessageArrived,16
			":/AppRes/Resources/snd_microwave_beep.wav",	// eSndDefOfferAccepted,17
			":/AppRes/Resources/snd_notify2.wav",			// eSndDefOfferRejected,18
			":/AppRes/Resources/snd_camera_snapshot.wav",	// eSndDefCameraClick,19
			":/AppRes/Resources/snd_busy_phone_signal.wav",	// eSndDefBusy,20
			":/AppRes/Resources/snd_sonar.wav",				// eSndDefOfferStillWaiting,21
			":/AppRes/Resources/snd_file_xfer_complete.wav",// eSndDefFileXferComplete,22
			":/AppRes/Resources/snd_bike_bell.wav",			// eSndDefUserBellMessage,23
			":/AppRes/Resources/snd_neck_snap.wav",			// eSndDefNeckSnap,24
			":/AppRes/Resources/snd_yes.wav",				// eSndDefYes,25

			":/AppRes/Resources/snd_byebye.wav",			// eSndDefAppShutdown,26
	};
};

//============================================================================
VxSndInstance::VxSndInstance( ESndDef sndDef, QObject* parent )
: QObject( parent )
, m_SndDef( sndDef )
{
}

//============================================================================
VxSndInstance::~VxSndInstance()
{
	stopPlay();
}

//============================================================================
bool VxSndInstance::startPlay( bool loopContinuous )
{
	if( ( eSndDefNone >= m_SndDef )
		|| ( eMaxSndDef <= m_SndDef ) )
	{
		// invalid or no sound
		return false;
	}

	if( !m_IsInitialized )
	{
		if( !initSndInstance() )
		{
			return false;
		}
	}

	if( !m_IsPlaying )
	{
		m_IsPlaying = true;
		m_PlaySndIdx = 0;
		wantAudioCallbacks( true );
	}

	return true;
}

//============================================================================
void VxSndInstance::stopPlay( void )
{
	if( m_IsPlaying )
	{
		wantAudioCallbacks( false );
		m_IsPlaying = false;
		m_PlaySndIdx = 0;
	}
}

//============================================================================
bool VxSndInstance::initSndInstance( void )
{
	if( m_IsInitialized )
	{
		return true;
	}

	QString resourceFile = g_SndResourcePaths[ m_SndDef ];
	VxResourceToRealFile realFile( resourceFile );

	m_WavFileName = realFile.getRealFilePathAndName();
	std::vector<int16_t> wavBytes;
	int wavRate{ 0 };
	int wavChannels{ 0 };
	int wavBitsPerSample{ 0 };
	m_IsInitialized = WavMgr::readWavFile( m_WavFileName, wavBytes, wavRate, wavChannels, wavBitsPerSample );
	if( wavChannels != 1 )
	{
		m_IsInitialized = false;
		LogMsg( LOG_ERROR, "%s only mono wav files allowed not %d channels", __func__, wavChannels );
		return false;
	}

	if( wavBitsPerSample != 16 )
	{
		m_IsInitialized = false;
		LogMsg( LOG_ERROR, "%s only 16 bit pcm wav files allowed", __func__ );
		return false;
	}

	m_WavSamples.clear();
	int16_t* bytesAs16bit = (int16_t*)wavBytes.data();
	int pcmSampleCnt = wavBytes.size();
	
	if( wavRate == AUDIO_DEVICE_SAMPLE_RATE )
	{
		std::copy( bytesAs16bit, bytesAs16bit + pcmSampleCnt, std::back_inserter( m_WavSamples ) );
	}
	else if( wavRate == 8000 && AUDIO_DEVICE_SAMPLE_RATE == 24000 )
	{
		for( int i = 0; i < pcmSampleCnt; ++i )
		{
			m_WavSamples.emplace_back( *bytesAs16bit );
			m_WavSamples.emplace_back( *bytesAs16bit );
			m_WavSamples.emplace_back( *bytesAs16bit );
			bytesAs16bit += 1;
		}
	}
	else if( wavRate == 8000 && AUDIO_DEVICE_SAMPLE_RATE == 16000 )
	{	
		for( int i = 0; i < pcmSampleCnt; ++i )
		{
			m_WavSamples.emplace_back( *bytesAs16bit );
			m_WavSamples.emplace_back( *bytesAs16bit );
			bytesAs16bit += 1;
		}
	}
	else
	{
		double ratio = (double)wavRate / AUDIO_DEVICE_SAMPLE_RATE; 
		size_t numFrames = wavBytes.size() / wavChannels;
    	size_t newNumFrames = (size_t)(numFrames / ratio);

		for (size_t i = 0; i < pcmSampleCnt; ++i) 
		{
			double sourcePos = i * ratio;
			size_t indexLow = (size_t)std::floor(sourcePos);
			size_t indexHigh = (indexLow + 1 < numFrames) ? indexLow + 1 : indexLow;
			float weightHigh = (float)(sourcePos - indexLow);
			float weightLow = 1.0f - weightHigh;

			for (int ch = 0; ch < wavChannels; ++ch) 
			{
				int16_t sample = (int16_t)(wavBytes[indexLow * wavChannels + ch] * weightLow +
										wavBytes[indexHigh * wavChannels + ch] * weightHigh);
				m_WavSamples.emplace_back(sample);
			}
		}
	}
	
	if( m_WavSamples.size() == 0 )
	{
		m_IsInitialized = false;
		LogMsg( LOG_ERROR, "%s unknown rate %d", __func__, wavRate );
		return false;
	}

	int fillCnt{ 0 };
	int remainder = m_WavSamples.size() % AUDIO_SAMPLES_PER_FRAME;
	if( remainder )
	{
		fillCnt = AUDIO_SAMPLES_PER_FRAME - remainder;
	}

	// fill with zeros to round up to an even frame
	for( int i = 0; i < fillCnt; ++i )
	{
		m_WavSamples.emplace_back( 0 );
	}

	return m_IsInitialized;
}

//============================================================================
void VxSndInstance::callbackAudioOutSpaceAvail( int freeSpaceLenBytes )
{
	int samplesRequested = freeSpaceLenBytes / AUDIO_BYTES_PER_SAMPLE;
	// only play same length as samplesRequested
	int samplesAvailable = m_WavSamples.size() - m_PlaySndIdx;
	if( samplesAvailable >= samplesRequested )
	{
		int16_t* buf = m_WavSamples.data();
		buf += m_PlaySndIdx;
		GetAppInstance().getAudioMgr().toGuiModuleAudioFrame( eMediaModuleSoundFx, buf, freeSpaceLenBytes );
		m_PlaySndIdx += samplesRequested;
		if( m_PlaySndIdx < m_WavSamples.size() )
		{
			// still more to be read
			return;
		}
	}

	// all done
	m_PlaySndIdx = 0;
	wantAudioCallbacks( false );
}

//============================================================================
void VxSndInstance::wantAudioCallbacks( bool wantCallbacks )
{
	if( VxIsAppShuttingDown() )
	{
		m_EffectsAudioCallbacksRequested = false;
		return;
	}

	if( m_EffectsAudioCallbacksRequested != wantCallbacks )
	{
		m_EffectsAudioCallbacksRequested = wantCallbacks;
		GetAppInstance().getAudioMgr().wantAudioOutSpaceAvailableCallback( this, wantCallbacks );
		GetAppInstance().getAudioMgr().toGuiWantSpeakerOutput( eMediaModuleSoundFx, wantCallbacks );
	}
}
