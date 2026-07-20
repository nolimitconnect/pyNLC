//============================================================================
// Copyright (C) 2019 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletCamSettings.h"

#include "AppCommon.h"	
#include "AppSettings.h"
#include "ActivityMsgBoxOk.h"

#include <GuiInterface/IFromGui.h>
#include <GuiInterface/ICamCapture.h>

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/IGlobalDb.h>

#include <QTimer>
#include <QSignalBlocker>

#include "ui_AppletCamSettings.h"

//============================================================================
AppletCamSettings::AppletCamSettings( AppCommon& app, QWidget* parent )
: AppletBase( OBJNAME_APPLET_CAM_SETTINGS, app, parent )
, ui(*(new Ui::AppletCamSettingsUi))
{
    setAppletType( eAppletCamSettings );
    setPluginType( ePluginTypeCamServer );
    ui.setupUi( getContentItemsFrame() );
    ui.m_CamVidWidget->setMediaModule( eMediaModuleCamSettings );
    setTitleBarText( DescribeApplet( m_EAppletType ) );

    ICamCapture& camCapture = ICamCapture::getICamCapture();
    if( !m_MyApp.getCamCaptureReady() )
    {
#if defined(TARGET_OS_ANDROID)
    // Start camera service once login/network is ready so camera devices are enumerated before Cam Settings is opened.
    ICamCapture::getICamCapture().startupCamCapture();
#endif // defined(TARGET_OS_ANDROID)

        if( ICamCapture::getICamCapture().isCamCaptureRequested() && !ICamCapture::getICamCapture().isCamCaptureRunning() )
        {
            ICamCapture::getICamCapture().setCamCaptureEnable( true );
        }

        m_MyApp.setCamCaptureReady( true );
    }



    if( !camCapture.isCamCaptureAvailable() )
    {
        // Android camera enumeration is async and can be empty until permission/service startup completes.
        LogMsg( LOG_WARN, "%s camera list initially empty", __func__ );
    }

    if( m_HisIdent )
    {
        setupCamFeed( &m_HisIdent->getNetIdent() );
    }
    else
    {
        setupCamFeed( m_MyApp.getAppGlobals().getMyNetIdent() );
    }

    startCamFeed();
    if( m_IsMyself )
    {
        m_DevicePollTimer = new QTimer( this );
        m_DevicePollTimer->setInterval( 500 );
        connect( m_DevicePollTimer, &QTimer::timeout, this, &AppletCamSettings::slotPollVideoDevices );
        updateInVideoDevices();
        connect( ui.m_InDeviceComboBox, &QComboBox::activated, this, &AppletCamSettings::inDeviceChanged );
        connect( ui.m_ApplyVideoInDeviceButton, &QPushButton::clicked, this, &AppletCamSettings::slotApplyInDeviceChange );
        m_DevicePollAttempts = 0;
        m_DevicePollTimer->start();
    }
    else
    {
        ui.m_InDeviceComboBox->setVisible( false );
        ui.m_ApplyVideoInDeviceButton->setVisible( false );
        ui.m_InDeviceLabel->setVisible( false );
    }

    m_MyApp.activityStateChange( this, true );
    m_MyApp.getUserMgr().wantGuiUserUpdateCallbacks( this, true );
}

//============================================================================
AppletCamSettings::~AppletCamSettings()
{
    if( m_DevicePollTimer )
    {
        m_DevicePollTimer->stop();
    }

    m_MyApp.getUserMgr().wantGuiUserUpdateCallbacks( this, false );
    m_MyApp.activityStateChange( this, false );
}

//============================================================================
void AppletCamSettings::setupCamFeed( VxNetIdent* feedNetIdent )
{
    if( !feedNetIdent )
    {
        LogMsg( LOG_ERROR, "setupCamFeed null feed ident" );
        vx_assert( false );
        return;
    }

    m_CamFeedIdent = feedNetIdent;
    m_CamFeedId = feedNetIdent->getMyOnlineId();
    m_IsMyself = m_CamFeedId == m_MyApp.getMyOnlineId();

    ui.m_CamVidWidget->showAllControls( true );
    ui.m_CamVidWidget->enableCamSourceControls( false );
    ui.m_CamVidWidget->setRecordFilePath( VxGetDownloadsDirectory().c_str() );
    ui.m_CamVidWidget->setRecordFriendName( m_CamFeedIdent->getOnlineName() );
    ui.m_CamVidWidget->setVideoFeedId( m_CamFeedId, eMediaModuleCamClient );

    QString bkgFile = m_MyApp.getCameraBackgroundFile();
    ui.m_CamVidWidget->setImageFromFile( bkgFile );
}

//============================================================================
void AppletCamSettings::startCamFeed( void )
{
    ICamCapture::getICamCapture().wantCamCapture( eMediaModuleCamClient, true );
}

//============================================================================
void AppletCamSettings::stopCamFeed( void )
{
    ICamCapture::getICamCapture().wantCamCapture( eMediaModuleCamClient, false );
}

//============================================================================
void AppletCamSettings::showEvent( QShowEvent* ev )
{
    // don't call AppletPeerBase::showEvent ... we don't want plugin offer/response for web cam server or client
    AppletBase::showEvent( ev );
    wantActivityCallbacks( true );
}

//============================================================================
void AppletCamSettings::hideEvent( QHideEvent* ev )
{
    // don't call AppletPeerBase::hideEvent ... we don't want plugin offer/response for web cam server or client
    wantActivityCallbacks( false );
    AppletBase::hideEvent( ev );
}

//============================================================================
void AppletCamSettings::closeEvent( QCloseEvent * ev )
{
    // don't call AppletPeerBase::hideEvent ... we don't want plugin offer/response for web cam server or client

    stopCamFeed();
    AppletBase::closeEvent( ev );
}

//============================================================================
void AppletCamSettings::resizeBitmapToFitScreen( QLabel * VideoScreen, QImage& oPicBitmap )
{
    QSize screenSize( VideoScreen->width(), VideoScreen->height() );
    oPicBitmap = oPicBitmap.scaled(screenSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );
}

//============================================================================
void AppletCamSettings::webCamSourceOffline()
{
    if( m_CamFeedIdent )
    {
        std::string statMsg = m_CamFeedIdent->getOnlineName();
        statMsg += "Cam Is Offline";
    }

    ui.m_CamVidWidget->showOfflineImage();
}

//============================================================================
void AppletCamSettings::slotToGuiRxedOfferReply( std::shared_ptr<GuiOfferSession> offerReply )
{
    if( !m_CamFeedIdent )
    {
        return;
    }

    if( ( ePluginTypeCamServer == offerReply->getPluginType() )
        && ( m_CamFeedIdent->getMyOnlineId() == offerReply->getUserIdent()->getMyOnlineId() ) )
    {
        if( eOfferResponseBusy == offerReply->getOfferResponse() )
        {
            playSound( eSndDefBusy );
        }

        if( eOfferResponseAccept != offerReply->getOfferResponse() )
        {
            webCamSourceOffline();
        }
    }
}; 

//============================================================================
void AppletCamSettings::slotToGuiSessionEnded( std::shared_ptr<GuiOfferSession> offer )
{
    if( !m_CamFeedIdent )
    {
        return;
    }

    if( ( ePluginTypeCamServer == offer->getPluginType() )
        && ( m_CamFeedIdent->getMyOnlineId() == offer->getUserIdent()->getMyOnlineId() ) )
    {
        webCamSourceOffline();
    }
}; 

//============================================================================
void AppletCamSettings::callbackOnlineStatusChange( GuiUser* guiUser, bool isOnline )
{
    if( !m_CamFeedIdent )
    {
        return;
    }

    if( m_CamFeedIdent->getMyOnlineId() == guiUser->getMyOnlineId() )
    {
        webCamSourceOffline();
    }
}

//============================================================================
void AppletCamSettings::inDeviceChanged( int index )
{
    if( index < 0 )
    {
        return;
    }

    QString camId = ui.m_InDeviceComboBox->currentText();

    if( camId.isEmpty() )
    {
        return;
    }

    if( !ICamCapture::getICamCapture().startCamCapture( camId.toUtf8().constData() ) )
    {
        ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Video In Device" ), camId + QObject::tr( " failed to initialize" ) );
        msgBox.exec();
    }
}

//============================================================================
void AppletCamSettings::updateInVideoDevices( void )
{
    QSignalBlocker blocker( ui.m_InDeviceComboBox );

    QString currentSelection = ui.m_InDeviceComboBox->currentText();
    ui.m_InDeviceComboBox->clear();

    std::vector<std::string> camList;
    ICamCapture::getICamCapture().getCamCaptureDevices( camList );

    bool hasDevices = !camList.empty();
    ui.m_InDeviceComboBox->setEnabled( hasDevices );
    ui.m_ApplyVideoInDeviceButton->setEnabled( hasDevices );
    if( !hasDevices )
    {
        return;
    }

    std::string defaultCamId = IGlobalDb::getIGlobalDb().getCamSourceId();

    int defaultIndex = -1;
    int devIndex = 0;
    for( auto& deviceDesc : camList )
    {
        if( defaultCamId == deviceDesc )
        {
            defaultIndex = devIndex;
        }
        else if( currentSelection == deviceDesc.c_str() )
        {
            defaultIndex = devIndex;
        }

        QString cameraId = QString::fromStdString( deviceDesc );
        ui.m_InDeviceComboBox->addItem( cameraId );
        devIndex++;
    }

    if( defaultIndex >= 0 )
    {
        ui.m_InDeviceComboBox->setCurrentIndex( defaultIndex );
    }
}

//============================================================================
void AppletCamSettings::slotPollVideoDevices( void )
{
    updateInVideoDevices();

    if( ui.m_InDeviceComboBox->count() > 0 )
    {
        m_DevicePollTimer->stop();
        return;
    }

    // Android camera enumeration is async after permission grant; stop polling after a reasonable timeout.
    m_DevicePollAttempts++;
    if( m_DevicePollAttempts >= 30 )
    {
        m_DevicePollTimer->stop();
    }
}

//============================================================================
void AppletCamSettings::slotApplyInDeviceChange( void )
{
    QString camId = ui.m_InDeviceComboBox->currentData().toString();
    if( camId.isEmpty() )
    {
        camId = ui.m_InDeviceComboBox->currentText();
    }

    if( !camId.isEmpty() )
    {
        if( ICamCapture::getICamCapture().startCamCapture( camId.toUtf8().constData() ) )
        {
            IGlobalDb::getIGlobalDb().setCamSourceId( camId.toUtf8().constData() );
            ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Video In Device" ), camId + QObject::tr( " device is saved as preferred Video In Device" ) );
            msgBox.exec();
        }
        else
        {
            ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Video In Device" ), camId + QObject::tr( " failed to initialize" ) );
            msgBox.exec();
        }
    }
    else
    {
       ActivityMsgBoxOk msgBox( m_MyApp, this, QObject::tr( "Video In Device" ), QObject::tr( "No Video In Device Is Available" ) );
       msgBox.exec();
    }
}
