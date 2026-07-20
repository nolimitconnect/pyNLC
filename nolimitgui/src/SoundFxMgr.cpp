//============================================================================
// Copyright (C) 2010 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "SoundFxMgr.h"

#include "AppCommon.h"
#include "AppSettings.h"

#include "VxSndInstance.h"

#include <CoreLib/VxDebug.h>

#include <QDebug>
#include <QCoreApplication>
#include <QMessageBox>

const char* DescribeSnd( ESndDef sndDef )
{
	switch( sndDef )
	{
	case eSndDefNone:
		return "Snd None";
	case eSndDefIgnore:
		return "Snd Ignore";
	case eSndDefCancel:
		return "Snd Cance";
	case eSndDefAlarmPleasant:
		return "Snd AlarmPleasant";
	case eSndDefAlarmAnoying:
		return "Snd AlarmAnoying";
	case eSndDefButtonClick:
		return "Snd ButtonClick";
	case eSndDefChoice1:
		return "Snd Choice1";
	case eSndDefChoice2:
		return "Snd Choice2";
	case eSndDefSending:
		return "Snd Sendin";
	case eSndDefNotify1:
		return "Snd Notify1";
	case eSndDefNotify2:
		return "Snd Notify2";
	case eSndDefPaperShredder:
		return "Snd PaperShredder";
	case eSndDefPhoneRing1:
		return "Snd PhoneRing1";
	case eSndDefReject:
		return "Snd Reject";
	case eSndDefShare:
		return "Snd Share";
	case eSndDefByeBye:
		return "Snd ByeBye";
	case eSndDefMessageArrived:
		return "Snd MessageArrived";
	case eSndDefOfferAccepted:
		return "Snd OfferAccepted";
	case eSndDefOfferRejected:
		return "Snd OfferRejected";
	case eSndDefCameraClick:
		return "Snd CameraClick";
	case eSndDefBusy:
		return "Snd Busy";
	case eSndDefOfferStillWaiting:
		return "Snd StillWaiting";
	case eSndDefFileXferComplete:
		return "Snd XferComplete";
	case eSndDefUserBellMessage:
		return "Snd BellMessage";
	case eSndDefNeckSnap:
		return "Snd NeckSnap";
	case eSndDefYes:
		return "Snd Yes";
	case eSndDefAppShutdown:
		return "Snd AppShutdown";
	default:
		return "Snd Invalid";
	}
}

//============================================================================
SoundFxMgr& GetSndFxMgrInstance( void )
{
	return GetAppInstance().getSoundFxMgr();
}

//============================================================================
SoundFxMgr::~SoundFxMgr()
{
	sndFxMgrShutdown();
}

//============================================================================ 
void SoundFxMgr::slotStartPhoneRinging( void )
{
	playSnd( eSndDefPhoneRing1, true );
}

//============================================================================ 
void SoundFxMgr::slotStopPhoneRinging( void )
{
	if( m_CurSndPlaying 
		&& ( eSndDefPhoneRing1 == m_CurSndPlaying->getSndDef() ) )
	{
		m_CurSndPlaying->stopPlay();
		m_CurSndPlaying = nullptr;
	}
}

//============================================================================
void SoundFxMgr::slotPlayNotifySound( void )
{
	playSnd( eSndDefNotify1, true );
}

//============================================================================
void SoundFxMgr::slotPlayShredderSound( void )
{
	playSnd( eSndDefPaperShredder, true );
}

//============================================================================
void SoundFxMgr::mutePhoneRing( bool bMute )
{
	m_MutePhoneRing = bMute;
	if( bMute )
	{
		slotStopPhoneRinging();
	}
}

//============================================================================
void SoundFxMgr::muteNotifySound( bool bMute )
{
	m_MuteNotifySnd = bMute;
}

//============================================================================
bool SoundFxMgr::sndFxMgrStartup( void )
{
	if( m_Initialized )
	{
		return true;
	}

    for( int i = 0; i < eMaxSndDef; i++ )
    {
        VxSndInstance* sndInstance = new VxSndInstance( (ESndDef)i, this );
        m_SndList.emplace_back( sndInstance );
    }

    m_Initialized = true;
	return true;
}

//============================================================================
bool SoundFxMgr::sndFxMgrShutdown( void )
{
	if( !m_SndList.isEmpty() )
	{
		for( auto sndInstance : m_SndList )
		{
			if( sndInstance )
			{
				sndInstance->stopPlay();
				delete sndInstance;
			}
		}

		m_SndList.clear();
	}

	m_CurSndPlaying = nullptr;
    m_Initialized = false;
	return true;
}

//============================================================================
VxSndInstance * SoundFxMgr::playSnd( ESndDef sndDef, bool loopContinuous  )
{
#ifdef DISABLE_AUDIO
    return nullptr;
#endif // DISABLE_AUDIO
    if(!m_Initialized)
    {
        return nullptr;
    }

	if( GetAppInstance().getAppSettings().getDisableAllSoundEffects() )
	{
		return nullptr;
	}

    if( !GetAppInstance().getIsAppInitialized() )
    {
        return nullptr;
    }

	if( LogEnabled( eLogUsers ) && sndDef != eSndDefButtonClick )
	{
		LogMsg( LOG_VERBOSE, "SoundFxMgr::%s play sound %s", __func__, DescribeSnd( sndDef ) );
	}

	if( m_MutePhoneRing 
		&& ( eSndDefPhoneRing1 == sndDef ) )
	{
		return m_SndList[ eSndDefNone ];
	}

	if( m_MuteNotifySnd 
		&& ( ( eSndDefNotify1 == sndDef ) || ( eSndDefNotify2 == sndDef ) ) )
	{
		return m_SndList[ eSndDefNone ];
	}

    if( ( sndDef < m_SndList.size() )
		&& ( 0 <= sndDef ) )
	{
		if( m_CurSndPlaying )
		{
			m_CurSndPlaying->stopPlay();
			m_CurSndPlaying = nullptr;
		}

		if( LogEnabled( eLogUsers ) )LogModule( eLogUsers, LOG_VERBOSE, "SoundFxMgr::%s play sound %s", __func__, DescribeSnd( sndDef ) );
		m_CurSndPlaying = m_SndList[ sndDef ];
		m_CurSndPlaying->startPlay( loopContinuous );

		return m_CurSndPlaying;
	}
	else
	{
		return m_SndList[ eSndDefNone ];
	}
}

//============================================================================
void SoundFxMgr::stopSnd( ESndDef sndDef )
{
    if(!m_Initialized)
    {
        return;
    }
    
	if( m_CurSndPlaying 
		&& ( sndDef == m_CurSndPlaying->getSndDef() ) )
	{
		m_CurSndPlaying->stopPlay();

		m_CurSndPlaying = nullptr;
	}
}

//============================================================================
void SoundFxMgr::slotSndFinished( VxSndInstance * sndInstance )
{
	if( m_CurSndPlaying == sndInstance )
	{

		m_CurSndPlaying = nullptr;
	}
}
