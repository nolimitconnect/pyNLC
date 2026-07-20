//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VidWidget.h"

#include "AppCommon.h"

#include "AppletMgr.h"
#include "AppSettings.h"
#include "GuiHelpers.h"
#include "GuiParams.h"
#include "GuiPlayerMgr.h"
#include "HomeWindow.h"

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/IGlobalDb.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxTime.h>
#include <CoreLib/VxTimeUtil.h>

#include <GuiInterface/ICamCapture.h>

#include <QDebug>
#include <QTimer>
#include <QFile>

#include <time.h>

#include "ui_VidWidget.h"

namespace
{
	const int		MOTION_ALARM_EXPIRE_MS		= 5000;
	const int		MOTION_RECORD_EXPIRE_MS		= 30000;
}

//============================================================================
VidWidget::VidWidget(QWidget* parent)
: QWidget(parent)
, ui(*(new Ui::VidWidgetClass))
, m_MyApp( GetAppInstance() )
, m_Engine( m_MyApp.getEngine() )
, m_AppSettings( m_MyApp.getAppSettings() )
, m_MyOnlineId( m_MyApp.getMyOnlineId() )
, m_ThumbnailPreview( 0 )
, m_DisablePreview( false )
, m_DisableRecordControls( false )
, m_RecFriendName( "" )
, m_RecFilePath( "" )
, m_RecFileName( "" )
, m_IconToggle( false )
, m_IconToggleTimer( new QTimer( this ) )
, m_MotionAlarmOn( false )
, m_MotionAlarmDetected( false )
, m_MotionAlarmExpireTimer( new QTimer( this ) )
, m_MotionRecordOn( false )
, m_MotionRecordDetected( false )
, m_MotionRecordExpireTimer( new QTimer( this ) )
, m_InNormalRecord( false )
{
    setObjectName( "VxWidget" );
	ui.setupUi(this);
	showAllControls( false );
	m_ThumbnailPreview = new VxLabel( ui.m_VideoScreen );
    ui.m_VideoFrame->setThumbnailPreview( m_ThumbnailPreview );
    ui.m_VideoFrame->setVideoScreen( ui.m_VideoScreen );
    m_ThumbnailPreview->setImageFromFile( ":/AppRes/Resources/web_cam_buffering.png" );

	ui.m_CamSourceButton->setIcon( eMyIconCamSelectNormal );
	ui.m_CamSourceButton->setEnabled( ICamCapture::getICamCapture().getCameraCount() > 1 );
	ui.m_CamRotateButton->setIcon( eMyIconCamRotateNormal );
	ui.m_ImageRotateButton->setIcon( eMyIconImageRotateNormal );

	ui.m_VidFilesButton->setIcon( eMyIconFileBrowseNormal );
	ui.m_VidFilesButton->setEnabled( false );
	ui.m_PictureSnapshotButton->setIcon( eMyIconCameraNormal );
	ui.m_MotionRecordButton->setIcon( eMyIconRecordMotionNormal );
	ui.m_NormalRecordButton->setIcon( eMyIconRecordMovieNormal );
	ui.m_MotionAlarmButton->setIcon( eMyIconMotionAlarmWhite );

	ui.m_MotionBar->setTextVisible( false );
	ui.m_MotionBar->setRange( 0, 100000 );
	ui.m_MotionSensitivitySlider->setRange( 0, 100000 );

	connect( &m_MyApp,					SIGNAL(signalStatusMsg(QString)),		this, SLOT(slotStatusMsg(QString)) );

	connect( ui.m_VidFilesButton,		SIGNAL(clicked()),						this, SLOT(slotVidFilesButtonClicked()) );
	connect( ui.m_PictureSnapshotButton,SIGNAL(clicked()),						this, SLOT(slotPictureSnapshotButton()) );
	connect( ui.m_MotionAlarmButton,	SIGNAL(clicked()),						this, SLOT(slotMotionAlarmButtonClicked()) );
	connect( ui.m_MotionRecordButton,	SIGNAL(clicked()),						this, SLOT(slotRecMotionButtonClicked()) );
	connect( ui.m_NormalRecordButton,	SIGNAL(clicked()),						this, SLOT(slotRecNormalButtonClicked()) );

	connect( m_IconToggleTimer,			SIGNAL(timeout()),			this, SLOT(slotIconToggleTimeout()) );
	connect( m_MotionAlarmExpireTimer,	SIGNAL(timeout()),			this, SLOT(slotMotionAlarmTimeout()) );
	connect( m_MotionRecordExpireTimer,	SIGNAL(timeout()),			this, SLOT(slotMotionRecordTimeout()) );

	connect( ui.m_VideoScreen,			SIGNAL(clicked()),			this, SLOT(slotWidgetClicked()) );	
	connect( m_ThumbnailPreview,		SIGNAL(clicked()),			this, SLOT(slotWidgetClicked()) );	
	connect( ui.m_CamPreviewButton,		SIGNAL(clicked()),			this, SLOT(slotCamPreviewButtonClicked()) );	
	connect( ui.m_CamRotateButton,		SIGNAL(clicked()),			this, SLOT(slotCamRotateButtonClicked()) );	
	connect( ui.m_ImageRotateButton,	SIGNAL(clicked()),			this, SLOT(slotFeedRotateButtonClicked()) );

	connect( ui.m_CamSourceButton,		SIGNAL(clicked()),			this, SLOT(slotCamSourceButtonClicked()) );
	connect( ui.m_CamEnableButton,		SIGNAL(clicked()),			this, SLOT(slotCamEnableButtonClicked()) );

	ui.m_CamSourceButton->setEnabled( ICamCapture::getICamCapture().getCameraCount() > 1 );

    showUserMsgLabel( false );
	showOfflineImage();
	updateVidFeedImageRotation();
	updateCamEnable();

	// BRJ temp for testing
	//ui.m_VidFilesButton->setEnabled( true );
	ui.m_VidFilesButton->setVisible( false );
	applyVideoUiMode();
}

//============================================================================
VidWidget::~VidWidget()
{
	if( m_VideoFeedId.isValid() && eMediaModuleInvalid != m_MediaModule)
	{
		// stop previous feed
		m_Engine.fromGuiWantMediaInput( m_VideoFeedId, eMediaInputVideoJpg, m_MediaModule, m_VideoFeedId, false );
	}

	// Remove ALL registrations for this client so no stale pointer remains in GuiPlayerMgr
	m_MyApp.getPlayerMgr().wantPlayVideoCallbacks( m_MyApp.getMyOnlineId(), this, false );
	if( m_VideoFeedId.isValid() && m_VideoFeedId != m_MyOnlineId )
	{
		m_MyApp.getPlayerMgr().wantPlayVideoCallbacks( m_VideoFeedId, this, false );
	}
}

//============================================================================
MyIcons&  VidWidget::getMyIcons( void )
{
	return m_MyApp.getMyIcons();
}

//============================================================================
VxLabel * VidWidget::getVideoScreen( void )
{
	return ui.m_VideoScreen;
}

//============================================================================
void VidWidget::setAspectRatio( float aspectRatio )
{
    ui.m_VideoFrame->setVideoScreenAspectRatio( aspectRatio );
}

//============================================================================
void VidWidget::setVideoFeedId( VxGUID& feedOnlineId, EMediaModule mediaModule )
{ 
	m_MediaModule = mediaModule;
	if( feedOnlineId != m_VideoFeedId )
	{
		if( m_VideoFeedId.isValid() )
		{
			// stop previous feed
			m_MyApp.getPlayerMgr().wantPlayVideoCallbacks( m_VideoFeedId, this, false );
		}

		m_VideoFeedId = feedOnlineId; 
		if( m_VideoFeedId == m_MyOnlineId )
		{
			disablePreview( true );
		}

		if( m_VideoFeedId.isValid() )
		{
			m_MyApp.getPlayerMgr().wantPlayVideoCallbacks( m_VideoFeedId, this, true );
            // trigger MediaProcessor to send jpgs
            if( eMediaModuleInvalid != m_MediaModule )
            {
                m_Engine.fromGuiWantMediaInput( m_VideoFeedId, eMediaInputVideoJpg, m_MediaModule, m_VideoFeedId, true );
            }
		}
		else
		{
			showOfflineImage();
		}
	}
}

//============================================================================
void VidWidget::setVideoUiMode( EVideoUiMode videoUiMode )
{
	if( m_VideoUiMode != videoUiMode )
	{
		m_VideoUiMode = videoUiMode;
		applyVideoUiMode();
	}
}

//============================================================================
void VidWidget::applyVideoUiMode( void )
{
	// Reset all optional controls so repeated mode switches never leave stale UI behind.
	showFeedControls( false );
	showRecordControls( false );
	showMotionSensitivityControls( false );

	ui.m_CamRotateButton->setVisible( false );
	ui.m_CamSourceButton->setVisible( false );
	ui.m_CamEnableButton->setVisible( false );
	ui.m_CamPreviewButton->setVisible( false );
	ui.m_ImageRotateButton->setVisible( false );
	ui.m_PictureSnapshotButton->setVisible( false );
	ui.m_MotionAlarmButton->setVisible( false );
	ui.m_MotionRecordButton->setVisible( false );
	ui.m_NormalRecordButton->setVisible( false );
	ui.m_RecordSensitivityFrame->setVisible( false );
	ui.m_MotionBar->setVisible( false );
	ui.m_MotionSensitivitySlider->setVisible( false );

	switch( m_VideoUiMode )
	{
	case eVideoUiModePhoto:
		showFeedControls( true );
		disablePreview( true );
		ui.m_ImageRotateButton->setVisible( true );
		break;

	case eVideoUiModeInputWidget:
		disablePreview( true );
		break;

	case eVideoUiModeInputPhoto:
		showFeedControls( true );
		disablePreview( true );
		//ui.m_CamRotateButton->setVisible( true );
		ui.m_CamSourceButton->setVisible( true );
        ui.m_ImageRotateButton->setVisible( true );
		ui.m_PictureSnapshotButton->setVisible( true );
		break;

	case eVideoUiModeAssetVideo:
		showFeedControls( true );
		disablePreview( true );
		ui.m_ImageRotateButton->setVisible( true );
		break;

	case eVideoUiModeCamServerClient:
		showFeedControls( true );
		showRecordControls( true );
		disablePreview( true );
		ui.m_ImageRotateButton->setVisible( true );
		ui.m_PictureSnapshotButton->setVisible( true );
		ui.m_MotionAlarmButton->setVisible( true );
		ui.m_MotionRecordButton->setVisible( true );
		ui.m_NormalRecordButton->setVisible( true );
		// Local camera source/preview controls are not applicable when viewing a remote cam server feed.
		ui.m_CamRotateButton->setVisible( false );
		ui.m_CamSourceButton->setVisible( false );
		ui.m_CamEnableButton->setVisible( false );
		ui.m_CamPreviewButton->setVisible( false );
		break;

	case eVideoUiModeChat:
	default:
		// Preserve current full behavior for chat pages; click-to-toggle handles visibility.
		disablePreview( false );
		showFeedControls( false );
		showRecordControls( false );
		ui.m_CamRotateButton->setVisible( true );
		ui.m_CamSourceButton->setVisible( true );
		ui.m_CamEnableButton->setVisible( true );
		ui.m_CamPreviewButton->setVisible( true );
		ui.m_ImageRotateButton->setVisible( true );
		ui.m_PictureSnapshotButton->setVisible( true );
		ui.m_MotionAlarmButton->setVisible( true );
		ui.m_MotionRecordButton->setVisible( true );
		ui.m_NormalRecordButton->setVisible( true );
		break;
	}
}

//============================================================================
void VidWidget::showOfflineImage( void )
{
	QString bkgFile = m_MyApp.getCameraBackgroundFile();
	ui.m_VideoScreen->setImageFromFile( bkgFile );
}

//============================================================================
void VidWidget::showUserMsgLabel( bool showCtrls )
{
    ui.m_UserMsgLabel->setVisible( showCtrls );
}

//============================================================================
void VidWidget::showAllControls( bool showCtrls )
{
	showFeedControls( showCtrls );
	showRecordControls( showCtrls );
	showMotionSensitivityControls( showCtrls );	
}

//============================================================================
void VidWidget::enableVidFilesButton( bool enable )
{
	ui.m_VidFilesButton->setEnabled( enable );
}

//============================================================================
void VidWidget::enableCamSourceControls( bool enable )
{
	ui.m_CamRotateButton->setEnabled( enable );
	ui.m_CamPreviewButton->setEnabled( enable );
}

//============================================================================
void VidWidget::enableCamFeedControls( bool enable )
{
	ui.m_ImageRotateButton->setEnabled( enable );
}

//============================================================================
void VidWidget::showFeedControls( bool showCtrls )
{
	ui.m_VidCtrlFrame->setVisible( showCtrls );
}

//============================================================================
bool VidWidget::isFeedControlsVisible( void )
{
	return ui.m_VidCtrlFrame->isVisible();
}

//============================================================================
void VidWidget::showRecordControls( bool showCtrls )
{
	ui.m_RecordCtrlFrame->setVisible( showCtrls );
	showMotionSensitivityControls( showCtrls );
}

//============================================================================
bool VidWidget::isRecordControlsVisible( void )
{
	return ui.m_RecordCtrlFrame->isVisible();
}

//============================================================================
void VidWidget::showMotionSensitivityControls( bool showCtrls )
{
	ui.m_RecordSensitivityFrame->setVisible( showCtrls );
}

//============================================================================
bool VidWidget::isMotionSensitivityControlsVisible( void )
{
	return ui.m_RecordSensitivityFrame->isVisible();
}

//============================================================================
void VidWidget::setVidImageRotation( int imageRotation )
{
	ui.m_VideoScreen->setVidImageRotation( imageRotation );
}

//============================================================================
int VidWidget::getVidImageRotation( void )
{
	return ui.m_VideoScreen->getVidImageRotation();
}

//============================================================================
void VidWidget::slotStatusMsg( QString userMsg )
{
	ui.m_UserMsgLabel->setText( userMsg );
}

//============================================================================
void VidWidget::slotWidgetClicked( void )
{
	emit clicked();
	bool visState = ! isFeedControlsVisible();
	showFeedControls( visState );
	if( !m_DisableRecordControls )
	{
		showRecordControls( visState );
	}
}

//============================================================================
void VidWidget::slotCamPreviewButtonClicked( void )
{
	if( !m_DisablePreview )
	{
		bool isVisible = m_AppSettings.getCamShowPreview();
		m_AppSettings.setCamShowPreview( !isVisible );
		updatePreviewVisibility();
		if( !isVisible && ! m_DisablePreview )
		{
			m_MyApp.toGuiUserMessage( "Cam Preview Enabled" );
		}
		else
		{
			m_MyApp.toGuiUserMessage( "Cam Preview Disabled" );
		}
	}
}

//============================================================================
void VidWidget::updatePreviewVisibility( void )
{
	bool showPreview = m_AppSettings.getCamShowPreview();
	if( m_DisablePreview )
	{
		showPreview = false;
	}

	m_ThumbnailPreview->setVisible( showPreview );
	if( showPreview )
	{
		ui.m_CamPreviewButton->setIcon( eMyIconCamPreviewCancelNormal );
		if( eMediaModuleInvalid != getMediaModule() )
		{
			m_Engine.fromGuiWantMediaInput( m_MyOnlineId, eMediaInputVideoJpg, getMediaModule(), m_MyOnlineId, true);
		}
	}
	else
	{
		ui.m_CamPreviewButton->setIcons( eMyIconCamPreviewNormal );
		if( eMediaModuleInvalid != getMediaModule() )
		{
			m_Engine.fromGuiWantMediaInput( m_MyOnlineId, eMediaInputVideoJpg, getMediaModule(), m_MyOnlineId, false );
		}
	}
}

//============================================================================
void VidWidget::slotFeedRotateButtonClicked( void )
{
	if( eVideoUiModePhoto == m_VideoUiMode )
	{
		int imageRotation = ui.m_VideoScreen->getVidImageRotation();
		imageRotation += 90;
		if( imageRotation >= 360 )
		{
			imageRotation = 0;
		}

		setVidImageRotation( imageRotation );
		QImage curImage = ui.m_VideoScreen->getLastVideoImage();
		if( curImage.isNull() )
		{
			curImage = m_StillImage;
		}
		if( curImage.isNull() && !m_StillImageFileName.isEmpty() )
		{
			curImage.load( m_StillImageFileName );
		}
		if( !curImage.isNull() )
		{
			ui.m_VideoScreen->playVideoFrame( curImage );
		}

		m_MyApp.toGuiUserMessage( "Photo Rotation %d", imageRotation );
		return;
	}

	int feedRotation = IGlobalDb::getIGlobalDb().getVidFeedRotation();
	feedRotation += 90;
	if( feedRotation >= 360 )
	{
		feedRotation = 0;
	}

	IGlobalDb::getIGlobalDb().setVidFeedRotation( feedRotation );
	updateVidFeedImageRotation();
	m_MyApp.toGuiUserMessage( "Contact Feed Rotation %d", feedRotation );
	emit signalFeedRotationChanged( feedRotation );
}

//============================================================================
void VidWidget::updateVidFeedImageRotation( void )
{
	int feedRotation = IGlobalDb::getIGlobalDb().getVidFeedRotation();
	setVidImageRotation( feedRotation );
}

//============================================================================
void VidWidget::slotCamRotateButtonClicked( void )
{
	int camRotation = ICamCapture::getICamCapture().rotateCurrentCamCapture();
    
	m_MyApp.toGuiUserMessage( "My Cam Rotation %d", camRotation );
	emit signalCamRotationChanged( camRotation );
}

//============================================================================
void VidWidget::disablePreview( bool disable )
{
	m_DisablePreview = disable;
	if( m_DisablePreview )
	{
		m_ThumbnailPreview->setVisible( false );
		ui.m_CamPreviewButton->setVisible( false );
	}
	else
	{
		ui.m_CamPreviewButton->setVisible( true );
		updatePreviewVisibility();
	}
}

//============================================================================
bool VidWidget::setImageFromFile( QString fileName )
{
	bool result = ui.m_VideoScreen->setImageFromFile( fileName );
	if( result )
	{
		m_StillImageFileName = fileName;
		m_StillImage.load( fileName );
	}

	return result;
}

//============================================================================
void VidWidget::callbackGuiPlayMotionVideoFrame( VxGUID& feedOnlineId, QImage& vidFrame, int motion0To100000 )
{
    if( m_FreezeFrameEnabled )
    {
        // Don't update the video screen if freeze frame is enabled to allow the last captured frame to remain visible.
        return;
    }

	if( feedOnlineId == m_VideoFeedId )
	{
		ui.m_VideoScreen->playMotionVideoFrame( vidFrame, motion0To100000 );
		if( ui.m_RecordSensitivityFrame->isVisible() )
		{
			updateVidFeedMotion( motion0To100000 );
		}
	}
	else if( !m_DisablePreview
			&& m_ThumbnailPreview->isVisible() 
			&& (feedOnlineId == m_MyOnlineId ) )
	{
		m_ThumbnailPreview->playMotionVideoFrame( vidFrame, motion0To100000 );
	}
}

//============================================================================
void VidWidget::callbackGuiPlayVideoFrame( VxGUID& onlineId, QImage& vidFrame )
{
    if( m_FreezeFrameEnabled )
    {
        // Don't update the video screen if freeze frame is enabled to allow the last captured frame to remain visible.
        return;
    }

	 ui.m_VideoScreen->playVideoFrame( vidFrame );
}

//============================================================================
void VidWidget::showEvent( QShowEvent* ev )
{
	QWidget::showEvent( ev );
	updatePreviewVisibility();
	if( m_VideoFeedId.isValid() )
	{
		if( eMediaModuleInvalid != getMediaModule() )
		{
			m_Engine.fromGuiWantMediaInput( m_VideoFeedId, eMediaInputVideoJpg, getMediaModule(), m_VideoFeedId, true );
		}
		m_MyApp.getPlayerMgr().wantPlayVideoCallbacks( m_VideoFeedId, this, true );
	}
}

//============================================================================
void VidWidget::hideEvent( QHideEvent* ev )
{
	QWidget::hideEvent( ev );
	if( m_VideoFeedId.isValid() )
	{
		m_MyApp.getPlayerMgr().wantPlayVideoCallbacks( m_VideoFeedId, this, false );
		if( eMediaModuleInvalid != getMediaModule() )
		{
			m_Engine.fromGuiWantMediaInput( m_VideoFeedId, eMediaInputVideoJpg, getMediaModule(), m_VideoFeedId, false );
		}
	}
}

//============================================================================
void VidWidget::slotIconToggleTimeout( void )
{
	ui.m_MotionAlarmButton->setNotifyType( m_MotionAlarmDetected ? eNotifyOnline : eNotifyOffline  );
	ui.m_MotionRecordButton->setNotifyType( m_MotionRecordDetected ? eNotifyOnline : eNotifyOffline );
	ui.m_NormalRecordButton->setNotifyType( m_InNormalRecord ? eNotifyOnline : eNotifyOffline );
}

//============================================================================
void VidWidget::slotMotionAlarmTimeout( void )
{
	m_MotionAlarmExpireTimer->stop();
	m_MotionAlarmDetected = false;
	ui.m_MotionAlarmButton->setNotifyType( eNotifyOffline );
	ui.m_MotionAlarmButton->setIcon( m_MotionAlarmOn ? eMyIconMotionAlarmRed : eMyIconMotionAlarmWhite );

	updateMotionBarColor();
}

//============================================================================
void VidWidget::slotMotionRecordTimeout( void )
{
	m_MotionRecordExpireTimer->stop();
	m_MotionRecordDetected = false;
	ui.m_MotionRecordButton->setNotifyType( eNotifyOffline );
	if( m_MotionRecordOn )
	{
		m_Engine.fromGuiVideoRecord( eVideoRecordStatePauseRecording, m_VideoFeedId, m_RecFileName.toUtf8().constData() );
	}

	updateMotionBarColor();
}

//============================================================================
void VidWidget::updateVidFeedMotion( int motion0To100000 )
{
	ui.m_MotionBar->setValue( motion0To100000 );
	ui.m_MotionBar->update();
	if( motion0To100000 >= ui.m_MotionSensitivitySlider->value() )
	{
		if( m_MotionRecordOn )
		{
			m_MotionRecordExpireTimer->start( MOTION_RECORD_EXPIRE_MS );
			if( !m_MotionRecordDetected )
			{
				m_MotionRecordDetected = true;
				m_Engine.fromGuiVideoRecord( eVideoRecordStateResumeRecording, m_VideoFeedId, m_RecFileName.toUtf8().constData() );
				m_MyApp.toGuiUserMessage( "Video Motion Record Resumed" );
				updateMotionBarColor();
				ui.m_MotionRecordButton->setNotifyType( eNotifyOnline );
			}
		}

		if( m_MotionAlarmOn )
		{
			m_MotionAlarmExpireTimer->start( MOTION_ALARM_EXPIRE_MS );
			if( !m_MotionAlarmDetected )
			{
				m_MotionAlarmDetected = true;
				playMotionAlarm();
				updateMotionBarColor();
				ui.m_MotionAlarmButton->setNotifyType( eNotifyOnline );
			}
		}
	}
}

//============================================================================
void VidWidget::updateMotionBarColor( void )
{
	if( m_MotionAlarmDetected || m_MotionRecordDetected )
	{
		//ui.m_MotionBar->setStyleSheet( "QProgressBar {background-color: #FF0000;}" );
	}
	else
	{
		//ui.m_MotionBar->setStyleSheet( "QProgressBar {background-color: #00FF00;}" );
	}
}

//============================================================================
void VidWidget::playMotionAlarm( void )
{
	m_MyApp.playSound( eSndDefAlarmPleasant );
}

//============================================================================
void VidWidget::slotVidFilesButtonClicked( void )
{
	if( !m_MotionRecordOn
		&& !m_InNormalRecord )
	{
		m_MyApp.getAppletMgr().launchApplet( eAppletLibrary, &m_MyApp.getHomeWindow(), GuiParams::describeFileFilter( eFileFilterVideo ) );
	}
	else
	{
		LogMsg( LOG_ERROR, "VidWidget::slotVidFilesButtonClicked dont launch Library Activity while recording" );
	}
}

//============================================================================
void VidWidget::slotPictureSnapshotButton( void )
{
	QImage photoImage = getVideoScreen()->getLastVideoImage();
	if( photoImage.isNull() )
	{
		m_MyApp.playSound( eSndDefIgnore );
		LogMsg( LOG_ERROR, "VidWidget::slotPictureSnapshotButton video image is null\n" );
		m_MyApp.toGuiUserMessage( "No Image To Take A Snap Shot Of" );
	}
	else
	{
		m_MyApp.playSound( eSndDefCameraClick );
		QString photoFileName = m_RecFilePath + m_RecFriendName + VxTimeUtil::getFileNameCompatibleDateAndTime( GetLocalTimeMs() ).c_str();
		photoFileName += ".png";
		QFile photoFile(photoFileName);
		photoFile.open(QIODevice::WriteOnly);
		photoImage.save(&photoFile, "PNG");
		photoFile.close();

		FileInfo fileInfo;
		if( VxFileUtil::getFileInfo( photoFileName.toUtf8().constData(), fileInfo ) && fileInfo.getFileLength() > 500 )
		{
			QString thumbFileName;
			if( GuiHelpers::generateThumbFromImageFile( photoFileName.toUtf8().constData(), fileInfo.getThumbId(), thumbFileName ) )
			{
				ThumbInfo thumbInfo;
				if( !GuiHelpers::addThumbAsset( m_MyApp, thumbFileName, fileInfo.getThumbId(), thumbInfo ) )
				{
					fileInfo.getThumbId().clear();
				}
			}

			m_MyApp.getEngine().fromGuiSetFileIsInLibrary( fileInfo, true );
		}
		else
		{
			m_MyApp.toGuiUserMessage( "ERROR: Snapshot file create failed" );
			VxFileUtil::deleteFile( photoFileName.toUtf8().constData() );
		}
	}
}

//============================================================================
void VidWidget::slotMotionAlarmButtonClicked( void )
{
	if( m_MotionAlarmOn )
	{
		m_MotionAlarmExpireTimer->stop();
		m_MotionAlarmDetected = false;
	}

	m_MotionAlarmOn = !m_MotionAlarmOn;
	ui.m_MotionAlarmButton->setNotifyType( eNotifyOffline );
	ui.m_MotionAlarmButton->setIcon( m_MotionAlarmOn ? eMyIconMotionAlarmRed : eMyIconMotionAlarmWhite );
	if( m_MotionAlarmOn )
	{
		showMotionSensitivityControls( true );
		m_MyApp.toGuiUserMessage( "Motion Alarm Enabled" );
	}
	else
	{
		m_MyApp.toGuiUserMessage( "Motion Alarm Disabled" );
	}
}

//============================================================================
void VidWidget::slotRecMotionButtonClicked( void )
{
	if( !m_InNormalRecord )
	{
		if( !m_RecFilePath.isEmpty() )
		{
			ui.m_MotionRecordButton->setNotifyType( eNotifyOffline );

			if( m_MotionRecordOn )
			{
				ui.m_MotionRecordButton->setIcon( eMyIconRecordMotionNormal );
				m_MotionRecordOn = false;
				m_Engine.fromGuiVideoRecord( eVideoRecordStateStopRecording, m_VideoFeedId, m_RecFileName.toUtf8().constData() );

				addVideoFileToLibrary( m_RecFileName.toUtf8().constData() );

			}
			else
			{
				enableVidFilesButton( false );
				m_MotionRecordDetected = false;
				m_MotionRecordOn = true;
				m_RecFileName = m_RecFilePath + m_RecFriendName + VxTimeUtil::getFileNameCompatibleDateAndTime( GetLocalTimeMs() ).c_str();
				m_RecFileName += ".avi";
				m_Engine.fromGuiVideoRecord( eVideoRecordStateStartRecordingInPausedState, m_VideoFeedId, m_RecFileName.toUtf8().constData() );
				ui.m_MotionRecordButton->setNotifyType( eNotifyOnline );
				m_MyApp.toGuiUserMessage( "Video Motion Record Started" );
			}
		}
	}
	else
	{
		m_MyApp.toGuiUserMessage( "Already recording video without motion detect" );
	}

	//if( !m_RecFilePath.isEmpty() )
	//{
	//	if( m_MotionAlarmOn )
	//	{
	//		showMotionSensitivityControls( true );
	//	}
	//	else
	//	{
	//		if( false == m_MotionAlarmOn )
	//		{
	//			showMotionSensitivityControls( false );
	//		}
	//	}
	//}
}

//============================================================================
void VidWidget::slotRecNormalButtonClicked( void )
{
	if( !m_MotionRecordOn )
	{
		if( !m_RecFilePath.isEmpty() )
		{
			if( m_InNormalRecord )
			{
				ui.m_NormalRecordButton->setIcon( eMyIconRecordMovieNormal );
				ui.m_NormalRecordButton->setNotifyType( eNotifyOffline );
				m_InNormalRecord = false;

				m_Engine.fromGuiVideoRecord( eVideoRecordStateStopRecording, m_VideoFeedId, m_RecFileName.toUtf8().constData() );

				addVideoFileToLibrary( m_RecFileName.toUtf8().constData() );
			}
			else
			{
				enableVidFilesButton( false );
				m_InNormalRecord = true;
				m_RecFileName = m_RecFilePath + m_RecFriendName + VxTimeUtil::getFileNameCompatibleDateAndTime( GetLocalTimeMs() ).c_str();
				m_RecFileName += ".avi";
				m_Engine.fromGuiVideoRecord( eVideoRecordStateStartRecording, m_VideoFeedId, m_RecFileName.toUtf8().constData() );
				ui.m_NormalRecordButton->setNotifyType( eNotifyOnline );
				m_MyApp.toGuiUserMessage( "Starting Video Record" );
			}
		}
	}
}

//============================================================================
void VidWidget::slotCamSourceButtonClicked( void )
{
	ICamCapture::getICamCapture().nextCamera();
}

//============================================================================
void VidWidget::slotCamEnableButtonClicked( void )
{
	ICamCapture::getICamCapture().setCamCaptureEnable(!ICamCapture::getICamCapture().getCamCaptureEnable());
	updateCamEnable();
}

//============================================================================
void VidWidget::updateCamEnable( void )
{
	if( ICamCapture::getICamCapture().getCamCaptureEnable() )
	{
		ui.m_CamEnableButton->setIcon( eMyIconCamcorderCancel );
	}
	else
	{
		ui.m_CamEnableButton->setIcon( eMyIconCamcorderNormal );
	}
}

//============================================================================
void VidWidget::addVideoFileToLibrary( std::string vidFileName )
{
	FileInfo fileInfo;
	if( VxFileUtil::getFileInfo( vidFileName.c_str(), fileInfo ) && fileInfo.getFileLength() > 5000 )
	{
		//QString thumbFileName;
		//if( GuiHelpers::generateThumbFromImageFile( photoFileName.toUtf8().constData(), fileInfo.getThumbId(), thumbFileName ) )
		//{
		//	ThumbInfo thumbInfo;
		//	if( !GuiHelpers::addThumbAsset( m_MyApp, thumbFileName, fileInfo.getThumbId(), thumbInfo ) )
		//	{
		//		fileInfo.getThumbId().clear();
		//	}
		//}

		m_MyApp.getEngine().fromGuiSetFileIsInLibrary( fileInfo, true );
		m_MyApp.toGuiUserMessage( "Added motion video to library %s", m_RecFileName.toUtf8().constData() );
		enableVidFilesButton( true );
	}
	else
	{
		VxFileUtil::deleteFile( m_RecFileName.toUtf8().constData() );
		LogMsg( LOG_ERROR, "VidWidget::videoMotionRecord file %s is too short", m_RecFileName.toUtf8().constData() );
		m_MyApp.toGuiUserMessage( "ERROR: Motion video record file was too short" );
	}
}

//============================================================================
void VidWidget::enableFreezeFrame( bool enable )
{
    if(	enable )
	{
		QImage curImage = ui.m_VideoScreen->getLastVideoImage();
		if( !curImage.isNull() )
        {
			ui.m_VideoScreen->playVideoFrame( curImage );
		}
    }

    m_FreezeFrameEnabled = enable;
}