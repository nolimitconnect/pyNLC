 //============================================================================
// Copyright (C) 2013 Brett R. Jones
// 
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#ifdef TARGET_OS_WINDOWS

#include "VxVidCap.h"
#include <CoreLib/VxDebug.h>

//============================================================================
//=== constants ===//
//============================================================================

VxVidCap	g_oVxVidCap;

//============================================================================
//! return interface to video capture
IVxVidCap * VxGetVidCapInterface( void )
{
	return &g_oVxVidCap;
}


//============================================================================
VxVidCap::VxVidCap()
	: m_iVidSrcCnt(0)
	, m_iPreferedImageWidth(320)
	, m_iPreferedImageHeight(240) 
	, m_iStartupCnt(0)
{
}

//============================================================================
VxVidCap::~VxVidCap()
{
}

//============================================================================
//! startup return number of video sources ( iVidSrcNum is zero to number of Video Sources - 1 )
int VxVidCap::startupVidCap( int iPreferedImageWidth, int iPreferedImageHeight )
{
	m_iStartupCnt++;
	if( 1 == m_iStartupCnt )
	{
		// only startup on first call
		m_iPreferedImageWidth = iPreferedImageWidth;
		m_iPreferedImageHeight = iPreferedImageHeight;

		int32_t rc;
		VxVidCapImp * poVidCap;
		//! initialize drivers for all video sources
		for( int iIdx = 0; iIdx < MAX_VFW_DEVICES; iIdx++ )
		{
			poVidCap = new VxVidCapImp();
            if( capGetDriverDescriptionA(	iIdx,
											poVidCap->as8DeviceName,
											sizeof( poVidCap->as8DeviceName ),
											poVidCap->as8DeviceVersion,
											sizeof( poVidCap->as8DeviceVersion )) )
			{
				try
				{
					
					rc = poVidCap->Initialize( m_iVidSrcCnt, iIdx, m_iPreferedImageWidth, m_iPreferedImageHeight );
					if( 0 == rc )
					{
						m_paoVidCap[ m_iVidSrcCnt ] = poVidCap;
						m_iVidSrcCnt++;
					}
					else
					{
						delete poVidCap;
						LogMsg( LOG_ERROR, "VxVidCap::startupVidCap: Initialize error %d driver %d\n", rc, iIdx );
					}
				}
				catch(...)
				{
					delete poVidCap;
					LogMsg( LOG_ERROR, "VxVidCap::startupVidCap: exception driver %d\n", iIdx );
				}
			}
			else
			{
				// no more sources
				delete poVidCap;
				break;
			}
		}
	}

	return m_iVidSrcCnt;
}
//============================================================================
//! shutdown	
void VxVidCap::shutdownVidCap( void )
{
	if( m_iStartupCnt > 0 )
	{
		m_iStartupCnt--;
		if( 0 == m_iStartupCnt )
		{
			//! initialize drivers for all video sources
			for( int iIdx = 0; iIdx < m_iVidSrcCnt; iIdx++ )
			{
				m_paoVidCap[ iIdx ]->Destroy();
				delete m_paoVidCap[ iIdx ];
				m_paoVidCap[ iIdx ] = 0;
			}
			m_iVidSrcCnt = 0;
		}
	}
}

//============================================================================
//! start preview 
bool VxVidCap::startPreview(	VIDCAP_HWND	pvHwnd,		// handle of window to preview video on	
								int			iVidSrcNum,	// video source to enable ( 0 = default )
								int			iXPos,		// x pos to draw at
								int			iYPos,		// y pos to draw at
								int			iWidth,		// if zero then max width possible in window
								int			iHeight,	// if zero then max height possible in window
								int			iFps		// preview frame rate
								)
{
	if( iVidSrcNum < m_iVidSrcCnt )
	{
		return m_paoVidCap[ iVidSrcNum ]->EnablePreviewVideo(	pvHwnd,		// handle of window to preview video on	
																iXPos,		// x pos to draw at
																iYPos,		// y pos to draw at
																iWidth,		// if zero then max width possible in window
																iHeight,	// if zero then max height possible in window
																iFps );		// preview frame rate
	}

	LogMsg( LOG_ERROR, "VxVidCap::startPreview: video source out of range\n");
	return false;
}
//============================================================================
//! stop preview 
bool VxVidCap::stopPreview( int iVidSrcNum )
{
	if( iVidSrcNum < m_iVidSrcCnt )
	{
		return m_paoVidCap[ iVidSrcNum ]->DisablePreviewVideo();
	}

	LogMsg( LOG_ERROR, "VxVidCap::startPreview: video source out of range\n");
	return false;
}

//static VxElapseTimer g_oSnapshotTimer;
//static VxElapseTimer g_oIntervalTimer;
//============================================================================
//! take snap shot
// NOTE 1: caller must delete the returned buffer
uint8_t * VxVidCap::takeSnapShot( int iVidSrcNum, uint32_t& u32RetImageLen, uint32_t& u32RetFormat )
{
	BITMAPINFO * poBitmap = NULL;

	if( iVidSrcNum < m_iVidSrcCnt )
	{
		//g_oSnapshotTimer.startTimer();
		if( m_paoVidCap[ iVidSrcNum ]->CaptureDIB( &poBitmap, 0, &u32RetImageLen, &u32RetFormat ) )
		{
			//LogMsg( LOG_DEBUG, "VxVidCap::takeSnapShot: SUCESS time %3.3f sec interval %3.3f\n", g_oSnapshotTimer.elapsedSec(), g_oIntervalTimer.elapsedSec());
			return (uint8_t *)(poBitmap);
		}
		else
		{
			LogMsg( LOG_ERROR, "VxVidCap::takeSnapShot: CaptureDIB FAILED\n");
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "VxVidCap::takeSnapShot: video source out of range\n");
	}

	return NULL;
}

#endif // TARGET_OS_WINDOWS
