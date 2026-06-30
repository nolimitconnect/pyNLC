//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioMgr.h"

#include "miniaudio/AudioUtils.h"

#include <GuiInterface/IToGui.h>

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

//============================================================================
// return true if any microphone device is available to be enabled
bool AudioMgr::toGuiIsMicrophoneDeviceAvailable( void )
{
    return m_MaMicDeviceCount > 0;
}

//============================================================================
// enable disable microphone data callback
void AudioMgr::toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput )
{
    if( LogEnabled( eLogVoice ) ) LogModule( eLogVoice, LOG_DEBUG, "AudioMgr::%s want mic? %d module %s", __func__, wantMicInput, DescribeMediaModule( mediaModule ) );
#if defined(Q_OS_ANDROID)
    if( wantMicInput && !GuiParams::requestPermission("android.permission.RECORD_AUDIO") )
    {
        LogMsg( LOG_ERROR, "AudioMgr::%s microphone permission denied; module %s", __func__, DescribeMediaModule( mediaModule ) );
        return;
    }
#endif // defined(Q_OS_ANDROID)

    bool found{ false };
    m_WantMicMutex.lock();
    size_t prevWantMicCnt = m_WantMicList.size();
    for( auto iter = m_WantMicList.begin(); iter != m_WantMicList.end(); iter++ )
    {
        if( *iter == mediaModule )
        {
            found = true;
            if( !wantMicInput )
            {
                m_WantMicList.erase( iter );
                break;
            }
        }
    }

    if( wantMicInput && !found )
    {
        m_WantMicList.emplace_back( mediaModule );
    }

    size_t wantMicCnt = m_WantMicList.size();
    m_WantMicMutex.unlock();

    if( prevWantMicCnt != wantMicCnt )
    {
        if( !prevWantMicCnt && wantMicCnt )
        {
            // mic count went from 0 to 1, enable mic
            enableAudioIn( true );
        }
        else if( prevWantMicCnt && !wantMicCnt )
        {
            // mic count went from 1 to 0, disable mic
            enableAudioIn( false );
        }

        IToGui::getIToGui().toGuiUpdateWantMicrophoneCount( static_cast<int>(wantMicCnt) );
    }
}

//============================================================================
// enable disable sound out
void AudioMgr::toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput )
{
    bool found{ false };

    if( eMediaModulePlayerNlc == mediaModule && wantSpeakerOutput )
    {
        m_PlayerNlcSpeakerDisablePending = false;
    }

    if( eMediaModulePlayerNlc == mediaModule && !wantSpeakerOutput )
    {
        if( getAudioOutPipelineQueuedSampleCnt() > 0 )
        {
            m_PlayerNlcSpeakerDisablePending = true;
            if( !m_AudioOutDisablePending )
            {
                requestDeferredAudioOutDisable();
            }

            if( !m_AudioOutDisableTimer.isActive() )
            {
                m_AudioOutDisableTimer.start(10);
            }

            if( LogEnabled( eLogVoice ) ) LogModule( eLogVoice, LOG_DEBUG,
                "AudioMgr::%s defer NlcPlayer speaker disable until queued audio drains", __func__ );
            return;
        }
    }

    m_WantSpeakerMutex.lock();
    size_t prevWantSpeakerCnt = m_WantSpeakerList.size();
    for( auto iter = m_WantSpeakerList.begin(); iter != m_WantSpeakerList.end(); iter++ )
    {
        if( *iter == mediaModule )
        {
            found = true;
            if( !wantSpeakerOutput )
            {
                m_WantSpeakerList.erase( iter );
                break;
            }
        }
    }

    if( wantSpeakerOutput && !found )
    {
        m_WantSpeakerList.emplace_back( mediaModule );
    }

    if( !wantSpeakerOutput && eMediaModulePlayerNlc == mediaModule )
    {
        m_PlayerNlcSpeakerDisablePending = false;
    }

    size_t wantSpeakerCnt = m_WantSpeakerList.size();
    m_WantSpeakerMutex.unlock();

    if( prevWantSpeakerCnt != wantSpeakerCnt )
    {
        if( !prevWantSpeakerCnt && wantSpeakerCnt )
        {
            // speaker count went from 0 to 1, enable speaker
            enableAudioOut( true );
        }
        else if( prevWantSpeakerCnt && !wantSpeakerCnt )
        {
            // speaker count went from 1 to 0, disable speaker
            enableAudioOut( false );
        }

        IToGui::getIToGui().toGuiUpdateWantSpeakerCount( static_cast<int>(wantSpeakerCnt) );
    }

    if( LogEnabled( eLogVoice ) ) LogModule( eLogVoice, LOG_DEBUG, "AudioMgr::%s want speaker? %d module %s cnt %d", __func__, wantSpeakerOutput, DescribeMediaModule( mediaModule ), wantSpeakerCnt );
}

//============================================================================
void AudioMgr::wantAudioOutSpaceAvailableCallback( AudioCallbackSpaceAvailable* callback, bool want )
{
    auto iter = std::find( m_AudioOutSpaceAvailableClientList.begin(), m_AudioOutSpaceAvailableClientList.end(), callback );
    if( want )
    {
        if( iter == m_AudioOutSpaceAvailableClientList.end() )
        {
            m_AudioOutSpaceAvailableClientList.emplace_back( callback );
        }
    }
    else
    {    
        if( iter != m_AudioOutSpaceAvailableClientList.end() )
        {
            m_AudioOutSpaceAvailableClientList.erase( iter );
        }
    }
}