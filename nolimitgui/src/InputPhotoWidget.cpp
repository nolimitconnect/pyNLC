//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "InputPhotoWidget.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "ChatEntryWidget.h"
#include "GuiParams.h"
#include "InputClientBaseCallback.h"
#include "VxLabel.h"

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxFileUtil.h>

#include <GuiInterface/ICamCapture.h>

#include <time.h>

#include <QMessageBox>

#include "ui_InputPhotoWidget.h"

//============================================================================
InputPhotoWidget::InputPhotoWidget( QWidget* parent )
: InputBaseWidget( GetAppInstance(), parent )
, ui(*(new Ui::InputPhotoWidget))
{
	m_AssetInfo.setAssetType( eAssetTypePhoto );
    // qDebug() << "InputPhotoWidget::InputPhotoWidget ";

	ui.setupUi( this );
    ui.m_SnapShotButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_RotateCamButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_SelectCamButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_CancelPhotoButton->setSquareButtonSize( eButtonSizeTiny );
	ui.m_BackButton->setSquareButtonSize( eButtonSizeTiny );

	ui.m_SnapShotButton->setIcons( eMyIconCameraNormal );
	ui.m_RotateCamButton->setIcons( eMyIconCamRotateNormal );
	ui.m_SelectCamButton->setIcons( eMyIconCamSelectNormal );
	ui.m_CancelPhotoButton->setIcons( eMyIconCameraCancel );
	// TODO.. add camera selection
	ui.m_SelectCamButton->setEnabled( false );

    ui.m_BackButton->setIcons( eMyIconBack );
	ui.m_VidWidget->setVideoUiMode( eVideoUiModeInputPhoto );

	connect( ui.m_SnapShotButton,				SIGNAL(clicked()),							this, SLOT(slotSnapShotButtonClicked()) );
	connect( ui.m_RotateCamButton,				SIGNAL(clicked()),							this, SLOT(slotRotateCamButtonClicked()) );
	connect( ui.m_SelectCamButton,				SIGNAL(clicked()),							this, SLOT(slotSelectCamButtonClicked()) );
	connect( ui.m_CancelPhotoButton,			SIGNAL(clicked()),							this, SLOT(slotCancelPhotoButtonClicked()) );
    // connect( ui.m_VidWidget->getVideoScreen(),	SIGNAL(signalPlayVideoFrame( QImage,int)),	this, SLOT(slotVideoFrameBitmap( QImage,int)) );
    connect( ui.m_BackButton, SIGNAL(clicked()), this, SLOT(slotExitPhotoWidget() ) );
}

//============================================================================
void InputPhotoWidget::slotSnapShotButtonClicked( void )
{
	m_PicImage = ui.m_VidWidget->getVideoScreen()->getLastVideoImage();
	if( m_PicImage.isNull() )
	{
		QMessageBox::warning(this, QObject::tr("Photo Snapshot Error"), QObject::tr("Photo Unavailable" ) );
	}
	else
	{
		if( fillAssetBaseInfo( true ) )
		{
			generateFileName( eAssetTypePhoto, m_AssetInfo.getAssetUniqueId() );

			m_AssetInfo.setAssetNameFromFileNameAndPath( m_FileName );

			if( false == m_PicImage.save( m_FileName.c_str() ) )
			{
				QString msgText = QObject::tr( "Failed to write photo to file " ) + m_FileName.c_str();
				QMessageBox::warning( this, QObject::tr( "Error Writing Photo To File" ), msgText );
			}
			else
			{
				uint64_t fileLen = VxFileUtil::getFileLen( m_FileName.c_str() );
				if( 1000 > fileLen )
				{
					// not long enough to be a photo
					VxFileUtil::deleteFile( m_FileName.c_str() );
					QMessageBox::warning( this, QObject::tr( "Photo Snapshot Error" ), QObject::tr( "Photo File Not Found" ) );
				}
				else
				{
					m_AssetInfo.setAssetLength( fileLen );
					if( addOptionalComment() )
					{
						m_ClientCallback->handleAssetAction( m_IsPersonalRecorder ? eAssetActionAddToAssetMgr : eAssetActionAddAssetAndSend, m_AssetInfo );
					}
				}
			}
		}
	}

	emit signalInputCompleted();
}

//============================================================================
void InputPhotoWidget::slotRotateCamButtonClicked( void )
{
	ICamCapture::getICamCapture().rotateCurrentCamCapture();

    // always keep the video on screen at zero rotation. capture system should rotate it
    ui.m_VidWidget->setVidImageRotation( 0 );
}

//============================================================================
void InputPhotoWidget::slotSelectCamButtonClicked( void )
{
}

//============================================================================
void InputPhotoWidget::slotCancelPhotoButtonClicked( void )
{
	emit signalInputCompleted();
}

//============================================================================
void InputPhotoWidget::hideEvent(QHideEvent* hideEvent)
{
	InputBaseWidget::hideEvent(hideEvent);
    if( m_GroupieId.isHostedIdValid() && ( false == VxIsAppShuttingDown() ) )
	{
		m_MyApp.getEngine().fromGuiWantMediaInput( m_AssetInfo.getHistoryId(), eMediaInputVideoJpg, getMediaModule(), m_AssetInfo.getHistoryId(), false );
	}
}

//============================================================================
void InputPhotoWidget::showEvent(QShowEvent* showEvent)
{
	InputBaseWidget::showEvent(showEvent);
    if( m_GroupieId.isHostedIdValid() && ( false == VxIsAppShuttingDown() ) )
	{
		ui.m_VidWidget->setVideoFeedId( m_AssetInfo.getCreatorId(), getMediaModule() );
		m_MyApp.getEngine().fromGuiWantMediaInput( m_AssetInfo.getHistoryId(), eMediaInputVideoJpg, getMediaModule(), m_AssetInfo.getHistoryId(), true );
	}
}

//============================================================================
void InputPhotoWidget::callbackGuiPlayMotionVideoFrame( VxGUID& feedOnlineId, QImage& vidFrame, int motion0To100000 )
{
	ui.m_VidWidget->callbackGuiPlayMotionVideoFrame( feedOnlineId, vidFrame, motion0To100000 );
}

//============================================================================
void InputPhotoWidget::slotExitPhotoWidget( void )
{
    this->setVisible( false );
    m_ChatEntryWidget->setEntryMode( eAssetTypeUnknown );
    m_ChatEntryWidget->setVisible( true );
}
