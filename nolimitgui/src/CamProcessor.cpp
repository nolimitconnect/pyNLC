//============================================================================
// Copyright (C) 2025 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "CamProcessor.h"

#include "CamLogic.h"
#include "CamRgbVideo.h"

#if defined(USE_LIBJPEG_TURBO)
#include <libjpeg-turbo/VxJpgLib.h>
#else
#include <libjpg/VxJpgLib.h>
#endif // defined(USE_LIBJPEG_TURBO)

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxElapseTimer.h>

#include <MediaProcessor/CamJpgVideo.h>

#include <VxVideoLib/VxRescaleRgb.h>

#include <stdlib.h>

namespace
{
	const int VIDEO_MAX_MOTION_VALUE = 100000;
};

//============================================================================
CamProcessor::CamProcessor( CamLogic& camLogic )
: m_CamLogic( camLogic )
{
    m_ProcessCamRgbThread = std::thread( [this]() { processCamRgbThreaded(); } );
}

//============================================================================
CamProcessor::~CamProcessor()
{
    shutdownCamProcessor();
}

//============================================================================
void CamProcessor::shutdownCamProcessor( void )
{
    m_Abort.store( true );
    m_CamRgbCondVar.notify_all();
    if( m_ProcessCamRgbThread.joinable() )
    {
        m_ProcessCamRgbThread.join();
    }
}

//============================================================================

//============================================================================
void CamProcessor::processCamCapture( int width, int height, std::shared_ptr<uint8_t>& rgbData, int dataLen )
{
    CamRgbVideo * rawVideo = new CamRgbVideo( rgbData, dataLen, width, height );

    {
        std::lock_guard<std::mutex> lock( m_CamRgbMutex );
        m_ProcessCamRgbQue.push( rawVideo );
    }
    m_CamRgbCondVar.notify_one();
}

//============================================================================
void CamProcessor::processCamRgbThreaded( void )
{
    while( !m_Abort.load() )
    {
        CamRgbVideo* rawVideo = nullptr;
        {
            std::unique_lock<std::mutex> lock( m_CamRgbMutex );
            m_CamRgbCondVar.wait( lock, [this](){ return m_Abort.load() || !m_ProcessCamRgbQue.empty(); } );
            if( m_Abort.load() )
                break;
            rawVideo = m_ProcessCamRgbQue.front();
            m_ProcessCamRgbQue.pop();
        }

        if( rawVideo )
        {
            int64_t startTime;
            if( LogEnabled( eLogWebCam ) )
            {
                startTime = GetGmtTimeMs();
            }

            processCamVideoRgb( rawVideo );
            delete rawVideo;

            // if( LogEnabled( eLogWebCam ) )
            // {
            //     static int frameCnt = 0;
            //     frameCnt++;
            //     if( frameCnt >= 100 )
            //     {
            //         frameCnt = 0;
            //         int64_t endTime = GetGmtTimeMs();
            //         LogMsg( LOG_VERBOSE, "CamProcessor::%s took %d ms at %d", __func__,
            //                (int)(endTime - startTime), GetApplicationAliveMs() );
            //     }
            // }
        }
    }

    LogMsg( LOG_INFO, "CamProcessor::%s leaving function", __func__ );
}

//============================================================================
int CamProcessor::calculateImageMotion( std::shared_ptr<uint8_t> rgbData, int dataLen )
{
    if( dataLen != m_LastRgbDataLen || !m_LastRgbData )
    {
        m_LastRgbData = rgbData;
        m_LastRgbDataLen = dataLen;
        return 0;
    }

    constexpr int MOTION_STEP = 12; // sample every 4th pixel (3 bytes/pixel * 4)
    std::shared_ptr<uint8_t> lastMotion = m_LastRgbData;
    const uint8_t* motionBuf = lastMotion.get();
    const uint8_t* cmpData = rgbData.get();
    unsigned int vidDiff = 0;
    for( int i = 0; i < dataLen; i += MOTION_STEP )
    {
        vidDiff += cmpData[i] > motionBuf[i] ? ( cmpData[i] - motionBuf[i] ) : ( motionBuf[i] - cmpData[i] );
    }

    double videoSensitivity = ( (double)dataLen / MOTION_STEP ) * 64.0;
    double difNormalized = ((double)vidDiff * VIDEO_MAX_MOTION_VALUE) / videoSensitivity;
    int motion0To100000 = difNormalized > VIDEO_MAX_MOTION_VALUE ? VIDEO_MAX_MOTION_VALUE : (int)difNormalized;
    m_LastRgbData = rgbData;
    m_LastRgbDataLen = dataLen;
    return motion0To100000;
}

//============================================================================
void CamProcessor::processCamVideoRgb( CamRgbVideo* rgbVideo )
{
    //LogMsg( LOG_WARN, "CamProcessor::%s start %d", __func__, GetApplicationAliveMs() );
    int motion = calculateImageMotion( rgbVideo->m_VidData, rgbVideo->m_VidDataLen );

    uint8_t* pu8VidData	= rgbVideo->m_VidData.get();
    int width			= rgbVideo->m_Width;
    int height			= rgbVideo->m_Height;
    int rotation		= m_CamLogic.getCamCaptureRotation();

	bool needToDeleteVidData{ false };
    bool bResize = (( 320 != width ) || ( 240 != height ));
	if( bResize )
	{
		// scale to 320x200.. the reason we don't use the GdvBufferUtil to scale at same time as convert is that
		// it looks like crap. here we use averaging
		pu8VidData = VxResizeRgbImage(	pu8VidData, 
                                        width,
                                        height,
			                            320, 
			                            240,
                                        rotation );

		needToDeleteVidData = true;
        width = 320;
        height = 240;
	}
    else if( 0 != rotation )
	{
		// need to rotate
		pu8VidData = VxResizeRgbImage(	pu8VidData, 
                                        width,
                                        height,
			                            320, 
			                            240,
                                        rotation );
		needToDeleteVidData = true;
	}

	// compress from rgb to jpg for sending
	long s32JpgDataLen = 0;
	// take a guess at what size the jpg will be
    int iMaxJpgSize = width * height * 3;
    // seems like a waste of memory but we have no way of knowing jpg size before converting and memcpy after creation is cpu drain
    std::shared_ptr<uint8_t> jpgData( new uint8_t[iMaxJpgSize] );

    int32_t rc = VxBmp2Jpg(	 24,							// number of bits each pixel..(For now must be 24)
						     pu8VidData,					// bits of bmp to convert
                             width,						// width of image in pixels
                             height,						// height of image in pixels
                             JPG_CONVERT_QUALITY,			// quality of image
						     iMaxJpgSize,					// maximum length of pu8RetJpg
                             jpgData.get(),                 // buffer to return Jpeg image
						     &s32JpgDataLen );				// return length of jpeg image
	#if defined( LOG_JPG_SIZE )
        LogMsg( LOG_VERBOSE, "VxBmp2Jpg processCamVideoIn size %d", s32JpgDataLen );
	#endif //defined( LOG_JPG_SIZE )
    if( needToDeleteVidData )
    {
        delete[] pu8VidData;
    }

	if( 0 == rc )
	{
    std::shared_ptr<CamJpgVideo> jpgVideo( new CamJpgVideo( jpgData, s32JpgDataLen, motion, 0, eMediaModuleCamClient ) );
        m_CamLogic.processCamCapture( jpgVideo );
	}
	else
	{
        LogMsg( LOG_ERROR, "CamProcessor::%s JPEG Conversion error %d", __func__, rc );
	}
}


//============================================================================
