//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "InputVideoWidget.h"

#include "AppCommon.h"
#include "AppSettings.h"
#include "ChatEntryWidget.h"
#include "GuiHelpers.h"
#include "GuiParams.h"

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxGlobals.h>

#include <GuiInterface/ICamCapture.h>

#include "ui_InputVideoWidget.h"

//============================================================================
InputVideoWidget::InputVideoWidget( QWidget* parent )
: InputBaseWidget( GetAppInstance(), parent )
, ui(*(new Ui::InputVideoWidget))
{
    // qDebug() << "InputVideoWidget::InputVideoWidget ";
	m_AssetInfo.setAssetType( eAssetTypeVideo );

	ui.setupUi( this );
    ui.m_CancelRecordButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_StartStopRecButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_RotateCamButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_SelectVidSrcButton->setSquareButtonSize( eButtonSizeTiny );
    ui.m_BackButton->setSquareButtonSize( eButtonSizeTiny );

	ui.m_CancelRecordButton->setIconOverrideColor( m_MyApp.getAppTheme().getCancelColor() );
	ui.m_CancelRecordButton->setIcons( eMyIconCancelRecord );
    ui.m_CancelRecordButton->setVisible( false );

    ui.m_BackButton->setIcons( eMyIconBack );

	ui.m_StartStopRecButton->setIsToggleButton( true );
	ui.m_StartStopRecButton->setIcons( eMyIconCamcorderNormal );
	ui.m_RotateCamButton->setIcons( eMyIconCamRotateNormal );
	ui.m_SelectVidSrcButton->setIcons( eMyIconCamSelectNormal );
	ui.m_SelectVidSrcButton->setEnabled( false );
    ui.m_SelectVidSrcButton->setVisible( false );
	ui.m_VidWidget->setVideoUiMode( eVideoUiModeInputWidget );
    
	connect( ui.m_StartStopRecButton,		SIGNAL(clicked()),	this, SLOT(slotBeginRecord()) );
	connect( ui.m_RotateCamButton,			SIGNAL(clicked()),	this, SLOT(slotRotateCamButtonClicked()) );
	connect( ui.m_CancelRecordButton,		SIGNAL(clicked()),	this, SLOT(slotRecordCancelButtonClicked()) );
    connect( ui.m_BackButton, SIGNAL(clicked()), this, SLOT(slotExitVideoWidget() ) );
    ui.m_VidWidget->enableFreezeFrame( false );
}

//============================================================================
void InputVideoWidget::showEvent(QShowEvent* showEvent)
{
	InputBaseWidget::showEvent(showEvent);
    if( m_GroupieId.isHostedIdValid() && ( false == VxIsAppShuttingDown() ) )
	{
		ui.m_VidWidget->setVideoFeedId( m_AssetInfo.getCreatorId(), getMediaModule() );
		m_MyApp.getEngine().fromGuiWantMediaInput( m_AssetInfo.getCreatorId(), eMediaInputVideoJpg, getMediaModule(), m_AssetInfo.getCreatorId(), true );
        ui.m_VidWidget->setVidImageRotation( 0 );
        ui.m_VidWidget->enableFreezeFrame( false );
	}
}

//============================================================================
void InputVideoWidget::hideEvent(QHideEvent* hideEvent)
{
    if( m_GroupieId.isHostedIdValid() && ( false == VxIsAppShuttingDown() ) )
	{
		if( m_IsRecording )
		{
			m_IsRecording = false;
			videoRecord( eAssetActionRecordCancel );
            ui.m_StartStopRecButton->setIcon( eMyIconCamcorderNormal );
            ui.m_CancelRecordButton->setVisible( false );      
		}

		ui.m_VidWidget->setVideoFeedId( m_AssetInfo.getCreatorId(), getMediaModule() );
	}

	InputBaseWidget::hideEvent(hideEvent);
}

//============================================================================
void InputVideoWidget::slotRotateCamButtonClicked( void )
{
    ICamCapture::getICamCapture().rotateCurrentCamCapture();

    // always keep the video on screen at zero rotation. capture system should rotate it
    ui.m_VidWidget->setVidImageRotation( 0 );
}

//============================================================================
void InputVideoWidget::slotBeginRecord( void )
{
	if( m_IsRecording )
	{
		m_IsRecording = false;
        ui.m_VidWidget->enableFreezeFrame( true );
		videoRecord( eAssetActionRecordEnd );
        
        ui.m_StartStopRecButton->setIcon( eMyIconCamcorderNormal );
        ui.m_CancelRecordButton->setVisible( false );
		emit signalInputCompleted();
	}
	else
	{
		m_IsRecording = true;
		if( videoRecord( eAssetActionRecordBegin ) )
		{
			m_MyApp.getEngine().fromGuiWantMediaInput( m_AssetInfo.getCreatorId(), eMediaInputVideoJpg, getMediaModule(), m_AssetInfo.getCreatorId(), true );
            ui.m_VidWidget->enableFreezeFrame( false );
			ui.m_StartStopRecButton->setIcon( eMyIconCamcorderCancel );
			ui.m_CancelRecordButton->setVisible( true );
		}
		else
		{
			GuiHelpers::errorMsgBox( eErrMsgVideoClipFailedToStart, GuiHelpers::getParentPageFrame( this ) );
		}
	}
}

//============================================================================
void InputVideoWidget::slotExitVideoWidget( void )
{
    if( m_IsRecording )
    {
        m_IsRecording = false;
        videoRecord( eAssetActionRecordCancel );
        ui.m_StartStopRecButton->setIcon( eMyIconCamcorderNormal );
        ui.m_CancelRecordButton->setVisible( false );
    }

    if( m_ChatEntryWidget )
    {
        //m_MyApp.getEngine().fromGuiWantMediaInput( m_AssetInfo.getCreatorId(), eMediaInputVideoJpg, m_AssetInfo.getCreatorId(), false );
        this->setVisible( false );
        m_ChatEntryWidget->setEntryMode( eAssetTypeUnknown );
        m_ChatEntryWidget->setVisible( true );
    }
}

//============================================================================
void InputVideoWidget::slotRecordCancelButtonClicked( void )
{
	if( m_IsRecording )
	{
		m_IsRecording = false;
		videoRecord( eAssetActionRecordCancel );
        ui.m_StartStopRecButton->setIcon( eMyIconCamcorderNormal );
	}

	emit signalInputCompleted();
}

//============================================================================
void InputVideoWidget::callbackGuiPlayMotionVideoFrame( VxGUID& feedOnlineId, QImage& vidFrame, int motion0To100000 )
{
	ui.m_VidWidget->callbackGuiPlayMotionVideoFrame( feedOnlineId, vidFrame, motion0To100000 );
}
