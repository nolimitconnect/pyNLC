
#pragma once
//============================================================================
// Copyright (C) 2010 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "SoundDefs.h"

#include <QObject>
#include <QVector>

class QIODevice;
class QSound;
class MyQtSoundInput;
class MyQtSoundOutput;
class VxSndInstance;

class SoundFxMgr : public QObject
{
	Q_OBJECT
public:
    SoundFxMgr() = default;
	virtual ~SoundFxMgr() override;

	bool						sndFxMgrStartup( void );
	bool						sndFxMgrShutdown( void );

	VxSndInstance*			    playSnd( ESndDef sndDef, bool loopContinuous = false );
	void						stopSnd( ESndDef sndDef );

signals:
	void						signalSndFinished( VxSndInstance* sndInstance );

public slots:
	void						mutePhoneRing( bool bMute );
	void						muteNotifySound( bool bMute );
	void						slotStartPhoneRinging( void );
	void						slotStopPhoneRinging( void );
	void						slotPlayNotifySound( void );
	void						slotPlayShredderSound( void );

private slots:
	void						slotSndFinished( VxSndInstance * sndInstance );

protected:

	//=== vars ===//
	bool						m_MutePhoneRing{ false };
	bool						m_MuteNotifySnd{ false };
    bool                        m_Initialized{ false };

	QVector<VxSndInstance *>	m_SndList;
	VxSndInstance *			    m_CurSndPlaying{ nullptr };

};

