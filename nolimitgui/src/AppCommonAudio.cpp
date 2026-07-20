//============================================================================
// Copyright (C) 2009 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppCommon.h"
#include "SoundFxMgr.h"

#include <P2PEngine/P2PEngine.h>
#include <MediaProcessor/MediaProcessor.h>

#include <GuiInterface/ICamCapture.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include <QTimer>

//============================================================================
void AppCommon::scheduleHardwareCtrlStateReplay( void )
{
	if( m_ToGuiHardwareCtrlReplayPending || VxIsAppShuttingDown() )
	{
		return;
	}

	m_ToGuiHardwareCtrlReplayPending = true;
	QTimer::singleShot( 0, this, [this]()
	{
		m_ToGuiHardwareCtrlReplayPending = false;

		if( VxIsAppShuttingDown() )
		{
			return;
		}

		if( m_ToGuiHardwareCtrlBusy )
		{
			scheduleHardwareCtrlStateReplay();
			return;
		}

		replayHardwareCtrlState();
	} );
}

//============================================================================
void AppCommon::replayHardwareCtrlState( void )
{
	if( m_ToGuiHardwareCtrlBusy || VxIsAppShuttingDown() )
	{
		return;
	}

	const bool wantMicInput = m_AudioMgr.getIsMicrophoneRunning();
	const bool micMuted = m_AudioMgr.getIsMicrophoneMuted();
	const bool wantSpeakerOutput = m_AudioMgr.getIsSpeakerRunning();
	const bool speakerMuted = m_AudioMgr.getIsSpeakerMuted();

	const bool wantVideoCapture = ICamCapture::getICamCapture().isCamCaptureRequested();
	const bool camEnabled = ICamCapture::getICamCapture().getCamCaptureEnable();
	const bool camCaptureRunning = ICamCapture::getICamCapture().isCamCaptureRunning();

	m_ToGuiHardwareCtrlBusy = true;
	for( auto toGuiClient : m_ToGuiHardwareCtrlList )
	{
		toGuiClient->callbackToGuiWantMicrophoneRecording( wantMicInput );
		toGuiClient->callbackToGuiMicrophoneMuted( micMuted );
		toGuiClient->callbackToGuiWantSpeakerOutput( wantSpeakerOutput );
		toGuiClient->callbackToGuiSpeakerMuted( speakerMuted );
		toGuiClient->callbackToGuiWantVideoCapture( wantVideoCapture );
		toGuiClient->callbackToGuiCameraEnable( camEnabled );
		toGuiClient->callbackToGuiCaptureRunning( camCaptureRunning );
	}
	m_ToGuiHardwareCtrlBusy = false;
}

//============================================================================
bool AppCommon::toGuiIsMicrophoneDeviceAvailable( void )
{
	if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_VERBOSE, "AppCommon::%s", __func__ );
	if( VxIsAppShuttingDown() )
	{
		return false;
	}

	return m_AudioMgr.getIsMicrophoneAvailable();
}

//============================================================================
void AppCommon::toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput )
{
	if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_VERBOSE, "AppCommon::%s %d", __func__, wantMicInput );
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalWantMicrophoneRecording( mediaModule, wantMicInput );
}

//============================================================================
void AppCommon::slotInternalWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput )
{
    bool wasMicAvailable = m_AudioMgr.getIsMicrophoneRunning();
    m_AudioMgr.toGuiWantMicrophoneRecording( mediaModule, wantMicInput );
    bool isMicAvailable = m_AudioMgr.getIsMicrophoneRunning();
    if( wasMicAvailable != isMicAvailable )
    {
        m_ToGuiHardwareCtrlBusy = true;
        for( auto toGuiClient : m_ToGuiHardwareCtrlList )
        {
            toGuiClient->callbackToGuiWantMicrophoneRecording( wantMicInput );
        }

        m_ToGuiHardwareCtrlBusy = false;
        if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_VERBOSE, "AppCommon::%s %d done", __func__, isMicAvailable );
    }
}

//============================================================================
void AppCommon::toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

    if( mediaModule != eMediaModuleSoundFx )
	{
		if( LogEnabled( eLogVoice ) ) LogModule( eLogVoice, LOG_VERBOSE, "AppCommon::%s want speaker? %d module %s", __func__, wantSpeakerOutput, DescribeMediaModule( mediaModule ) );
	}

	emit signalInternalWantSpeakerOutput( mediaModule, wantSpeakerOutput );
}

//============================================================================
void AppCommon::slotInternalWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

    bool wasSpeakerAvailable = m_AudioMgr.getIsSpeakerRunning();
    m_AudioMgr.toGuiWantSpeakerOutput( mediaModule, wantSpeakerOutput );
    bool isSpeakerAvailable = m_AudioMgr.getIsSpeakerRunning();

    if( wasSpeakerAvailable != isSpeakerAvailable )
    {
        if( m_ToGuiHardwareCtrlBusy )
        {
			LogMsg( LOG_WARN, "AppCommon::%s ToGuiHardware busy; skipping nested callback", __func__ );
			scheduleHardwareCtrlStateReplay();
			return;
        }

        m_ToGuiHardwareCtrlBusy = true;
        for( auto toGuiClient : m_ToGuiHardwareCtrlList )
        {
            toGuiClient->callbackToGuiWantSpeakerOutput( isSpeakerAvailable );
        }

        m_ToGuiHardwareCtrlBusy = false;
    }
}

//============================================================================
//! playback audio
int AppCommon::toGuiModuleAudioFrame( EMediaModule mediaModule, int16_t * pu16PcmData, int pcmDataLenInBytes )
{
    if( VxIsAppShuttingDown() )
    {
        return 0;
    }

    return m_AudioMgr.toGuiModuleAudioFrame( mediaModule, pu16PcmData, pcmDataLenInBytes );
}

//============================================================================
//! playback audio
int AppCommon::toGuiPlayerNlcAudio( EMediaModule mediaModule, float* audioDataFloat, int audioDataLenInBytes )
{
	if( VxIsAppShuttingDown() )
	{
		return 0;
	}

	return m_AudioMgr.toGuiPlayerNlcAudio( mediaModule, audioDataFloat, audioDataLenInBytes );
}

//============================================================================
float AppCommon::toGuiGetAudioDelaySeconds( EMediaModule mediaModule )
{
	if( VxIsAppShuttingDown() )
	{
		return 0.0f;
	}

	return m_AudioMgr.toGuiGetAudioDelaySeconds( mediaModule );
}

//============================================================================
float AppCommon::toGuiGetAudioCacheFreeSpaceBytes( EMediaModule mediaModule )
{
	if( VxIsAppShuttingDown() )
	{
		return 0.0f;
	}

	return m_AudioMgr.toGuiGetAudioCacheFreeSpaceBytes( mediaModule );
}

//============================================================================
float AppCommon::toGuiGetAudioCacheMaxSeconds( EMediaModule mediaModule )
{
	if( VxIsAppShuttingDown() )
	{
		return 0.0f;
	}

	return m_AudioMgr.toGuiGetAudioCacheMaxSeconds( mediaModule );
}

//============================================================================
void AppCommon::toGuiUpdateWantMicrophoneCount( int wantMicCnt )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

    m_AudioMgr.toGuiUpdateWantMicrophoneCount( wantMicCnt );
}

//============================================================================
void AppCommon::toGuiUpdateWantSpeakerCount( int wantSpeakerCnt )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}
    
    m_AudioMgr.toGuiUpdateWantSpeakerCount( wantSpeakerCnt );
}

//============================================================================
/// Mute/Unmute microphone
void AppCommon::fromGuiMuteMicrophone( bool muteMic )
{
	m_AudioMgr.setIsMicrophoneMuted( muteMic );
    getEngine().fromGuiMuteMicrophone( muteMic );

	if( m_ToGuiHardwareCtrlBusy )
	{
		LogMsg( LOG_WARN, "AppCommon::%s ToGuiHardware busy; skipping nested callback", __func__ );
		scheduleHardwareCtrlStateReplay();
		return;
	}

	m_ToGuiHardwareCtrlBusy = true;
	for( auto toGuiClient : m_ToGuiHardwareCtrlList )
	{
		toGuiClient->callbackToGuiMicrophoneMuted( muteMic );
	}

	m_ToGuiHardwareCtrlBusy = false;
}

//============================================================================
/// Returns true if microphone is muted
bool AppCommon::fromGuiIsMicrophoneMuted( void )
{
    return getEngine().fromGuiIsMicrophoneMuted();
}

//============================================================================
/// Mute/Unmute speaker
void AppCommon::fromGuiMuteSpeaker( bool muteSpeaker )
{
	m_AudioMgr.setIsSpeakerMuted( muteSpeaker );
    getEngine().fromGuiMuteSpeaker( muteSpeaker );

	if( m_ToGuiHardwareCtrlBusy )
	{
		LogMsg( LOG_WARN, "AppCommon::%s ToGuiHardware busy; skipping nested callback", __func__ );
		scheduleHardwareCtrlStateReplay();
		return;
	}

	m_ToGuiHardwareCtrlBusy = true;
	for( auto& toGuiClient : m_ToGuiHardwareCtrlList )
	{
		toGuiClient->callbackToGuiSpeakerMuted( muteSpeaker );
	}

	m_ToGuiHardwareCtrlBusy = false;
}

//============================================================================
/// Returns true if speaker is muted
bool AppCommon::fromGuiIsSpeakerMuted( void )
{
    return getEngine().fromGuiIsSpeakerMuted();
}
