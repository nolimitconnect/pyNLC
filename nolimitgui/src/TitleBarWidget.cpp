//============================================================================
// Copyright (C) 2015 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "TitleBarWidget.h"

#include "AppCommon.h"
#include "AppletMgr.h"
#include "AppSettings.h"
#include "AppletPopupMenu.h"
#include "GuiAudioMgr.h"
#include "GuiOfferMgr.h"
#include "GuiHelpers.h"
#include "GuiParams.h"
#include "GuiPlayerMgr.h"
#include "GuiPluginMgr.h"

#include "MyIcons.h"
#include "SoundFxMgr.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxTime.h>

#include <GuiInterface/ICamCapture.h>

#include <QFrame>

#include "ui_TitleBarWidget.h"

VxPushButton * TitleBarWidget::getBackButton( void ) { return ui.m_BackDlgButton; }

//============================================================================
TitleBarWidget::TitleBarWidget( QWidget* parent )
: QWidget( parent )
, ui(*(new Ui::TitleBarWidgetUi))
, m_MyApp( GetAppInstance() )
, m_OfferMgr( m_MyApp.getOfferMgr() )
, m_CamTimer(new QTimer(this))
{
	ui.setupUi( this );
    setFixedHeight( GuiParams::getButtonSize( eButtonSizeSmall ).height() + 4 );

    ui.m_TestButton->setVisible( false );

    ui.m_WantMicCountLabel->setVisible( false );
    ui.m_WantSpeakerCountLabel->setVisible( false );

    ui.m_NoLimitAppButton->setFixedSize( eButtonSizeSmall );
    ui.m_PowerOffButton->setFixedSize( eButtonSizeSmall );

    ui.m_OfferListButton->setFixedSize( eButtonSizeSmall );
    ui.m_OfferCountLabel->setVisible( false );
    ui.m_HostJoinRequestListButton->setFixedSize( eButtonSizeSmall );
    ui.m_HostJoinRequestCountLabel->setVisible( false );
    ui.m_MuteMicButton->setFixedSize( eButtonSizeSmall );
    ui.m_MuteSpeakerButton->setFixedSize( eButtonSizeSmall );
    ui.m_CameraStartStopButton->setFixedSize( eButtonSizeSmall );
    ui.m_MenuTopButton->setFixedSize( eButtonSizeSmall );
    ui.m_MenuButton->setFixedSize( eButtonSizeSmall );
    ui.m_BackDlgButton->setFixedSize( eButtonSizeSmall );

    ui.m_FriendRequestListButton->setFixedSize( eButtonSizeSmall );
    ui.m_FriendRequestListButton->setIcon( eMyIconFriendRequestList );

    ui.m_NetAvailStatusWidget->setFixedSizeAbsolute( GuiParams::getButtonSize( eButtonSizeSmall ) );

    ui.m_MicVolPeakBar->setFixedSize( GuiParams::getDefaultFontHeight() / 2, GuiParams::getButtonSize( eButtonSizeSmall ).height() );
    ui.m_MicVolPeakBar->setOrientation( Qt::Vertical );
    ui.m_MicVolPeakBar->setRange( 0, 100 );

    float aspectRatio = 320.0f / 240.0f;
    ui.m_CamPreviewScreen->setFixedSize( (int)(GuiParams::getButtonSize( eButtonSizeSmall ).height() * aspectRatio + 1), GuiParams::getButtonSize( eButtonSizeSmall ).height() );

    ui.m_HostJoinRequestListButton->setIcon( eMyIconPersonsOfferList );
    ui.m_OfferListButton->setIcon( eMyIconOfferList );
    ui.m_MenuButton->setIcon( eMyIconMenu );

	ui.m_NoLimitAppButton->setUseTheme( false );
	ui.m_NoLimitAppButton->setProperty( "NoLimitConnectIcon", true );
	ui.m_NoLimitAppButton->setAppIcon( eMyIconApp, parent );

    m_MutedMic = m_MyApp.getAppSettings().getIsMicrophoneMuted();
    m_MutedSpeaker = m_MyApp.getAppSettings().getIsSpeakerMuted();

	setPowerButtonIcon();
	setMicrophoneIcon( m_MutedMic ? eMyIconMicrophoneOff : eMyIconMicrophoneOn );
    setSpeakerIcon( m_MutedSpeaker ? eMyIconSpeakerOff : eMyIconSpeakerOn );
    setCameraIcon();
    setTopMenuButtonIcon();
    setBackButtonIcon();

    setCameraButtonVisibility( false );
    setMenuTopButtonVisibility( false );

    // everyone except home page has back button but not power off button
    setPowerButtonVisibility( false );
    setBackButtonVisibility( true );

    connect( ui.m_NoLimitAppButton,         SIGNAL(clicked()), this, SLOT(slotApplicationIconClicked()) );

    connect( ui.m_PowerOffButton,           SIGNAL(clicked()), this, SLOT(slotPowerButtonClicked()) );
    connect( ui.m_HostJoinRequestListButton,   SIGNAL(clicked()), this, SLOT(slotHostJoinRequestButtonClicked()) );
    connect( ui.m_OfferListButton,          SIGNAL(clicked()), this, SLOT(slotOfferListButtonClicked()) );
    connect( ui.m_BackDlgButton,            SIGNAL(clicked()), this, SLOT(slotBackButtonClicked()) );
    connect( ui.m_MuteMicButton,            SIGNAL(clicked()), this, SLOT(slotMuteMicButtonClicked()) );
    connect( ui.m_MuteSpeakerButton,        SIGNAL(clicked()), this, SLOT(slotMuteSpeakerButtonClicked()) );
    connect( ui.m_CameraStartStopButton,    SIGNAL(clicked()), this, SLOT(slotCameraSnapshotButtonClicked()) );
    connect( ui.m_CamPreviewScreen,         SIGNAL(clicked()), this, SLOT(slotCamPreviewClicked()) );

    connect( &m_MyApp,                      SIGNAL(signalNetAvailStatus(ENetAvailStatus)), this, SLOT(slotToGuiNetAvailStatus(ENetAvailStatus)) );

    connect( m_CamTimer,                    SIGNAL(timeout()),                          this, SLOT(slotCamTimeout()) );

    connect( ui.m_NetAvailStatusWidget,     SIGNAL(clicked()),                          this, SLOT(slotSignalHelpClick()) );

    connect( ui.m_MenuButton,               SIGNAL(clicked()),                          this, SLOT(slotTitleBarUserMenuButtonClicked()) );

    connect( ui.m_FriendRequestListButton,  SIGNAL(clicked()),                          this, SLOT(slotFriendRequestListButtonClicked()) );

    connect( ui.m_TestButton,               SIGNAL(clicked()),                          this, SLOT(slotTestButtonClicked()) );

    ui.m_MicVolPeakBar->setFixedWidth( GuiParams::getDefaultFontHeight() / 2 );
    ui.m_MicVolPeakBar->setRange( 0, 50 ); // make twice as sensitive to make it more obvious

    updateTitleBar();
    connect( &m_MyApp,                      SIGNAL(signalSystemReady(bool)),            this, SLOT(slotSystemReady(bool)) );

    callbackActiveOfferCount( m_MyApp.getOfferMgr().getActiveOfferCount(),  m_MyApp.getOfferMgr().getHistoryOfferCount() );
    callbackJoinRequestCount( m_MyApp.getHostJoinMgr().getJoinRequestCount() );
}

//============================================================================
TitleBarWidget::~TitleBarWidget()
{
    wantCallbacks( false );
}

//============================================================================
void TitleBarWidget::wantCallbacks( bool enable )
{
    if( enable != m_CallbacksRequested )
    {
        m_CallbacksRequested = enable;
        m_MyApp.wantToGuiHardwareCtrlCallbacks( this, enable );
        m_MyApp.wantToGuiActivityCallbacks( this, enable );
        m_MyApp.getOfferMgr().wantGuiOfferCallbacks( this, enable );
        m_MyApp.getHostJoinMgr().wantHostJoinCallbacks( this, enable );
        m_MyApp.getPluginMgr().wantPluginMgrCallbacks( this, enable ); 
        m_MyApp.getFriendRequestMgr().wantFriendRequestListCallbacks( this, enable ); 
    }

    updateCamCallbackRequests();
    updateAudioLevelCallbackRequests();
}

//============================================================================
void TitleBarWidget::updateTitleBar( void )
{
    GuiAudioMgr& audioMgr = m_MyApp.getAudioMgr();
    bool isMicEnabled = audioMgr.getIsMicrophoneRunning();
    callbackToGuiWantMicrophoneRecording( isMicEnabled );

    m_MutedMic = audioMgr.getIsMicrophoneMuted();
    callbackToGuiMicrophoneMuted( m_MutedMic ); 

    m_MutedSpeaker = audioMgr.getIsSpeakerMuted();
    callbackToGuiSpeakerMuted( m_MutedSpeaker );

    bool isCamRequested = ICamCapture::getICamCapture().isCamCaptureRequested();
    callbackToGuiWantVideoCapture( isCamRequested );

    checkTitleBarIconsFit();

    ui.m_CamPreviewScreen->setImageFromFile( m_MyApp.getCameraBackgroundFile() );

    updateWebServerClientCount();

    bool isCamEnabled = ICamCapture::getICamCapture().getCamCaptureEnable();
    callbackToGuiCameraEnable( isCamEnabled && isVisible() );

    update();
}

//============================================================================
void TitleBarWidget::slotSystemReady( bool isReady )
{
    if( isReady )
    {
        updateTitleBar();        
        updateCamCallbackRequests();
        updateAudioLevelCallbackRequests();
    }
}

//============================================================================
void TitleBarWidget::slotSignalHelpClick( void )
{
    m_MyApp.getAppletMgr().launchApplet( eAppletHelpNetSignalBars, getTitleBarParentPage() );
}

//============================================================================
void TitleBarWidget::showEvent( QShowEvent* showEvent )
{
    QWidget::showEvent( showEvent );
    if( ( false == VxIsAppShuttingDown() )
        && ( false == m_CallbacksRequested ) )
    {
        wantCallbacks( true );

        updateTitleBar();
    }
}

//============================================================================
void TitleBarWidget::hideEvent( QHideEvent* ev )
{
    QWidget::hideEvent( ev );
    if( m_CallbacksRequested && ( false == VxIsAppShuttingDown() ) )
    {
        wantCallbacks( false );
    }
}

//============================================================================
void TitleBarWidget::resizeEvent( QResizeEvent* ev )
{
    checkTitleBarIconsFit();
    QWidget::resizeEvent( ev );
}

//============================================================================
void TitleBarWidget::slotCamTimeout()
{
    if( GetApplicationAliveMs() - m_LastCamFrameTimeMs > 3000 )
    {
        m_CamTimer->stop();
        ui.m_CamPreviewScreen->setImageFromFile( m_MyApp.getCameraBackgroundFile() );
    }
}

//============================================================================
MyIcons&  TitleBarWidget::getMyIcons( void )
{
	return m_MyApp.getMyIcons();
}

//============================================================================
VxPushButton * TitleBarWidget::getAppIconPushButton( void )
{
	return ui.m_NoLimitAppButton;
}

//============================================================================
void TitleBarWidget::setTitleBarText( QString titleText )
{
    m_TitleText = titleText.toUtf8().constData();
	ui.StyledDlgTitleLabel->setText( titleText );
}

//============================================================================
QString TitleBarWidget::getTitleBarText( void )
{
	return ui.StyledDlgTitleLabel->text();
}

//============================================================================
void TitleBarWidget::callbackToGuiPluginStatus( EPluginType pluginType, int statusType, int statusValue )
{
    if( ePluginTypeCamServer == pluginType )
    {
        updateWebServerClientCount();
    }
}

//============================================================================
void TitleBarWidget::updateWebServerClientCount( void )
{
    if( m_MyApp.getPluginMgr().getIsCamServerEnabled() )
    {
        ui.m_CamClientCountLabel->setVisible( true );
        int webCamClientCount = m_MyApp.getPluginMgr().getCamServerClientCount();
        if( webCamClientCount < 0 )
        {
            ui.m_CamClientCountLabel->setText( "" );
        }
        else
        {
            ui.m_CamClientCountLabel->setText( QString( "%1" ).arg( webCamClientCount ) );
        }
    }
    else
    {
        ui.m_CamClientCountLabel->setVisible( false );
    }
}

//============================================================================
void TitleBarWidget::slotPowerButtonClicked( void )
{
	emit signalPowerButtonClicked();
}

//============================================================================
void TitleBarWidget::slotMuteMicButtonClicked( void )
{
	m_MutedMic = !m_MutedMic;
    m_MyApp.fromGuiMuteMicrophone( m_MutedMic );
    m_MyApp.getAppSettings().setIsMicrophoneMuted( m_MutedMic );
}

//============================================================================
void TitleBarWidget::slotMuteSpeakerButtonClicked( void )
{
	m_MutedSpeaker = !m_MutedSpeaker;
    m_MyApp.fromGuiMuteSpeaker( m_MutedSpeaker );
    m_MyApp.getAppSettings().setIsSpeakerMuted( m_MutedSpeaker );
}

//============================================================================
void TitleBarWidget::slotCameraSnapshotButtonClicked( void )
{
	emit signalCameraSnapshotButtonClicked();
}

//============================================================================
void TitleBarWidget::slotCamPreviewClicked( void )
{
	emit signalCamPreviewClicked();
    m_MyApp.getAppletMgr().launchApplet( eAppletCamSettings );
}

//============================================================================
void TitleBarWidget::slotMenuTopButtonClicked( void )
{
	emit signalMenuTopButtonClicked();
}

//============================================================================
void TitleBarWidget::slotBackButtonClicked( void )
{
	emit signalBackButtonClicked();
}

//======= button visibility ====//
//============================================================================
void TitleBarWidget::setPopupVisibility( bool hideBackButton )
{
    m_IsPopupDialog = true;
    setNetStatusVisibility( false );
    setOfferListButtonVisibility( false );
    setHostJoinRequestListButtonVisibility( false );

    setMenuTopButtonVisibility( false );
    setMenuListButtonVisibility( false );
    if( hideBackButton )
    {
        setBackButtonVisibility( false );
    }
}

//============================================================================
void TitleBarWidget::setBackButtonVisibility( bool visible )
{
    ui.m_BackDlgButton->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setCamPreviewVisibility( bool visible )
{
	ui.m_CamPreviewScreen->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setCamViewerCountVisibility( bool visible )
{
	ui.m_CamClientCountLabel->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setPowerButtonVisibility( bool visible )
{
	ui.m_PowerOffButton->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setNetStatusVisibility( bool visible )
{
    ui.m_NetAvailStatusWidget->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setHostJoinRequestListButtonVisibility( bool visible )
{
    ui.m_HostJoinRequestListButton->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setOfferListButtonVisibility( bool visible )
{
    ui.m_OfferListButton->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setMenuTopButtonVisibility( bool visible )
{
	ui.m_MenuTopButton->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setMenuListButtonVisibility( bool visible )
{
    ui.m_MenuButton->setVisible( visible );
}

//============================================================================
void TitleBarWidget::setCameraButtonVisibility( bool visible )
{
	ui.m_CameraStartStopButton->setVisible( visible );
}

//============ set button icon ============//
//============================================================================
void TitleBarWidget::setPowerButtonIcon( EMyIcons myIcon )
{
	ui.m_PowerOffButton->setIcon( myIcon );
}

//============================================================================
void TitleBarWidget::setMicrophoneIcon( EMyIcons myIcon )
{
	ui.m_MuteMicButton->setIcon( myIcon );
}

//============================================================================
void TitleBarWidget::setSpeakerIcon( EMyIcons myIcon )
{
	ui.m_MuteSpeakerButton->setIcon( myIcon );
}

//============================================================================
void TitleBarWidget::setCameraIcon( EMyIcons myIcon )
{
	ui.m_CameraStartStopButton->setIcon( myIcon );
}

//============================================================================
void TitleBarWidget::setTopMenuButtonIcon( EMyIcons myIcon )
{
	ui.m_MenuTopButton->setIcon( myIcon );
}

//============================================================================
void TitleBarWidget::setBackButtonIcon( EMyIcons myIcon )
{
	ui.m_BackDlgButton->setIcon( myIcon );
}

//============ set button color ============//
//============================================================================
void TitleBarWidget::setPowerButtonColor( QColor iconColor )
{
	ui.m_PowerOffButton->setIconOverrideColor( iconColor );
}

//============================================================================
void TitleBarWidget::setMicrophoneColor( QColor iconColor )
{
	ui.m_BackDlgButton->setIconOverrideColor( iconColor );
}

//============================================================================
void TitleBarWidget::setSpeakerColor( QColor iconColor )
{
	ui.m_MuteMicButton->setIconOverrideColor( iconColor );
}

//============================================================================
void TitleBarWidget::setCameraColor( QColor iconColor )
{
	ui.m_CameraStartStopButton->setIconOverrideColor( iconColor );
}

//============================================================================
void TitleBarWidget::setTopMenuButtonColor( QColor iconColor )
{
	ui.m_MenuTopButton->setIconOverrideColor( iconColor );
}

//============================================================================
void TitleBarWidget::setBackButtonColor( QColor iconColor )
{
	ui.m_BackDlgButton->setIconOverrideColor( iconColor );
}

//============================================================================
void TitleBarWidget::slotToGuiNetAvailStatus( ENetAvailStatus eNetAvailStatus )
{
    ui.m_NetAvailStatusWidget->toGuiNetAvailStatus( eNetAvailStatus );
    bool isCamEnabled = ICamCapture::getICamCapture().isCamCaptureRequested();
    callbackToGuiWantVideoCapture( isCamEnabled );
}

//============================================================================
void TitleBarWidget::slotHostJoinRequestButtonClicked( void )
{
    m_MyApp.getAppletMgr().launchApplet( eAppletHostJoinRequestList, getTitleBarParentPage()  );
}

//============================================================================
void TitleBarWidget::slotOfferListButtonClicked( void )
{
    m_MyApp.getAppletMgr().launchApplet( eAppletOfferList, getTitleBarParentPage() );
}

//============================================================================
void TitleBarWidget::slotTitleBarUserMenuButtonClicked( void )
{
    LogMsg( LOG_VERBOSE, "slotTitleBarUserMenuButtonClicked" );
    AppletPopupMenu* popupMenu = dynamic_cast<AppletPopupMenu*>(m_MyApp.getAppletMgr().launchApplet( eAppletPopupMenu, getTitleBarParentFrame() ) );
    if( popupMenu )
    {
        popupMenu->showTitleBarUserMenu();
    }
}

//============================================================================
QWidget* TitleBarWidget::getTitleBarParentFrame( void )
{
    QWidget* parentWdiget = dynamic_cast<QWidget*>(parent());
    QWidget* frame = GuiHelpers::findParentContentFrame( parentWdiget );
    if( frame )
    {
        parentWdiget = frame;
    }

    return parentWdiget;
}

//============================================================================
QWidget* TitleBarWidget::getTitleBarParentPage( void )
{
    QWidget* parentWdiget = dynamic_cast<QWidget*>(parent());
    QWidget* page = GuiHelpers::findParentPage( parentWdiget );
    if( page )
    {
        parentWdiget = page;
    }

    return parentWdiget;
}

//============================================================================
void TitleBarWidget::checkTitleBarIconsFit( void )
{
    int iconCnt = 5;
    if( ui.m_PowerOffButton->isVisible() )
    {
        iconCnt++;
    }

    if( ui.m_BackDlgButton->isVisible() )
    {
        iconCnt++;
    }

    if( ui.m_MuteMicButton->isVisible() )
    {
        iconCnt += 2;
    }

    if( ui.m_CamPreviewScreen->isVisible() )
    {
        iconCnt += 2;
    }

    if( !m_IsPopupDialog )
    {
        // remove icons from title bar if title bar cannot fit all of them
        if( !GuiParams::canFitIcons( eButtonSizeSmall, iconCnt, geometry().width() ) )
        {
            ui.m_MuteSpeakerButton->setVisible( false );
            ui.m_NetAvailStatusWidget->setVisible( false );
        }
        else
        {
            ui.m_MuteSpeakerButton->setVisible( true );
            ui.m_NetAvailStatusWidget->setVisible( true );
        }
    }
}

//============================================================================
void TitleBarWidget::slotApplicationIconClicked( void )
{
    if( m_MyApp.getIsAppInitialized() )
    {
        LogMsg( LOG_VERBOSE, "slotApplicationIconClicked" );
        AppletPopupMenu* applet = dynamic_cast<AppletPopupMenu*>(GetAppInstance().getAppletMgr().launchApplet( eAppletPopupMenu, dynamic_cast<QWidget*>(getTitleBarParentPage()) ));
		if( applet )
		{
			applet->showTitleBarAppMenu();
		}
    }
    else
    {
        GuiHelpers::showApplicationNotReadyError( false ); 
    }
}

//============================================================================
void TitleBarWidget::callbackToGuiWantMicrophoneRecording( bool wantMicInput )
{
    if( m_MicrophonePlaying != wantMicInput )
    {
        m_MicrophonePlaying = wantMicInput;
        if( wantMicInput )
        {
            ui.m_MicVolPeakBar->setValue( 0 );
        }

        checkTitleBarIconsFit();
    }
}

//============================================================================
void TitleBarWidget::callbackToGuiWantVideoCapture( bool wantVideoCapture )
{
    m_CamPlaying = wantVideoCapture;
    if( wantVideoCapture )
    {
        m_CamTimer->start( 500 );
    }
    else
    {
        m_CamTimer->stop();
        ui.m_CamPreviewScreen->setImageFromFile( m_MyApp.getCameraBackgroundFile() );
    }

    checkTitleBarIconsFit();
}

//============================================================================
void TitleBarWidget::callbackToGuiMicrophoneMuted( bool isMuted )
{
    m_MutedMic = isMuted;
    setMicrophoneIcon( m_MutedMic ? eMyIconMicrophoneOff : eMyIconMicrophoneOn );
}

//============================================================================
void TitleBarWidget::callbackToGuiSpeakerMuted( bool isMuted )
{
    m_MutedSpeaker = isMuted;
    setSpeakerIcon( m_MutedSpeaker ? eMyIconSpeakerOff : eMyIconSpeakerOn );
}

//============================================================================
void TitleBarWidget::callbackToGuiCameraEnable( bool enableCamera )
{
    if( m_CamEnabled == enableCamera )
    {
        return;
    }

    m_CamEnabled = enableCamera;
    if( !m_CamEnabled )
    {
        ui.m_CamPreviewScreen->setImageFromFile( m_MyApp.getCameraBackgroundFile() );
    }
}

//============================================================================
void TitleBarWidget::callbackToGuiCaptureRunning( bool camCaptureRunning )
{
    if( camCaptureRunning )
    {
        if( !ui.m_CamPreviewScreen->isVisible() )
        {
            ui.m_CamPreviewScreen->setVisible( true );
        }
    }
    else
    {
        ui.m_CamPreviewScreen->setImageFromFile( m_MyApp.getCameraBackgroundFile() );
        // sometimes a few frame are shown after call .. reset to background after a while
        m_CamTimer->start( 300 );
    }
}

//============================================================================
void TitleBarWidget::callbackGuiVideoTitleBarPixmap( QPixmap& vidPixmap )
{
    if( !m_CamEnabled || !isVisible() )
    {
        return;
    }

    int appAliveMs = GetApplicationAliveMs();
    // only allow 5 fps for title bar video to avoid bogging down the app with too many video frames
    if( appAliveMs - m_LastCamFrameTimeMs < 200 )
    {        
        return;
    }

    m_LastCamFrameTimeMs = appAliveMs;
    if( !m_CamPlaying )
    {
        m_CamPlaying = true;
    }

    if( ui.m_CamPreviewScreen->isVisible() )
    {
        ui.m_CamPreviewScreen->setPixmap( vidPixmap );
    }       
}

//============================================================================
void TitleBarWidget::callbackActiveOfferCount( int activeCnt, int historyCnt ) 
{
    ENotifyType notifyType{ eNotifyNone };
    if( activeCnt )
    {
        notifyType = eNotifyOnline;
    }
    else if( historyCnt )
    {
        notifyType = eNotifyOffline;
    }

    ui.m_OfferListButton->setNotifyType( notifyType );
    ui.m_OfferCountLabel->setVisible( activeCnt > 0 );
    ui.m_OfferCountLabel->setText( QString::number( activeCnt ) );
}

//============================================================================
void TitleBarWidget::callbackJoinRequestCount( int requestCnt )
{
    ui.m_HostJoinRequestListButton->setNotifyType( requestCnt > 0 ? eNotifyOnline : eNotifyOffline );
    ui.m_HostJoinRequestCountLabel->setVisible( requestCnt > 0 );
    ui.m_HostJoinRequestCountLabel->setText( QString::number( requestCnt ) );
}

//============================================================================
void TitleBarWidget::updateCamCallbackRequests( void )
{
    if( m_CallbacksRequested != m_VidCallbacksRequested )
    {
        if( m_CallbacksRequested )
        {
            // check if my online id is valid before requesting cam callbacks
            if( !m_MyOnlineId.isValid() )
            {
                m_MyOnlineId = m_MyApp.getMyOnlineId();
            }

            if( isVisible() && !m_VidCallbacksRequested && m_MyApp.getMyOnlineId().isValid() )
            {
                m_VidCallbacksRequested = true;
                m_MyApp.getPlayerMgr().setTitleBarVideoImageSize( ui.m_CamPreviewScreen->frameSize() );
                m_MyApp.getPlayerMgr().wantVideoTitleBarCallbacks( this, true );
            }
        }
        else
        {
            m_VidCallbacksRequested = false;
            m_MyApp.getPlayerMgr().wantVideoTitleBarCallbacks( this, false );
        }
    }
}

//============================================================================
void TitleBarWidget::updateAudioLevelCallbackRequests( void )
{
    if( m_CallbacksRequested != m_AudioLevelCallbacksRequested )
    {
        if( m_CallbacksRequested )
        {
            // check if my online id is valid before requesting cam callbacks
            if( !m_MyOnlineId.isValid() )
            {
                m_MyOnlineId = m_MyApp.getMyOnlineId();
            }

            if( isVisible() && !m_AudioLevelCallbacksRequested )
            {
                m_AudioLevelCallbacksRequested = true;
                m_MyApp.getAudioMgr().wantMicrophoneLevelCallbacks( this, true );
                setWantMicrophoneCount( m_MyApp.getAudioMgr().getWantMicrophoneCount() );
                setWantSpeakerCount( m_MyApp.getAudioMgr().getWantSpeakerCount() );
            }
        }
        else
        {
            m_AudioLevelCallbacksRequested = false;
            m_MyApp.getAudioMgr().wantMicrophoneLevelCallbacks( this, false );
        }
    }
}

//============================================================================
void TitleBarWidget::callbackGuiMicrophoneLevel( int micLevel )
{
    if( m_LastMicLevel != micLevel )
    {
        m_LastMicLevel = micLevel;
        ui.m_MicVolPeakBar->setValue( micLevel );
    }
}

//============================================================================
void TitleBarWidget::callbackWantMicrophoneCount( int wantMicCount )
{
    setWantMicrophoneCount( wantMicCount );
}

//============================================================================
void TitleBarWidget::callbackWantSpeakerCount( int wantSpeakerCount )
{
    setWantSpeakerCount( wantSpeakerCount );
}

//============================================================================
void TitleBarWidget::callbackGuiFriendRequestListUpdated( GuiFriendRequest* friendRequest )
{
    updateFriendRequestNotify();
}

//============================================================================
void TitleBarWidget::callbackGuiFriendRequestListRemoved( VxGUID requestId )
{
    updateFriendRequestNotify();
}

//============================================================================
void TitleBarWidget::slotFriendRequestListButtonClicked( void )
{
    m_MyApp.getAppletMgr().launchApplet( eAppletFriendRequestList, getTitleBarParentPage()  );
}

//============================================================================
void TitleBarWidget::slotTestButtonClicked( void )
{
    m_MyApp.getSoundFxMgr().playSnd( eSndDefIgnore );
}

//============================================================================
void TitleBarWidget::updateFriendRequestNotify( void )
{
    bool haveRequests = m_MyApp.getFriendRequestMgr().getRequestCount() != 0;
    ENotifyType notifyType = haveRequests ? eNotifyOnline : eNotifyOffline;

    ui.m_FriendRequestListButton->setNotifyType( notifyType );
}

//============================================================================
void TitleBarWidget::setWantMicrophoneCount( int wantMicCount )
{
    ui.m_WantMicCountLabel->setVisible( wantMicCount != 0 );
    ui.m_WantMicCountLabel->setText( QString::number( wantMicCount ) );
}

//============================================================================
void TitleBarWidget::setWantSpeakerCount( int wantSpeakerCount )
{
    ui.m_WantSpeakerCountLabel->setVisible( wantSpeakerCount != 0 );
    ui.m_WantSpeakerCountLabel->setText( QString::number( wantSpeakerCount ) );
}