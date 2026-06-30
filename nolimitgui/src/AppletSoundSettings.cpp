//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletSoundSettings.h"
#include "ActivityMsgBoxOk.h"

#include "AppCommon.h"
#include "AppGlobals.h"
#include "AppSettings.h"
#include "GuiParams.h"

#include "GuiHelpers.h"
#include "GuiAudioMgr.h"
#include "SoundFxMgr.h"
#include "VxResourceToRealFile.h"

#include <P2PEngine/P2PEngine.h>
#include <MediaProcessor/MediaProcessor.h>

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxGlobals.h>

#include <QByteArray>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QScopedPointer>
#include <QFileInfo>
#include <QMediaDevices>

#include "ui_AppletSoundSettings.h"

namespace
{
    const int MAX_INFO_MSG_SIZE = 2048;
}

//============================================================================
AppletSoundSettings::AppletSoundSettings( AppCommon& app, QWidget*	parent )
: AppletClientBase( OBJNAME_APPLET_SOUND_SETTINGS, app, parent )
, ui(*(new Ui::AppletSoundSettingsUi))
{
    setAppletType( eAppletSoundSettings );
    ui.setupUi( getContentItemsFrame() );
    setTitleBarText( DescribeApplet( m_EAppletType ) );
    ui.m_StatusMsgLabel->setVisible( true );

    ui.m_AudioInPeakProgressBar->setValue( 0 );

    connectBarWidgets();

    // before we connect signals we load the test files into the combo box so that signal handlers don't get called when we set the current index of the combo box
    const QString resourcePath = ":/AppRes/Resources/NlcTestAudio.wav";
    VxResourceToRealFile realFile( resourcePath );
    if( !realFile.getRealFilePathAndName().empty() )
    {
        const QString testFile = QString::fromStdString( realFile.getRealFilePathAndName() );
        ui.m_TestFileComboBox->addItem( testFile );
    }

    int echoDelayMs = getAppSettings().getEchoDelayParam();
    ui.m_EchoDelayLineEdit->setText( QString::number( echoDelayMs ) );
    ui.m_TestDelayResultLineEdit->setText( QString::number( 0 ) );

    connect( ui.m_GenerateToneCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotGenerateToneCheckBox(int)) );

    connect( ui.m_TestSoundDelayButton, SIGNAL(clicked()), this, SLOT(slotStartTestSoundDelay()) );
    connect( ui.m_EchoDelaySaveButton, SIGNAL(clicked()), this, SLOT(slotEchoDelaySaveButtonClicked()) );

    updateInAudioDevices();

    connect( ui.m_InDeviceComboBox, SIGNAL(activated(int)), this, SLOT(slotInDeviceComboBoxChanged(int)) );
    connect( ui.m_ApplyDefaultInDeviceButton, SIGNAL(clicked()), this, SLOT(slotApplyDefaultInDeviceButtonClicked()) );

    updateOutAudioDevices();

    connect( ui.m_OutDeviceComboBox, SIGNAL(activated(int)), this, SLOT(slotOutDeviceComboBoxChanged(int)) );
    connect( ui.m_ApplyDefaultOutDeviceButton, SIGNAL(clicked()), this, SLOT(slotApplyDefaultOutDeviceButtonClicked()) );

    connect( ui.m_PlayTestFileButton, SIGNAL(clicked()), this, SLOT(slotPlayTestFileButtonClicked()) );

    connect( ui.m_NoAecLoopbackCheckBox, SIGNAL(clicked()), this, SLOT(slotNoAecLoopbackCheckBoxClicked()) );
    connect( ui.m_WithAecLoopbackCheckBox, SIGNAL(clicked()), this, SLOT(slotWithAecLoopbackCheckBoxClicked()) );

    connect( ui.m_ShowInWaveFormCheckBox, SIGNAL(clicked()), this, SLOT(slotShowInWaveFormCheckBoxClicked()) );
    connect( ui.m_ShowOutWaveFormCheckBox, SIGNAL(clicked()), this, SLOT(slotShowOutWaveFormCheckBoxClicked()) );
    connect( ui.m_ShowSoundInCheckBox, SIGNAL(clicked()), this, SLOT(slotShowSoundInCheckBoxClicked()) );
    connect( ui.m_ShowSoundOutCheckBox, SIGNAL(clicked()), this, SLOT(slotShowSoundOutCheckBoxClicked()) );
    connect( ui.m_ShowLogCheckBox, SIGNAL(clicked()), this, SLOT(slotShowLogCheckBoxClicked()) );

    connect( ui.m_LogWidget, SIGNAL(signalVerboseLogEnable(bool)), this, SLOT(slotVerboseLogEnable(bool)) );

    connect( ui.m_AgcCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotAgcEnable(int)) );
    connect( ui.m_NoiseSuppressionCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotNoiseSuppressionEnable(int)) );

    connect( this, SIGNAL(signalAudioDelayTestStarted()), this, SLOT(slotAudioDelayTestStarted()), Qt::QueuedConnection );
    connect( this, SIGNAL(signalAudioDelayTestProgress(int,bool)), this, SLOT(slotAudioDelayTestProgress(int,bool)), Qt::QueuedConnection );
    connect( this, SIGNAL(signalAudioDelayTestFinished(int,bool)), this, SLOT(slotAudioDelayTestFinished(int,bool)), Qt::QueuedConnection );

    m_MyApp.activityStateChange( this, true );

    if( !m_MyApp.getAudioMgr().getIsSpeakerAvailable() )
    {
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Speaker Device Unavailable" ), QObject::tr( "No speaker device is available to enable" ) );
        msgBox.exec();
    }
    else if( !m_MyApp.getAudioMgr().getIsMicrophoneAvailable() )
    {
        m_MyApp.getSoundFxMgr().playSnd( eSndDefBusy );
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Microphone Device Unavailable" ), QObject::tr( "No microphone device is available to enable" ) );
        msgBox.exec();
    }

    loadUiFromAppSettings();

    ui.m_LogWidget->initLogCallback();

    m_MyApp.getAudioMgr().toGuiWantSpeakerOutput( eMediaModuleSoundSettings, true );
    m_MyApp.getAudioMgr().toGuiWantMicrophoneRecording( eMediaModuleSoundSettings, true );
    m_MyApp.getAudioMgr().wantMicrophoneLevelCallbacks( this, true );

    m_MyApp.getAudioMgr().wantAudioDelayTestCallbacks( this, true );

    if( !checkMicrophonePermission() )
    {
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Microphone Permission Denied" ), QObject::tr( "The application does not have permission to access the microphone" ) );
        msgBox.exec();
        onBackButtonClicked();
    }
}

//============================================================================
AppletSoundSettings::~AppletSoundSettings()
{
    // Do not leave in testing loopback state
    getAudioMgr().setNoAecLoopbackEnable( false ); 
    getAudioMgr().setWithAecLoopbackEnable( false ); 

    m_MyApp.getAudioMgr().wantAudioDelayTestCallbacks( this, false );

    m_MyApp.getAudioMgr().wantMicrophoneLevelCallbacks( this, false );
    m_MyApp.getAudioMgr().toGuiWantMicrophoneRecording( eMediaModuleSoundSettings, false );
    m_MyApp.getAudioMgr().toGuiWantSpeakerOutput( eMediaModuleSoundSettings, false );

    m_MyApp.activityStateChange( this, false );
}

//============================================================================
GuiAudioMgr& AppletSoundSettings::getAudioMgr( void )
{
    return m_MyApp.getAudioMgr();
}

//============================================================================
AppSettings& AppletSoundSettings::getAppSettings( void )
{
    return m_MyApp.getAppSettings();
}

//============================================================================
void AppletSoundSettings::setStatusLabel( QString strMsg )
{
    ui.m_StatusMsgLabel->setText( strMsg );
}

//============================================================================
void AppletSoundSettings::showEvent( QShowEvent* ev )
{
    ActivityBase::showEvent( ev );
    wantActivityCallbacks( true );
}

//============================================================================
void AppletSoundSettings::hideEvent( QHideEvent* ev )
{
    wantActivityCallbacks( false );
    ActivityBase::hideEvent( ev );
}

//============================================================================
void AppletSoundSettings::toGuiInfoMsg( char * infoMsg )
{
    QString infoStr( infoMsg );

    static QRegularExpression removeExpress( "[\\n\\r]" );
    infoStr.remove( removeExpress );

    emit signalInfoMsg( infoStr );
}

//============================================================================
void AppletSoundSettings::infoMsg( const char* errMsg, ... )
{
    char as8Buf[ MAX_INFO_MSG_SIZE ];
    va_list argList;
    va_start( argList, errMsg );
    vsnprintf( as8Buf, sizeof( as8Buf ), errMsg, argList );
    as8Buf[ sizeof( as8Buf ) - 1 ] = 0;
    va_end( argList );

    toGuiInfoMsg( as8Buf );
}

//============================================================================
void AppletSoundSettings::statusMsg( const char* errMsg, ... )
{
    char as8Buf[ MAX_INFO_MSG_SIZE ];
    va_list argList;
    va_start( argList, errMsg );
    vsnprintf( as8Buf, sizeof( as8Buf ), errMsg, argList );
    as8Buf[ sizeof( as8Buf ) - 1 ] = 0;
    va_end( argList );

    setStatusLabel( as8Buf );
}

//============================================================================
void AppletSoundSettings::updateInAudioDevices( void )
{
    ui.m_InDeviceComboBox->clear();

    std::vector<std::string>& inDeviceList = m_MyApp.getAudioMgr().getAudioInDevices();

    int devIndex = 0;
    for( auto& deviceDesc : inDeviceList )
    {
        ui.m_InDeviceComboBox->addItem( deviceDesc.c_str() );
        devIndex++;
    }
}

//============================================================================
void AppletSoundSettings::updateOutAudioDevices( void )
{
    ui.m_OutDeviceComboBox->clear();

    std::vector<std::string>& outDeviceList = m_MyApp.getAudioMgr().getAudioOutDevices();

    int devIndex = 0;
    for( auto& deviceDesc : outDeviceList )
    {
        ui.m_OutDeviceComboBox->addItem( deviceDesc.c_str() );
        devIndex++;
    }
}

//============================================================================
void AppletSoundSettings::slotApplyInDeviceChange( void )
{
    QString sndInDevDescription = ui.m_InDeviceComboBox->currentText();
    if( !sndInDevDescription.isEmpty() )
    {
        if( m_MyApp.getAudioMgr().setAudioInDevice( sndInDevDescription.toStdString() ) )
        {            
            ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Sound In Device" ), sndInDevDescription + QObject::tr( " device is saved as preferred Sound In Device" ) );
            msgBox.exec();
        }
        else
        {
            ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Sound In Device" ), sndInDevDescription + QObject::tr( " failed to initialize" ) );
            msgBox.exec();
        }
    }
    else
    {
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Sound In Device" ), QObject::tr( "No Sound In Device Is Available" ) );
        msgBox.exec();
    }
}

//============================================================================
void AppletSoundSettings::slotApplyOutDeviceChange( void )
{
    QString sndOutDevDescription = ui.m_OutDeviceComboBox->currentText();
    if( !sndOutDevDescription.isEmpty() )
    {
        if( m_MyApp.getAudioMgr().setAudioOutDevice( sndOutDevDescription.toStdString() ) )
        {
            m_MyApp.getAppSettings().setSoundOutDevice( sndOutDevDescription.toStdString() );
            ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Sound Out Device" ), sndOutDevDescription + QObject::tr( " device is saved as preferred Sound Out Device" ) );
            msgBox.exec();
        }
        else
        {
            ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Sound Out Device" ), sndOutDevDescription + QObject::tr( " failed to initialize" ) );
            msgBox.exec();
        }
    }
    else
    {
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Sound Out Device" ), QObject::tr( "No Sound Out Device Is Available" ) );
        msgBox.exec();
    }
}

//============================================================================
void AppletSoundSettings::slotStartTestSoundDelay( void )
{
    ui.m_StatusMsgLabel->setText( "" );
    if( eAudioTestStateNone == m_AudioTestState )
    {
        m_EchoDelayResultList.clear();
        m_MyApp.getAudioMgr().runEchoDelayTest();
    }
    else
    {
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Echo delay test is running" ), QObject::tr( "Echo delay test can not be run until the previous test finishes" ) );
        msgBox.exec();
    }
}

//============================================================================
void AppletSoundSettings::slotEchoDelaySaveButtonClicked( void )
{
    int delayMs = ui.m_EchoDelayLineEdit->text().toInt();
    if( delayMs < 40 || delayMs > 500 )
    {
        QMessageBox::information( this, QObject::tr( "Echo Delay Value Invalid" ), QObject::tr( "Echo Delay value must be between 40 and 500 milliseconds" ) );
    }
    else
    {
        getAppSettings().setEchoDelayParam( delayMs );
        getAudioMgr().setEchoDelayParam( delayMs );
        QMessageBox::information( this, QObject::tr( "Echo Delay Value Save" ), QObject::tr( "Echo Delay value has been saved for use by Echo Cancelation" ) );
    }
}

//============================================================================
void AppletSoundSettings::slotGenerateToneCheckBox( int checkedState )
{
    bool genTone = ui.m_GenerateToneCheckBox->isChecked();
    getAudioMgr().setEnableSpeakerTestTone( genTone );
    ui.m_AudioInPeakProgressBar->setValue( 0 );
}

//============================================================================
void AppletSoundSettings::callbackGuiMicrophoneLevel( int micLevel )
{
    ui.m_AudioInPeakProgressBar->setValue( micLevel );
}

//============================================================================
void AppletSoundSettings::slotPlayTestFileButtonClicked( void )
{
    QString testFile = ui.m_TestFileComboBox->currentText();
    if( !testFile.isEmpty() )
    {
        getAudioMgr().playTestFile( testFile.toStdString() );
    }
    else
    {
        QMessageBox::information( this, QObject::tr( "Play Test File" ), QObject::tr( "No test file is selected or test file is invalid" ) );
    }
}

//============================================================================
void AppletSoundSettings::slotInDeviceComboBoxChanged( int index )
{
    if( index < 0 || index >= ui.m_InDeviceComboBox->count() )
    {
        return;
    }

    slotApplyInDeviceChange();
}

//============================================================================
void AppletSoundSettings::slotOutDeviceComboBoxChanged( int index )
{
    if( index < 0 || index >= ui.m_OutDeviceComboBox->count() )
    {
        return;
    }

    slotApplyOutDeviceChange();
}

//============================================================================
void AppletSoundSettings::slotApplyDefaultInDeviceButtonClicked( void )
{
    slotApplyInDeviceChange();
}

//============================================================================
void AppletSoundSettings::slotApplyDefaultOutDeviceButtonClicked( void )
{
    slotApplyOutDeviceChange();
}

//============================================================================
void AppletSoundSettings::slotNoAecLoopbackCheckBoxClicked( void )
{
    bool loopbackEnabled = ui.m_NoAecLoopbackCheckBox->isChecked();
    getAudioMgr().setNoAecLoopbackEnable( loopbackEnabled );
    getAppSettings().setNoAecLoopback( loopbackEnabled );
}

//============================================================================
void AppletSoundSettings::slotWithAecLoopbackCheckBoxClicked( void )
{
    bool loopbackEnabled = ui.m_WithAecLoopbackCheckBox->isChecked();
    getAudioMgr().setWithAecLoopbackEnable( loopbackEnabled );
    getAppSettings().setWithAecLoopback( loopbackEnabled );
}

//============================================================================
void AppletSoundSettings::slotShowInWaveFormCheckBoxClicked( void )
{   
    bool showWaveForm = ui.m_ShowInWaveFormCheckBox->isChecked();
    ui.m_AudioInWaveFormFrame->setVisible( showWaveForm );
    getAppSettings().setShowInWaveForm( showWaveForm );
}

//============================================================================
void AppletSoundSettings::slotShowOutWaveFormCheckBoxClicked( void )
{   
    bool showWaveForm = ui.m_ShowOutWaveFormCheckBox->isChecked();
    ui.m_AudioOutWaveFormFrame->setVisible( showWaveForm );
    getAppSettings().setShowOutWaveForm( showWaveForm );
}

//============================================================================
void AppletSoundSettings::slotShowSoundInCheckBoxClicked( void )
{
    bool showSoundIn = ui.m_ShowSoundInCheckBox->isChecked();
    ui.m_InSettingsGroupBox->setVisible( showSoundIn );
    getAppSettings().setShowSoundInSettings( showSoundIn );
    getAudioMgr().wantMicrophoneLevelCallbacks( this, showSoundIn );
}

//============================================================================
void AppletSoundSettings::slotShowSoundOutCheckBoxClicked( void )
{
    bool showSoundOut = ui.m_ShowSoundOutCheckBox->isChecked();
    ui.m_OutSettingsGroupBox->setVisible( showSoundOut );
    getAppSettings().setShowSoundOutSettings( showSoundOut );
}

//============================================================================
void AppletSoundSettings::slotShowLogCheckBoxClicked( void )
{
    bool showLog = ui.m_ShowLogCheckBox->isChecked();
    ui.m_LogWidget->setVisible( showLog );
    getAppSettings().setShowSoundLog( showLog );
}

//============================================================================
void AppletSoundSettings::loadUiFromAppSettings( void )
{
    bool showInWaveForm = getAppSettings().getShowInWaveForm();
    ui.m_ShowInWaveFormCheckBox->setChecked( showInWaveForm );
    ui.m_AudioInWaveFormFrame->setVisible( showInWaveForm );
    bool showOutWaveForm = getAppSettings().getShowOutWaveForm();
    ui.m_ShowOutWaveFormCheckBox->setChecked( showOutWaveForm );
    ui.m_AudioOutWaveFormFrame->setVisible( showOutWaveForm );

    bool enableAgc = getAppSettings().getAgcEnabled();
    ui.m_AgcCheckBox->setChecked( enableAgc );

    bool showSoundIn = getAppSettings().getShowSoundInSettings();
    ui.m_ShowSoundInCheckBox->setChecked( showSoundIn );
    ui.m_InSettingsGroupBox->setVisible( showSoundIn );
    if( showSoundIn )
    {
        getAudioMgr().wantMicrophoneLevelCallbacks( this, showSoundIn );
    }

    bool showSoundOut = getAppSettings().getShowSoundOutSettings();
    ui.m_ShowSoundOutCheckBox->setChecked( showSoundOut );
    ui.m_OutSettingsGroupBox->setVisible( showSoundOut );

    bool showLog = getAppSettings().getShowSoundLog();
    ui.m_ShowLogCheckBox->setChecked( showLog );
    ui.m_LogWidget->setVisible( showLog );


    getAudioMgr().setNoAecLoopbackEnable( getAppSettings().getNoAecLoopback() ); 
    getAudioMgr().setWithAecLoopbackEnable( getAppSettings().getWithAecLoopback() ); 

    ui.m_NoAecLoopbackCheckBox->setChecked( getAppSettings().getNoAecLoopback() );
    ui.m_WithAecLoopbackCheckBox->setChecked( getAppSettings().getWithAecLoopback() );


    if( ui.m_InDeviceComboBox->count() > 0 )
    {
        std::string savedInDevice = getAppSettings().getSoundInDevice();
        int savedInDeviceIndex = ui.m_InDeviceComboBox->findText( QString::fromStdString( savedInDevice ) );
        if( savedInDeviceIndex < 0 || savedInDeviceIndex >= ui.m_InDeviceComboBox->count() )
        {
            savedInDeviceIndex = 0;
        }

        ui.m_InDeviceComboBox->setCurrentIndex( savedInDeviceIndex );
    }

    if( ui.m_OutDeviceComboBox->count() > 0 )
    {
        std::string savedOutDevice = getAppSettings().getSoundOutDevice();
        int savedOutDeviceIndex = ui.m_OutDeviceComboBox->findText( QString::fromStdString( savedOutDevice ) );
        if( savedOutDeviceIndex < 0 || savedOutDeviceIndex >= ui.m_OutDeviceComboBox->count() )
        {
            savedOutDeviceIndex = 0;
        }

        ui.m_OutDeviceComboBox->setCurrentIndex( savedOutDeviceIndex );
    }

    ui.m_AgcCheckBox->setChecked( getAppSettings().getAgcEnabled() );
    ui.m_NoiseSuppressionCheckBox->setChecked( getAppSettings().getNoiseSuppressionEnabled() );
}

//============================================================================
void AppletSoundSettings::slotVerboseLogEnable( bool verboseLogEnabled )
{
    // getAppSettings().setVerboseLog( verboseLogEnabled );
    // VxSetLogLevelFlags( (uint32_t)(( verboseLogEnabled ? LOG_VERBOSE : 0 ) | 0x000001fe ) );
}

//============================================================================
void AppletSoundSettings::slotAgcEnable( int checkedState )
{
    bool agcEnabled = ( checkedState != 0 );
    getAudioMgr().setAgcEnabled( agcEnabled );
    getAppSettings().setAgcEnabled( agcEnabled );
}

//============================================================================
void AppletSoundSettings::slotNoiseSuppressionEnable( int checkedState )
{
    bool noiseSuppressionEnabled = ( checkedState != 0 );
    getAudioMgr().setNoiseSuppressionEnabled( noiseSuppressionEnabled );
    getAppSettings().setNoiseSuppressionEnabled( noiseSuppressionEnabled );
}



//============================================================================
void AppletSoundSettings::onAudioDelayTestStarted( void )
{
    emit signalAudioDelayTestStarted();
}

//============================================================================
void AppletSoundSettings::onAudioDelayTestProgress( int delayMs, bool delayValueValid )
{
    emit signalAudioDelayTestProgress( delayMs, delayValueValid );
}

//============================================================================
void AppletSoundSettings::onAudioDelayTestFinished( int delayAverageMs, bool delayValueValid )
{
    emit signalAudioDelayTestFinished( delayAverageMs, delayValueValid );
}

//============================================================================
void AppletSoundSettings::slotAudioDelayTestStarted( void )
{
}

//============================================================================
void AppletSoundSettings::slotAudioDelayTestProgress( int delayMs, bool delayValueValid )
{
    if(!delayValueValid)
    {
        QString resultMsg;
        if( delayMs == 0 )
        {
            resultMsg = QObject::tr("Sound Delay Not Detected. Check speaker volume and that microphone is on ");
        }
        else if( delayMs < 50 )
        {
            resultMsg = QObject::tr("Sound Delay too short.. probably noise ");
        }
        else
        {
            resultMsg = QObject::tr("Sound Delay too long.. probably mic level low ");
        }

        resultMsg += QObject::tr("Delay: %1").arg(delayMs);
        ui.m_StatusMsgLabel->setText( resultMsg );
    } 
    else
    {
        QString resultMsg = QObject::tr("Sound Delay Test Value Valid ");
        resultMsg += QObject::tr("Delay: %1").arg(delayMs);
        ui.m_StatusMsgLabel->setText( resultMsg );
    }
}

//============================================================================
void AppletSoundSettings::slotAudioDelayTestFinished( int delayAverageMs, bool delayValueValid )
{
    if(delayValueValid)
    {
        QString resultMsg = QObject::tr("You can enter the measured delay. Average Delay: %1 ms").arg(delayAverageMs);
        QMessageBox::information(nullptr, QObject::tr("Audio Delay Test Success"), resultMsg);
    }
    else
    {
        QString resultMsg = QObject::tr("Try turning up the volume or placing microphone closer to speaker");
        QMessageBox::warning(nullptr, QObject::tr("Audio Delay Test Failed"), resultMsg);
    }

}

//============================================================================
bool AppletSoundSettings::checkMicrophonePermission( void )
{
#if !defined(TARGET_OS_ANDROID)
    return true;
#endif
    return GuiParams::requestPermission( "android.permission.RECORD_AUDIO" );
}