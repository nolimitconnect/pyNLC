//============================================================================
// Copyright (C) 2009 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppCommon.h"

#include <GuiInterface/ICamCapture.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include <QMessageBox>

//============================================================================
QString AppCommon::getCameraBackgroundFile( void )
{
    if( ICamCapture::getICamCapture().getCamCaptureEnable() )
    {
        return ":/AppRes/Resources/ic_cam_black.png";
    }
    else
    {
        return ":/AppRes/Resources/ic_cam_disabled.png";
    }
}

//============================================================================
void AppCommon::toGuiCamCaptureEnable( bool camCaptureEnabled )
{
	LogModule( eLogWebCam, LOG_INFO, "AppCommon::%s %d", __func__, camCaptureEnabled );
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalCamCaptureEnable( camCaptureEnabled );
}

//============================================================================
void AppCommon::slotInternalCamCaptureEnable( bool camCaptureEnabled )          
{
	LogModule( eLogWebCam, LOG_INFO, "AppCommon::%s %d", __func__, camCaptureEnabled );
	if( m_ToGuiHardwareCtrlBusy )
	{
		LogMsg( LOG_WARN, "AppCommon::%s ToGuiHardware busy; skipping nested callback", __func__ );
		scheduleHardwareCtrlStateReplay();
		return;
	}

	m_ToGuiHardwareCtrlBusy = true;
	for( auto& toGuiClient : m_ToGuiHardwareCtrlList )
	{
		toGuiClient->callbackToGuiCameraEnable( camCaptureEnabled );
	}

	m_ToGuiHardwareCtrlBusy = false;
}

//============================================================================
void AppCommon::toGuiWantCamCapture( EMediaModule mediaModule, bool wantVidCapture )
{
	LogModule( eLogWebCam, LOG_INFO, "#### AppCommon::toGuiWantCamCapture %s wantCapture %d", DescribeMediaModule( mediaModule ), wantVidCapture );
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalWantCamCapture( mediaModule, wantVidCapture );
}

//============================================================================
void AppCommon::slotInternalWantCamCapture( EMediaModule mediaModule, bool wantVidCapture )
{
	bool wasCamEnabled = ICamCapture::getICamCapture().isCamCaptureRunning();
	ICamCapture::getICamCapture().wantCamCapture( mediaModule, wantVidCapture );
	bool isCamEnabled = ICamCapture::getICamCapture().isCamCaptureRunning();

    if( wasCamEnabled != isCamEnabled )
    {
        if( isCamEnabled )
        {
            static bool bFirstTimeVideoCaptureStarted = true;
            if( bFirstTimeVideoCaptureStarted )
            {
                if( !ICamCapture::getICamCapture().isCamCaptureRunning() )
                {
                    QMessageBox::warning( this, QObject::tr( "Web Cam Video" ), QObject::tr( "No Video Capture Devices Found" ) );
                    return;
                }

//                m_CamSourceId = ICamCapture::getICamCapture().getCamId();

//                setCamCaptureRotation( IGlobalDb::getIGlobalDb().getCamRotation( m_CamSourceId ) );

                bFirstTimeVideoCaptureStarted = false;
            }
        }
        else
        {
           LogModule( eLogWebCam, LOG_INFO, "AppCommon::slotEnableVideoCapture stopping capture" );
        }

		m_ToGuiHardwareCtrlBusy = true;
		for( auto toGuiClient : m_ToGuiHardwareCtrlList )
		{
			toGuiClient->callbackToGuiWantVideoCapture( wantVidCapture );
		}

		m_ToGuiHardwareCtrlBusy = false;
    }
}

//============================================================================
void AppCommon::toGuiCamCaptureRunning( bool camCaptureRunning )
{
	LogModule( eLogWebCam, LOG_INFO, "AppCommon::%s %d", __func__, camCaptureRunning );
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalCamCaptureRunning( camCaptureRunning );
}

//============================================================================
void AppCommon::slotInternalCamCaptureRunning( bool camCaptureRunning )
{
	if( m_ToGuiHardwareCtrlBusy )
	{
		LogMsg( LOG_WARN, "AppCommon::%s ToGuiHardware busy; skipping nested callback", __func__ );
		scheduleHardwareCtrlStateReplay();
		return;
	}

	m_ToGuiHardwareCtrlBusy = true;
	for( auto& toGuiClient : m_ToGuiHardwareCtrlList )
	{
		toGuiClient->callbackToGuiCaptureRunning( camCaptureRunning );
	}

	m_ToGuiHardwareCtrlBusy = false;
}
