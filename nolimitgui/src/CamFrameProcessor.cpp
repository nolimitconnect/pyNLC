//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "CamFrameProcessor.h"

#include "AppCommon.h"
#include "CamLogic.h"
#include "GuiParams.h"

#include <P2PEngine/P2PEngine.h>
#include <MediaProcessor/MediaProcessor.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxElapseTimer.h>

#include <algorithm>
#include <limits>


namespace
{
    // from fourcc.org
    #define FOURCC_RGB		0x32424752

    constexpr int64_t CAM_CORRUPTION_LOG_THROTTLE_MS = 2000;
    constexpr int CAM_EXPECTED_WIDTH = 320;
    constexpr int CAM_EXPECTED_HEIGHT = 240;
    constexpr int CAM_EXPECTED_PIXEL_COUNT = CAM_EXPECTED_WIDTH * CAM_EXPECTED_HEIGHT;
    constexpr int CAM_EXPECTED_RGB_ROW_BYTES = CAM_EXPECTED_WIDTH * 3;
    constexpr uint32_t CAM_EXPECTED_RGB_DATA_LEN = CAM_EXPECTED_RGB_ROW_BYTES * CAM_EXPECTED_HEIGHT;
    constexpr int CAM_FIRST_FRAME_DUMP_BYTES = 64;

    void logCamFrameCorruption( int logLevel, int64_t timeNow, const char* fmt, ... )
    {
        static int64_t lastCorruptionLogMs = 0;
        if( timeNow < lastCorruptionLogMs + CAM_CORRUPTION_LOG_THROTTLE_MS )
        {
            return;
        }

        lastCorruptionLogMs = timeNow;

        char logBuffer[1024]{};
        va_list args;
        va_start( args, fmt );
        vsnprintf( logBuffer, sizeof( logBuffer ), fmt, args );
        va_end( args );

        LogMsg( logLevel, "%s", logBuffer );
    }

    void logCamFrameBytes( const char* label, const uint8_t* data, int dataLen )
    {
        if( !data || dataLen <= 0 )
        {
            LogMsg( LOG_ERROR, "%s no data to dump len=%d", label, dataLen );
            return;
        }

        int dumpLen = std::min( dataLen, CAM_FIRST_FRAME_DUMP_BYTES );
        char logBuffer[1024]{};
        int offset = snprintf( logBuffer, sizeof( logBuffer ), "%s len=%d bytes:", label, dataLen );
        for( int i = 0; i < dumpLen && offset > -1 && offset < (int)sizeof( logBuffer ); ++i )
        {
            offset += snprintf( logBuffer + offset, sizeof( logBuffer ) - offset, " %02x", data[i] );
        }

        LogMsg( LOG_INFO, "%s", logBuffer );
    }
};

//============================================================================
CamFrameProcessor::CamFrameProcessor( AppCommon& myApp, CamLogic& camLogic, QObject* parent )
    : QObject( parent )
    , m_MyApp( myApp )
    , m_CamLogic( camLogic )
{
}

//============================================================================
CamFrameProcessor::~CamFrameProcessor()
{
    enableProcessing( false );
}

//============================================================================
void CamFrameProcessor::enableProcessing( bool enable )
{
    if( m_ProcessFramesEnabled != enable )
    {
        m_ProcessFramesEnabled = enable;
        if( m_ProcessFramesEnabled )
        {
            m_DesiredFrameSize = GuiParams::getSnapshotDesiredSize();
        }
    }
}

//============================================================================
void CamFrameProcessor::slotVideoFrameChanged( const QVideoFrame& frame )
{
    static int64_t lastTimeMs = 0;
    int64_t timeNow = GetHighResolutionTimeMs();
    if( timeNow < lastTimeMs + CamLogic::CAM_SNAPSHOT_INTERVAL_MS )
    {
        //LogMsg( LOG_VERBOSE, "%s time ok %d ms", __func__, (int)(timeNow - lastTimeMs) );
        return;
    }
    
    lastTimeMs = timeNow;

    if( !m_ProcessFramesEnabled || !m_CamLogic.canProcessCamCapture() )
    {
        //if( !m_ProcessFramesEnabled )
        //{
        //    LogMsg( LOG_ERROR, "%s !m_ProcessFramesEnabled %d", __func__, GetHighResolutionTimeMs() );
        //}
        //else
        //{
        //    LogMsg( LOG_ERROR, "%s !m_CamLogic.canProcessCamCapture %d", __func__, GetApplicationAliveMs() );
        //}

        return;
    }

    if( !frame.isValid() )
    {
        logCamFrameCorruption( LOG_WARN, timeNow, "%s invalid frame", __func__ );
        return;
    }

    if( frame.width() <= 0 || frame.height() <= 0 )
    {
        logCamFrameCorruption( LOG_ERROR, timeNow, "%s invalid frame dimensions w=%d h=%d fmt=%d",
            __func__, frame.width(), frame.height(), (int)frame.surfaceFormat().pixelFormat() );
        return;
    }

    // IMPORTANT: Explicitly map the frame to ensure the buffer is accessible to the CPU
    QVideoFrame cloneFrame(frame);
    if( !cloneFrame.map( QVideoFrame::ReadOnly ) )
    {
        logCamFrameCorruption( LOG_ERROR, timeNow, "%s failed to map frame w=%d h=%d fmt=%d",
            __func__, frame.width(), frame.height(), (int)frame.surfaceFormat().pixelFormat() );
        return;
    }

    int planeCount = cloneFrame.planeCount();
    if( planeCount < 1 )
    {
        logCamFrameCorruption( LOG_ERROR, timeNow, "%s mapped frame has no planes w=%d h=%d fmt=%d",
            __func__, cloneFrame.width(), cloneFrame.height(), (int)cloneFrame.surfaceFormat().pixelFormat() );
        cloneFrame.unmap();
        return;
    }

    int frameStride = cloneFrame.bytesPerLine( 0 );
    int mappedBytes = cloneFrame.mappedBytes( 0 );
    const uint8_t* frameBits = cloneFrame.bits( 0 );
    if( !frameBits || frameStride <= 0 || mappedBytes < frameStride )
    {
        logCamFrameCorruption( LOG_ERROR, timeNow,
            "%s mapped frame corruption bits=%p planes=%d stride=%d mappedBytes=%d w=%d h=%d fmt=%d",
            __func__, frameBits, planeCount, frameStride, mappedBytes, cloneFrame.width(), cloneFrame.height(),
            (int)cloneFrame.surfaceFormat().pixelFormat() );
        cloneFrame.unmap();
        return;
    }

    if( frameStride < cloneFrame.width() )
    {
        logCamFrameCorruption( LOG_WARN, timeNow,
            "%s suspicious mapped stride stride=%d width=%d mappedBytes=%d planes=%d fmt=%d",
            __func__, frameStride, cloneFrame.width(), mappedBytes, planeCount,
            (int)cloneFrame.surfaceFormat().pixelFormat() );
    }

    if( cloneFrame.width() != CAM_EXPECTED_WIDTH || cloneFrame.height() != CAM_EXPECTED_HEIGHT )
    {
        logCamFrameCorruption( LOG_WARN, timeNow,
            "%s unexpected mapped dimensions w=%d h=%d expected=%dx%d fmt=%d",
            __func__, cloneFrame.width(), cloneFrame.height(), CAM_EXPECTED_WIDTH, CAM_EXPECTED_HEIGHT,
            (int)cloneFrame.surfaceFormat().pixelFormat() );
    }

    if( mappedBytes < CAM_EXPECTED_PIXEL_COUNT )
    {
        logCamFrameCorruption( LOG_ERROR, timeNow,
            "%s mapped frame too small mappedBytes=%d expectedAtLeast=%d stride=%d w=%d h=%d fmt=%d",
            __func__, mappedBytes, CAM_EXPECTED_PIXEL_COUNT, frameStride, cloneFrame.width(), cloneFrame.height(),
            (int)cloneFrame.surfaceFormat().pixelFormat() );
        cloneFrame.unmap();
        return;
    }

    QImage rgbImage = cloneFrame.toImage().convertToFormat( QImage::Format_RGB888 );
    // Always unmap after conversion
    cloneFrame.unmap();

    if( !rgbImage.isNull() )
    {
        if( rgbImage.width() <= 0 || rgbImage.height() <= 0 )
        {
            logCamFrameCorruption( LOG_ERROR, timeNow, "%s invalid rgb image dimensions w=%d h=%d",
                __func__, rgbImage.width(), rgbImage.height() );
            return;
        }

        if( rgbImage.width() > std::numeric_limits<int>::max() / 3 )
        {
            logCamFrameCorruption( LOG_ERROR, timeNow, "%s row byte overflow width=%d", __func__, rgbImage.width() );
            return;
        }

        int rowBytes = 3 * rgbImage.width();
        int imageStride = rgbImage.bytesPerLine();
        int rgbAvailableBytes = imageStride * rgbImage.height();
        if( imageStride < rowBytes )
        {
            logCamFrameCorruption( LOG_ERROR, timeNow,
                "%s rgb stride corruption stride=%d expected=%d w=%d h=%d",
                __func__, imageStride, rowBytes, rgbImage.width(), rgbImage.height() );
            return;
        }

        if( imageStride != rowBytes )
        {
            logCamFrameCorruption( LOG_WARN, timeNow,
                "%s rgb stride padded stride=%d expected=%d w=%d h=%d",
                __func__, imageStride, rowBytes, rgbImage.width(), rgbImage.height() );
        }

        if( rgbImage.width() != CAM_EXPECTED_WIDTH || rgbImage.height() != CAM_EXPECTED_HEIGHT )
        {
            logCamFrameCorruption( LOG_WARN, timeNow,
                "%s unexpected rgb dimensions w=%d h=%d expected=%dx%d",
                __func__, rgbImage.width(), rgbImage.height(), CAM_EXPECTED_WIDTH, CAM_EXPECTED_HEIGHT );
        }

        if( rgbAvailableBytes < (int)CAM_EXPECTED_RGB_DATA_LEN )
        {
            logCamFrameCorruption( LOG_ERROR, timeNow,
                "%s rgb image too small available=%d expectedAtLeast=%u stride=%d w=%d h=%d",
                __func__, rgbAvailableBytes, CAM_EXPECTED_RGB_DATA_LEN, imageStride, rgbImage.width(), rgbImage.height() );
            return;
        }

        if( rgbImage.height() > 0 && rowBytes > (int)(std::numeric_limits<uint32_t>::max() / (uint32_t)rgbImage.height()) )
        {
            logCamFrameCorruption( LOG_ERROR, timeNow,
                "%s data length overflow rowBytes=%d h=%d", __func__, rowBytes, rgbImage.height() );
            return;
        }

        uint32_t dataLen = (uint32_t)(rowBytes * rgbImage.height());

        std::shared_ptr<uint8_t> rgbData( new uint8_t[dataLen] );
        if( imageStride == rowBytes )
        {
            memcpy( rgbData.get(), rgbImage.bits(), dataLen );
        }
        else
        {
            uint8_t* dest = rgbData.get();
            for( int row = 0; row < rgbImage.height(); ++row )
            {
                const uint8_t* scanLine = rgbImage.constScanLine( row );
                if( !scanLine )
                {
                    logCamFrameCorruption( LOG_ERROR, timeNow,
                        "%s null rgb scanline row=%d stride=%d w=%d h=%d",
                        __func__, row, imageStride, rgbImage.width(), rgbImage.height() );
                    return;
                }

                memcpy( dest, scanLine, rowBytes );
                dest += rowBytes;
            }
        }

        // LogMsg( LOG_VERBOSE, "%s getCamProcessor().processCamCapture %d", __func__, GetApplicationAliveMs() );
        m_CamLogic.getCamProcessor().processCamCapture( rgbImage.width(), rgbImage.height(), rgbData, dataLen );
    }
    else
    {
        logCamFrameCorruption( LOG_ERROR, timeNow,
            "%s rgb conversion failed w=%d h=%d fmt=%d stride=%d mappedBytes=%d",
            __func__, cloneFrame.width(), cloneFrame.height(), (int)cloneFrame.surfaceFormat().pixelFormat(),
            frameStride, mappedBytes );
    }
}
