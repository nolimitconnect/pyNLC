//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "GuiAudioMgr.h"

#include "AppCommon.h"
#include "AppSettings.h"
#include "GuiAudioLevelCallback.h"

#include <libaudio-nlc/AudioMgr.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxTime.h>

//============================================================================
GuiAudioMgr::GuiAudioMgr( AppCommon& app )
    : m_MyApp( app )
    , ToGuiHardwareControlInterface()
    , m_AudioMgr( AudioMgr::getInstance() )
    , m_AudioLevelPeekTimer( new QTimer( this ) )
{
	m_AudioLevelPeekTimer->setInterval( 500 );
	connect( m_AudioLevelPeekTimer, SIGNAL(timeout()), this, SLOT(slotAudioPeekTimeout()) );

    connect( this, &GuiAudioMgr::signalUpdateWantMicrophoneCount, this, &GuiAudioMgr::slotUpdateWantMicrophoneCount, Qt::QueuedConnection );
    connect( this, &GuiAudioMgr::signalUpdateSpeakerOutputCount, this, &GuiAudioMgr::slotUpdateWantSpeakerCount, Qt::QueuedConnection );

    m_AudioMgr.setEnablePeekMicAmplitude( true ); // so we have mic amplitude to show in title bar
}

//============================================================================
void GuiAudioMgr::audioIoSystemStartup()
{
    if( !isAudioInitialized() )
    {
        int startTime = GetApplicationAliveMs();
        LogMsg( LOG_DEBUG, "%s begin at %d", __func__, startTime );

        m_AudioMgr.audioIoSystemStartup(); // initialize the audio manager
        int endTime = GetApplicationAliveMs();
        LogMsg( LOG_DEBUG, "%s took %d ms at %d", __func__, endTime - startTime, endTime );

        std::string savedOutDevice = m_MyApp.getAppSettings().getSoundOutDevice();
        if(!savedOutDevice.empty())
        {
            m_AudioMgr.setAudioOutDevice( savedOutDevice );
        }

        std::string savedInDevice = m_MyApp.getAppSettings().getSoundInDevice();
        if(!savedInDevice.empty())
        {
            m_AudioMgr.setAudioInDevice( savedInDevice );
        }

        m_MyApp.wantToGuiHardwareCtrlCallbacks( this, true );

        bool mutedMic = m_MyApp.getAppSettings().getIsMicrophoneMuted();
        m_MyApp.fromGuiMuteMicrophone( mutedMic );

        bool mutedSpeaker = m_MyApp.getAppSettings().getIsSpeakerMuted();
        m_MyApp.fromGuiMuteSpeaker( mutedSpeaker );
    }
}

//============================================================================
void GuiAudioMgr::audioIoSystemShutdown()
{
    if( isAudioInitialized() )
    {
        m_MyApp.wantToGuiHardwareCtrlCallbacks( this, false );
	    m_AudioMgr.audioIoSystemShutdown();
    }
}

//============================================================================
bool GuiAudioMgr::isAudioInitialized()
{
    return m_AudioMgr.isAudioInitialized();
}

//============================================================================
bool GuiAudioMgr::getIsMicrophoneAvailable( void )
{
    return m_AudioMgr.getIsMicrophoneAvailable();
}

//============================================================================
int GuiAudioMgr::getWantMicrophoneCount( void )
{
    return m_AudioMgr.getWantMicrophoneCount();
}

//============================================================================
int GuiAudioMgr::getWantSpeakerCount( void )
{
    return m_AudioMgr.getWantSpeakerCount();
}

//============================================================================
bool GuiAudioMgr::getIsMicrophoneRunning( void )
{
    return m_AudioMgr.getIsMicrophoneRunning();
}

//============================================================================ 
void GuiAudioMgr::setIsMicrophoneMuted( bool micMuted )
{
    m_AudioMgr.setIsMicrophoneMuted( micMuted );
}

//============================================================================ 
bool GuiAudioMgr::getIsMicrophoneMuted( void )
{
    return m_AudioMgr.getIsMicrophoneMuted();
}

//============================================================================
bool GuiAudioMgr::getIsSpeakerAvailable( void )
{
    return m_AudioMgr.getIsSpeakerAvailable();
}

//============================================================================
bool GuiAudioMgr::getIsSpeakerRunning( void )
{
    return m_AudioMgr.getIsSpeakerRunning();
}
  
//============================================================================
void GuiAudioMgr::setIsSpeakerMuted( bool speakerMuted )
{
    m_AudioMgr.setIsSpeakerMuted( speakerMuted );
}

//============================================================================
bool GuiAudioMgr::getIsSpeakerMuted( void )
{
    return m_AudioMgr.getIsSpeakerMuted();
}

//============================================================================
void GuiAudioMgr::toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput )
{
    m_AudioMgr.toGuiWantMicrophoneRecording( mediaModule, wantMicInput );
}

//============================================================================
void GuiAudioMgr::toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput )
{
    m_AudioMgr.toGuiWantSpeakerOutput( mediaModule, wantSpeakerOutput );
}

//============================================================================
void GuiAudioMgr::toGuiUpdateWantMicrophoneCount( int wantMicCnt )
{
    emit signalUpdateWantMicrophoneCount( wantMicCnt );
}

//============================================================================
void GuiAudioMgr::toGuiUpdateWantSpeakerCount( int wantSpeakerCnt )
{
    emit signalUpdateSpeakerOutputCount( wantSpeakerCnt );
}

//============================================================================
void GuiAudioMgr::wantAudioOutSpaceAvailableCallback( AudioCallbackSpaceAvailable* callback, bool want )
{
    m_AudioMgr.wantAudioOutSpaceAvailableCallback( callback, want );
}

//============================================================================
int GuiAudioMgr::toGuiModuleAudioFrame( EMediaModule mediaModule, int16_t * pu16PcmData, int pcmDataLenInBytes )
{
    return m_AudioMgr.toGuiModuleAudioFrame( mediaModule, pu16PcmData, pcmDataLenInBytes );
}

//============================================================================
int GuiAudioMgr::toGuiPlayerNlcAudio( EMediaModule mediaModule, float* audioDataFloat, int audioDataLenInBytes )
{
    return m_AudioMgr.toGuiPlayerNlcAudio( mediaModule, audioDataFloat, audioDataLenInBytes );
}

//============================================================================
float GuiAudioMgr::toGuiGetAudioDelaySeconds( EMediaModule mediaModule )
{
    return m_AudioMgr.toGuiGetAudioDelaySeconds( mediaModule );
}

//============================================================================
float GuiAudioMgr::toGuiGetAudioCacheFreeSpaceBytes( EMediaModule mediaModule )
{
    return m_AudioMgr.toGuiGetAudioCacheFreeSpaceBytes( mediaModule );
}

//============================================================================
float GuiAudioMgr::toGuiGetAudioCacheMaxSeconds( EMediaModule mediaModule )
{
    return m_AudioMgr.toGuiGetAudioCacheMaxSeconds( mediaModule );
}

//============================================================================
void GuiAudioMgr::slotUpdateWantMicrophoneCount( int wantMicCnt )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	for( auto toGuiClient : m_AudioLevelClientList )
	{
		toGuiClient->callbackWantMicrophoneCount( wantMicCnt );
	}
}

//============================================================================
void GuiAudioMgr::slotUpdateWantSpeakerCount( int wantSpeakerCnt )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	for( auto toGuiClient : m_AudioLevelClientList )
	{
		toGuiClient->callbackWantSpeakerCount( wantSpeakerCnt );
	}
}

//============================================================================
// NLC player related functions
//============================================================================
void GuiAudioMgr::setPlayerNlcActive( bool isActive )
{
    m_AudioMgr.setPlayerNlcActive( isActive );
}

//============================================================================
bool GuiAudioMgr::getPlayerNlcActive( void )
{
    return m_AudioMgr.getPlayerNlcActive();
}

//============================================================================
void GuiAudioMgr::setPlayerNlcAcceptInput( bool acceptInput )
{
    m_AudioMgr.setPlayerNlcAcceptInput( acceptInput );
}

//============================================================================
bool GuiAudioMgr::getPlayerNlcAcceptInput( void )
{
    return m_AudioMgr.getPlayerNlcAcceptInput();
}

//============================================================================
void GuiAudioMgr::clearPlayerNlcBuffers( void )
{
    m_AudioMgr.clearPlayerNlcBuffers();
}

//============================================================================
// Visualization related functions
//============================================================================
void GuiAudioMgr::wantAudioInVisualization( bool wanted )
{
    m_AudioMgr.wantAudioInVisualization( wanted );
}

//============================================================================
void GuiAudioMgr::wantAudioOutVisualization( bool wanted )
{
    m_AudioMgr.wantAudioOutVisualization( wanted );
}

//============================================================================
AudioFrameAecBuffer& GuiAudioMgr::getAudioInRawWaveformBuffer( void )
{
    return m_AudioMgr.getAudioInRawWaveformBuffer();
}

//============================================================================
AudioFrameAecBuffer& GuiAudioMgr::getAudioAecProcessedWaveformBuffer( void )
{
    return m_AudioMgr.getAudioAecProcessedWaveformBuffer();
}

//============================================================================
AudioFrameAecBuffer& GuiAudioMgr::getSpeakerOutWaveformBuffer( void )
{
    return m_AudioMgr.getSpeakerOutWaveformBuffer();
}

//============================================================================
// Audio Device related functions
//============================================================================
std::vector<std::string>& GuiAudioMgr::getAudioInDevices( void )
{
    return m_AudioMgr.getAudioInDevices();
}

//============================================================================
std::vector<std::string>& GuiAudioMgr::getAudioOutDevices( void )
{
    return m_AudioMgr.getAudioOutDevices();
}

//============================================================================
bool GuiAudioMgr::setAudioInDevice( std::string deviceDescription )
{
    return m_AudioMgr.setAudioInDevice( deviceDescription );
}

//============================================================================
bool GuiAudioMgr::setAudioOutDevice( std::string deviceDescription )
{
    return m_AudioMgr.setAudioOutDevice( deviceDescription );
}

//============================================================================
// Microphone level callback related functions
//============================================================================

//============================================================================
int GuiAudioMgr::getAudioInPeakAmplitude( void )
{
    return m_AudioMgr.getAudioInPeakAmplitude();
}

//============================================================================
void GuiAudioMgr::wantMicrophoneLevelCallbacks( GuiAudioLevelCallback* client, bool enable )
{
	for( auto iter = m_AudioLevelClientList.begin(); iter != m_AudioLevelClientList.end(); ++iter )
	{
        if( client == *iter )
		{
			if( enable )
			{
				return;
			}
			else
			{
				m_AudioLevelClientList.erase( iter );
				if( 0 == m_AudioLevelClientList.size() )
				{
					m_AudioLevelPeekTimer->stop();
				}

				return;
			}
		}
	}

	if( enable )
	{
		m_AudioLevelClientList.emplace_back( client );
		if( 1 == m_AudioLevelClientList.size() )
		{
			m_AudioLevelPeekTimer->start();
		}
	}
}

//============================================================================
void GuiAudioMgr::slotAudioPeekTimeout( void )
{
	if( m_AudioLevelClientList.empty() )
	{
		return;
	}

	int micLevel = getIsMicrophoneRunning() && !getIsMicrophoneMuted() ? getAudioInPeakAmplitude() : 0;

	for( auto& client : m_AudioLevelClientList )
	{
		client->callbackGuiMicrophoneLevel( micLevel );
	}
}

//============================================================================
// Configure
//============================================================================
void GuiAudioMgr::setNoAecLoopbackEnable( bool enable )
{
    m_AudioMgr.setNoAecLoopbackEnable( enable );
}

//============================================================================
void GuiAudioMgr::setWithAecLoopbackEnable( bool enable )
{
    m_AudioMgr.setWithAecLoopbackEnable( enable );
}

//============================================================================
void GuiAudioMgr::setAgcEnabled( bool enable )
{
    m_AudioMgr.setAgcEnabled( enable );
}

//============================================================================
void GuiAudioMgr::setNoiseSuppressionEnabled( bool enable )
{
    m_AudioMgr.setNoiseSuppressionEnabled( enable );
}

//============================================================================
// Audio test related functions
//============================================================================
void GuiAudioMgr::wantAudioDelayTestCallbacks( AudioDelayTestCallback* client, bool enable )
{
    m_AudioMgr.wantAudioDelayTestCallbacks( client, enable );
}

//============================================================================
bool GuiAudioMgr::runEchoDelayTest( void )
{
    return m_AudioMgr.runEchoDelayTest();
}

//============================================================================
void GuiAudioMgr::setEchoDelayParam( int delayMs )
{
    m_AudioMgr.setEchoDelayParam( delayMs );
}

//============================================================================
void GuiAudioMgr::setEnableSpeakerTestTone( int enableTestTone )
{
    m_AudioMgr.setEnableSpeakerTestTone( enableTestTone );
}

//============================================================================
void GuiAudioMgr::playTestFile( std::string testFile )
{
    m_AudioMgr.playTestFile( testFile );
}
