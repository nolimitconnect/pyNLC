#pragma once
//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletClientBase.h"

#include "AudioDefs.h"

#include "GuiAudioLevelCallback.h"
#include "GuiUserUpdateCallback.h"

#include "TestFileWavMgr.h"

#include <GuiInterface/IDefs.h>
#include <GuiInterface/IAudioDefs.h>

#include <libaudio-nlc/AudioDelayTestCallback.h>

#include <QString>
#include <QDialog>
#include <QIODevice>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AppletSoundSettingsUi;
}
QT_END_NAMESPACE

class GuiAudioMgr;
class AppSettings;

class AppletSoundSettings : public AppletClientBase, public GuiAudioLevelCallback, public AudioDelayTestCallback
{
	Q_OBJECT
public:
	AppletSoundSettings( AppCommon& app, QWidget* parent = nullptr );
	virtual ~AppletSoundSettings() override;

    GuiAudioMgr&                getAudioMgr( void );

    AppSettings&                getAppSettings( void );

    void                        infoMsg( const char* infoMsg, ... );
    void                        toGuiInfoMsg( char * logMsg );

    void                        statusMsg( const char* errMsg, ... );
    void						setStatusLabel( QString strMsg );

    void                        updateInAudioDevices( void );

    void                        updateOutAudioDevices( void );

signals:
    void                        signalAudioTestMsg( QString audioTestMsg );
    void                        signalInfoMsg( QString& infoStr );

    void                        signalAudioDelayTestStarted( void );
    void                        signalAudioDelayTestProgress( int delayMs, bool delayValueValid );
    void                        signalAudioDelayTestFinished( int delayAverageMs, bool delayValueValid );

protected slots:
    void                        slotApplyInDeviceChange( void );
    void                        slotApplyOutDeviceChange( void );

    void                        slotStartTestSoundDelay( void );
    void                        slotEchoDelaySaveButtonClicked( void );
 
    void                        slotGenerateToneCheckBox( int checkedState );

    void                        slotPlayTestFileButtonClicked( void );

    void                        slotInDeviceComboBoxChanged( int index );
    void                        slotOutDeviceComboBoxChanged( int index );  

    void                        slotApplyDefaultInDeviceButtonClicked( void );
    void                        slotApplyDefaultOutDeviceButtonClicked( void ); 

    void                        slotNoAecLoopbackCheckBoxClicked( void );
    void                        slotWithAecLoopbackCheckBoxClicked( void );
    
    void                        slotShowInWaveFormCheckBoxClicked( void );
    void                        slotShowOutWaveFormCheckBoxClicked( void );
    void                        slotShowSoundInCheckBoxClicked( void );
    void                        slotShowSoundOutCheckBoxClicked( void );
    void                        slotShowLogCheckBoxClicked( void );

    void                        slotVerboseLogEnable( bool verboseLogEnabled );

    void                        slotAgcEnable( int checkedState );
    void                        slotNoiseSuppressionEnable( int checkedState );

    void                        slotAudioDelayTestStarted( void );
    void                        slotAudioDelayTestProgress( int delayMs, bool delayValueValid );
    void                        slotAudioDelayTestFinished( int delayAverageMs, bool delayValueValid );

protected:
    void                        showEvent( QShowEvent* ev ) override;
    void                        hideEvent( QHideEvent* ev ) override;

    void						callbackGuiMicrophoneLevel( int micLevel ) override;

    void                        loadUiFromAppSettings( void );

    void                        onAudioDelayTestStarted( void ) override;
    void                        onAudioDelayTestProgress( int delayMs, bool delayValueValid ) override;
    void                        onAudioDelayTestFinished( int delayAverageMs, bool delayValueValid ) override;

    bool                        checkMicrophonePermission( void );

    //=== vars ===//
    Ui::AppletSoundSettingsUi&  ui;

    EAudioTestState             m_AudioTestState{ eAudioTestStateNone };
    std::vector<int>            m_EchoDelayResultList;
};
