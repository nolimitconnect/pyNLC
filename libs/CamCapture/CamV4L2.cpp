//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#if defined(TARGET_OS_LINUX)

#include "CamV4L2.h"
#include "CamCapture.h"
#include "CamProcessor.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxElapseTimer.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <fcntl.h>
#include <libjpeg-turbo/src/turbojpeg.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#if !defined(_IOR) || !defined(_IOW) || !defined(_IOWR)
#include <asm-generic/ioctl.h>
#endif
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>

namespace
{
    constexpr int64_t FRAME_THROTTLE_MS = 60; // ~15 fps, matches CamFrameProcessor
    constexpr int     MMAP_BUFFER_COUNT = 4;
    constexpr int     SELECT_TIMEOUT_US = 100'000; // 100 ms — lets us check m_Abort
    constexpr int     MAX_VIDEO_NODES   = 16;

    uint32_t getEffectiveCaps( const v4l2_capability& cap )
    {
        if( cap.capabilities & V4L2_CAP_DEVICE_CAPS )
        {
            return cap.device_caps;
        }

        return cap.capabilities;
    }

    void formatToFourcc( uint32_t pixelFormat, char (&fourcc)[5] )
    {
        fourcc[0] = (char)( pixelFormat & 0xFF );
        fourcc[1] = (char)( ( pixelFormat >> 8 ) & 0xFF );
        fourcc[2] = (char)( ( pixelFormat >> 16 ) & 0xFF );
        fourcc[3] = (char)( ( pixelFormat >> 24 ) & 0xFF );
        fourcc[4] = 0;
    }

    void scaleRgbNearest( const uint8_t* src, int srcW, int srcH,
                          uint8_t* dst, int dstW, int dstH )
    {
        if( !src || !dst || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0 )
        {
            return;
        }

        const int srcStride = srcW * 3;
        const int dstStride = dstW * 3;
        for( int y = 0; y < dstH; ++y )
        {
            int srcY = (int)( (int64_t)y * srcH / dstH );
            if( srcY >= srcH ) srcY = srcH - 1;

            const uint8_t* srcRow = src + (size_t)srcY * (size_t)srcStride;
            uint8_t* dstRow = dst + (size_t)y * (size_t)dstStride;
            for( int x = 0; x < dstW; ++x )
            {
                int srcX = (int)( (int64_t)x * srcW / dstW );
                if( srcX >= srcW ) srcX = srcW - 1;

                const uint8_t* srcPx = srcRow + (size_t)srcX * 3U;
                uint8_t* dstPx = dstRow + (size_t)x * 3U;
                dstPx[0] = srcPx[0];
                dstPx[1] = srcPx[1];
                dstPx[2] = srcPx[2];
            }
        }
    }

    static const uint8_t kDefaultJpegDht[] = {
        0xFF,0xC4,0x01,0xA2,
        0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
        0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,
        0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
        0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
        0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
        0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
        0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
        0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
        0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
        0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
        0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
        0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
        0xF9,0xFA,
        0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
        0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,
        0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,
        0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,
        0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,
        0x26,0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,
        0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,
        0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,
        0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,
        0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,
        0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,
        0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
        0xF8,0xF9,0xFA
    };

    bool insertDefaultDhtIfMissing( const uint8_t* jpegData,
                                    unsigned long jpegSize,
                                    std::vector<uint8_t>& patchedJpeg )
    {
        patchedJpeg.clear();
        if( !jpegData || jpegSize < 4 )
        {
            return false;
        }

        if( jpegData[0] != 0xFF || jpegData[1] != 0xD8 )
        {
            return false;
        }

        bool hasDht = false;
        size_t pos = 2;
        while( pos + 3 < jpegSize )
        {
            while( pos < jpegSize && jpegData[pos] != 0xFF )
            {
                ++pos;
            }
            if( pos + 1 >= jpegSize )
            {
                break;
            }

            size_t markerPos = pos;
            while( pos + 1 < jpegSize && jpegData[pos + 1] == 0xFF )
            {
                ++pos;
            }
            if( pos + 1 >= jpegSize )
            {
                break;
            }

            uint8_t marker = jpegData[pos + 1];
            pos += 2;

            if( marker == 0xD9 )
            {
                break;
            }

            if( marker == 0xDA )
            {
                if( hasDht )
                {
                    return false;
                }

                patchedJpeg.assign( jpegData, jpegData + markerPos );
                patchedJpeg.insert( patchedJpeg.end(),
                                    kDefaultJpegDht,
                                    kDefaultJpegDht + sizeof( kDefaultJpegDht ) );
                patchedJpeg.insert( patchedJpeg.end(), jpegData + markerPos, jpegData + jpegSize );
                return true;
            }

            if( marker == 0x01 || ( marker >= 0xD0 && marker <= 0xD7 ) )
            {
                continue;
            }

            if( pos + 1 >= jpegSize )
            {
                return false;
            }

            uint16_t segLen = (uint16_t)( ( jpegData[pos] << 8 ) | jpegData[pos + 1] );
            if( segLen < 2 || pos + segLen > jpegSize )
            {
                return false;
            }

            if( marker == 0xC4 )
            {
                hasDht = true;
            }

            pos += segLen;
        }

        return false;
    }

    bool sanitizeMjpegPayload( const uint8_t* payload,
                               unsigned long payloadSize,
                               std::vector<uint8_t>& sanitized )
    {
        sanitized.clear();
        if( !payload || payloadSize < 4 )
        {
            return false;
        }

        size_t soi = SIZE_MAX;
        size_t eoi = SIZE_MAX;

        for( size_t i = 0; i + 1 < payloadSize; ++i )
        {
            if( payload[i] == 0xFF && payload[i + 1] == 0xD8 )
            {
                soi = i;
                break;
            }
        }

        if( soi == SIZE_MAX )
        {
            return false;
        }

        for( size_t i = soi + 2; i + 1 < payloadSize; ++i )
        {
            if( payload[i] == 0xFF && payload[i + 1] == 0xD9 )
            {
                eoi = i + 2;
            }
        }

        if( eoi != SIZE_MAX )
        {
            sanitized.assign( payload + soi, payload + eoi );
            return true;
        }

        // Truncated JPEG frame: keep bytes from SOI and append EOI marker.
        sanitized.assign( payload + soi, payload + payloadSize );
        sanitized.push_back( 0xFF );
        sanitized.push_back( 0xD9 );
        return true;
    }

    bool mjpegToRgb( tjhandle jpegDecoder,
                     const uint8_t* mjpegData,
                     unsigned long mjpegSize,
                     uint8_t* rgbData,
                     int expectedWidth,
                     int expectedHeight )
    {
        if( !jpegDecoder || !mjpegData || !mjpegSize || !rgbData )
        {
            return false;
        }

        std::vector<uint8_t> sanitizedJpeg;
        if( !sanitizeMjpegPayload( mjpegData, mjpegSize, sanitizedJpeg ) )
        {
            return false;
        }

        auto decodeFrame = [&]( const uint8_t* jpegBytes, unsigned long jpegLen ) -> bool {
            int width = 0;
            int height = 0;
            int jpegSubsamp = 0;
            int jpegColorspace = 0;
            if( tjDecompressHeader3( jpegDecoder, jpegBytes, jpegLen,
                                     &width, &height, &jpegSubsamp, &jpegColorspace ) != 0 )
            {
                return false;
            }

            if( width <= 0 || height <= 0 )
            {
                return false;
            }

            if( width == expectedWidth && height == expectedHeight )
            {
                return tjDecompress2( jpegDecoder, jpegBytes, jpegLen,
                                      rgbData, width, 0, height, TJPF_RGB,
                                      TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE ) == 0;
            }

            std::vector<uint8_t> decodedRgb( (size_t)width * (size_t)height * 3U );
            if( tjDecompress2( jpegDecoder, jpegBytes, jpegLen,
                               decodedRgb.data(), width, 0, height, TJPF_RGB,
                               TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE ) != 0 )
            {
                return false;
            }

            scaleRgbNearest( decodedRgb.data(), width, height, rgbData, expectedWidth, expectedHeight );
            return true;
        };

        if( decodeFrame( sanitizedJpeg.data(), (unsigned long)sanitizedJpeg.size() ) )
        {
            return true;
        }

        std::vector<uint8_t> patchedJpeg;
        if( insertDefaultDhtIfMissing( sanitizedJpeg.data(), (unsigned long)sanitizedJpeg.size(), patchedJpeg ) )
        {
            return decodeFrame( patchedJpeg.data(), (unsigned long)patchedJpeg.size() );
        }

        return false;
    }
}

//============================================================================
CamV4L2::CamV4L2( CamCapture& camLogic )
    : m_CamCapture( camLogic )
{
}

//============================================================================
CamV4L2::~CamV4L2()
{
    closeDevice();
}

//============================================================================
void CamV4L2::enumerateDevices( std::vector<std::pair<std::string, std::string>>& devices )
{
    struct CamNodeCandidate
    {
        std::string devPath;
        bool supportsYuyv{ false };
        bool supportsMjpg{ false };
        int  nodeIndex{ 0 };
    };

    auto scoreCandidate = []( const CamNodeCandidate& c ) -> int {
        // Prefer a node that can output YUYV, then MJPG, then lower /dev/video index.
        int score = 0;
        if( c.supportsYuyv ) score += 100;
        if( c.supportsMjpg ) score += 10;
        score -= c.nodeIndex;
        return score;
    };

    devices.clear();
    std::map<std::string, CamNodeCandidate> bestNodeByCard;

    for( int i = 0; i < MAX_VIDEO_NODES; ++i )
    {
        std::string devPath = "/dev/video" + std::to_string( i );
        int fd = ::open( devPath.c_str(), O_RDWR | O_NONBLOCK );
        if( fd < 0 )
            continue;

        struct v4l2_capability cap{};
        if( ::ioctl( fd, VIDIOC_QUERYCAP, &cap ) == 0 )
        {
            uint32_t effectiveCaps = getEffectiveCaps( cap );
            if( ( effectiveCaps & V4L2_CAP_VIDEO_CAPTURE ) && ( effectiveCaps & V4L2_CAP_STREAMING ) )
            {
                std::string cardName = reinterpret_cast<const char*>( cap.card );

                bool supportsYuyv = false;
                bool supportsMjpg = false;
                struct v4l2_fmtdesc desc{};
                desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                for( desc.index = 0; ::ioctl( fd, VIDIOC_ENUM_FMT, &desc ) == 0; ++desc.index )
                {
                    if( desc.pixelformat == V4L2_PIX_FMT_YUYV )
                    {
                        supportsYuyv = true;
                    }
                    else if( desc.pixelformat == V4L2_PIX_FMT_MJPEG )
                    {
                        supportsMjpg = true;
                    }
                }

                if( !supportsYuyv && !supportsMjpg )
                {
                    continue;
                }

                CamNodeCandidate candidate;
                candidate.devPath = devPath;
                candidate.supportsYuyv = supportsYuyv;
                candidate.supportsMjpg = supportsMjpg;
                candidate.nodeIndex = i;

                auto found = bestNodeByCard.find( cardName );
                if( found == bestNodeByCard.end() || scoreCandidate( candidate ) > scoreCandidate( found->second ) )
                {
                    bestNodeByCard[cardName] = candidate;
                }
            }
        }
        ::close( fd );
    }

    for( const auto& item : bestNodeByCard )
    {
        devices.emplace_back( item.first, item.second.devPath );
        if( LogEnabled( eLogWebCam ) )
        {
            LogModule( eLogWebCam, LOG_VERBOSE,
                "CamV4L2::enumerateDevices selected '%s' -> %s (YUYV=%d MJPG=%d)",
                item.first.c_str(), item.second.devPath.c_str(), item.second.supportsYuyv ? 1 : 0, item.second.supportsMjpg ? 1 : 0 );
        }
    }

    LogMsg( LOG_INFO, "CamV4L2::enumerateDevices: %zu camera device(s) selected", devices.size() );
}

//============================================================================
bool CamV4L2::openDevice( const std::string& devPath, int desiredWidth, int desiredHeight )
{
    closeDevice(); // ensure clean state
    m_Abort.store( false );

    m_Fd = ::open( devPath.c_str(), O_RDWR | O_NONBLOCK );
    if( m_Fd < 0 )
    {
        LogMsg( LOG_ERROR, "CamV4L2::openDevice: cannot open %s: %s", devPath.c_str(), strerror( errno ) );
        return false;
    }

    // verify it is a streaming capture device
    struct v4l2_capability cap{};
    if( ::ioctl( m_Fd, VIDIOC_QUERYCAP, &cap ) < 0 )
    {
        LogMsg( LOG_ERROR, "CamV4L2::openDevice: VIDIOC_QUERYCAP failed on %s: %s", devPath.c_str(), strerror( errno ) );
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    uint32_t effectiveCaps = getEffectiveCaps( cap );
    if( !( effectiveCaps & V4L2_CAP_VIDEO_CAPTURE ) )
    {
        LogMsg( LOG_ERROR, "CamV4L2::openDevice: %s is not a video capture device", devPath.c_str() );
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    if( !( effectiveCaps & V4L2_CAP_STREAMING ) )
    {
        LogMsg( LOG_ERROR, "CamV4L2::openDevice: %s does not support streaming", devPath.c_str() );
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    // Prefer YUYV and fall back to MJPG when necessary.
    uint32_t requestedPixelFormat = 0;
    if( supportsFormat( V4L2_PIX_FMT_YUYV ) )
    {
        requestedPixelFormat = V4L2_PIX_FMT_YUYV;
    }
    else if( supportsFormat( V4L2_PIX_FMT_MJPEG ) )
    {
        requestedPixelFormat = V4L2_PIX_FMT_MJPEG;
    }
    else
    {
        LogMsg( LOG_ERROR, "CamV4L2::openDevice: %s supports neither YUYV nor MJPG", devPath.c_str() );
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    // negotiate preferred format at desired size; driver may round up/down
    struct v4l2_format fmt{};
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = (unsigned)desiredWidth;
    fmt.fmt.pix.height      = (unsigned)desiredHeight;
    fmt.fmt.pix.pixelformat = requestedPixelFormat;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    if( ::ioctl( m_Fd, VIDIOC_S_FMT, &fmt ) < 0 )
    {
        // some drivers need V4L2_FIELD_ANY
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
        if( ::ioctl( m_Fd, VIDIOC_S_FMT, &fmt ) < 0 )
        {
            LogMsg( LOG_ERROR, "CamV4L2::openDevice: VIDIOC_S_FMT failed on %s: %s", devPath.c_str(), strerror( errno ) );
            ::close( m_Fd );
            m_Fd = -1;
            return false;
        }
    }

    if( fmt.fmt.pix.pixelformat != requestedPixelFormat )
    {
        char fourcc[5]{};
        formatToFourcc( fmt.fmt.pix.pixelformat, fourcc );
        LogMsg( LOG_ERROR, "CamV4L2::openDevice: %s did not accept requested format; driver offered '%s'", devPath.c_str(), fourcc );
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    m_Width  = (int)fmt.fmt.pix.width;
    m_Height = (int)fmt.fmt.pix.height;
    m_PixelFormat = fmt.fmt.pix.pixelformat;
    m_BytesPerLine = fmt.fmt.pix.bytesperline;
    m_MinFrameBytes = fmt.fmt.pix.sizeimage;

    if( m_PixelFormat == V4L2_PIX_FMT_YUYV )
    {
        uint32_t packedBpl = (uint32_t)m_Width * 2U;
        if( m_BytesPerLine < packedBpl )
        {
            m_BytesPerLine = packedBpl;
        }

        uint32_t minByStride = m_BytesPerLine * (uint32_t)m_Height;
        if( m_MinFrameBytes < minByStride )
        {
            m_MinFrameBytes = minByStride;
        }

    }

    char chosenFourcc[5]{};
    formatToFourcc( m_PixelFormat, chosenFourcc );

    LogMsg( LOG_INFO, "CamV4L2::openDevice: opened %s %dx%d %s (requested %dx%d bpl=%u size=%u field=%u)",
        devPath.c_str(), m_Width, m_Height, chosenFourcc, desiredWidth, desiredHeight,
        m_BytesPerLine, m_MinFrameBytes, (unsigned)fmt.fmt.pix.field );

    if( !initMmap() )
    {
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    if( !startStreaming() )
    {
        uninitMmap();
        ::close( m_Fd );
        m_Fd = -1;
        return false;
    }

    m_CaptureThread = std::thread( [this]() { captureThreadFunc(); } );
    return true;
}

//============================================================================
void CamV4L2::closeDevice()
{
    m_Abort.store( true );
    if( m_CaptureThread.joinable() )
    {
        if( m_CaptureThread.get_id() == std::this_thread::get_id() )
        {
            // Never join the current thread; this can throw resource_deadlock_would_occur.
            LogMsg( LOG_WARN, "CamV4L2::closeDevice: skipping self-join on capture thread" );
            m_CaptureThread.detach();
        }
        else
        {
            try
            {
                m_CaptureThread.join();
            }
            catch( const std::system_error& err )
            {
                LogMsg( LOG_WARN, "CamV4L2::closeDevice: capture thread join failed: %s", err.what() );
                if( m_CaptureThread.joinable() )
                {
                    m_CaptureThread.detach();
                }
            }
        }
    }

    if( m_Fd >= 0 )
    {
        stopStreaming();
        uninitMmap();
        ::close( m_Fd );
        m_Fd = -1;
    }

    m_Width  = 0;
    m_Height = 0;
    m_PixelFormat = 0;
    m_BytesPerLine = 0;
    m_MinFrameBytes = 0;
    m_Abort.store( false );
}

//============================================================================
bool CamV4L2::supportsFormat( uint32_t pixelFormat )
{
    struct v4l2_fmtdesc desc{};
    desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for( desc.index = 0; ::ioctl( m_Fd, VIDIOC_ENUM_FMT, &desc ) == 0; ++desc.index )
    {
        if( desc.pixelformat == pixelFormat )
        {
            return true;
        }
    }

    return false;
}

//============================================================================
bool CamV4L2::initMmap()
{
    struct v4l2_requestbuffers req{};
    req.count  = MMAP_BUFFER_COUNT;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if( ::ioctl( m_Fd, VIDIOC_REQBUFS, &req ) < 0 )
    {
        LogMsg( LOG_ERROR, "CamV4L2::initMmap: VIDIOC_REQBUFS failed: %s", strerror( errno ) );
        return false;
    }

    if( req.count < 2 )
    {
        LogMsg( LOG_ERROR, "CamV4L2::initMmap: driver only granted %u mmap buffers (need at least 2)", req.count );
        return false;
    }

    m_Buffers.resize( req.count );

    for( unsigned i = 0; i < req.count; ++i )
    {
        struct v4l2_buffer buf{};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if( ::ioctl( m_Fd, VIDIOC_QUERYBUF, &buf ) < 0 )
        {
            LogMsg( LOG_ERROR, "CamV4L2::initMmap: VIDIOC_QUERYBUF[%u] failed: %s", i, strerror( errno ) );
            // unmap whatever was successfully mapped
            for( unsigned j = 0; j < i; ++j )
            {
                if( m_Buffers[j].start )
                    ::munmap( m_Buffers[j].start, m_Buffers[j].length );
            }
            m_Buffers.clear();
            return false;
        }

        m_Buffers[i].length = buf.length;
        m_Buffers[i].start  = ::mmap( nullptr, buf.length,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       m_Fd, buf.m.offset );

        if( m_Buffers[i].start == MAP_FAILED )
        {
            LogMsg( LOG_ERROR, "CamV4L2::initMmap: mmap[%u] failed: %s", i, strerror( errno ) );
            m_Buffers[i].start = nullptr;
            uninitMmap();
            return false;
        }
    }

    return true;
}

//============================================================================
void CamV4L2::uninitMmap()
{
    for( auto& buf : m_Buffers )
    {
        if( buf.start && buf.start != MAP_FAILED )
            ::munmap( buf.start, buf.length );
    }
    m_Buffers.clear();
}

//============================================================================
bool CamV4L2::startStreaming()
{
    for( unsigned i = 0; i < m_Buffers.size(); ++i )
    {
        struct v4l2_buffer buf{};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        if( ::ioctl( m_Fd, VIDIOC_QBUF, &buf ) < 0 )
        {
            LogMsg( LOG_ERROR, "CamV4L2::startStreaming: VIDIOC_QBUF[%u] failed: %s", i, strerror( errno ) );
            return false;
        }
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( ::ioctl( m_Fd, VIDIOC_STREAMON, &type ) < 0 )
    {
        LogMsg( LOG_ERROR, "CamV4L2::startStreaming: VIDIOC_STREAMON failed: %s", strerror( errno ) );
        return false;
    }

    if( LogEnabled( eLogWebCam ) )
    {
        LogModule( eLogWebCam, LOG_VERBOSE, "CamV4L2::startStreaming: streaming started" );
    }
    return true;
}

//============================================================================
void CamV4L2::stopStreaming()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( ::ioctl( m_Fd, VIDIOC_STREAMOFF, &type ) < 0 )
    {
        LogMsg( LOG_WARN, "CamV4L2::stopStreaming: VIDIOC_STREAMOFF failed: %s", strerror( errno ) );
    }
}

//============================================================================
void CamV4L2::captureThreadFunc()
{
    if( LogEnabled( eLogWebCam ) )
    {
        LogModule( eLogWebCam, LOG_VERBOSE, "CamV4L2: capture thread started %dx%d", m_Width, m_Height );
    }

    int64_t lastFrameMs = 0;
    tjhandle jpegDecoder = nullptr;
    if( m_PixelFormat == V4L2_PIX_FMT_MJPEG )
    {
        jpegDecoder = tjInitDecompress();
        if( !jpegDecoder )
        {
            LogMsg( LOG_ERROR, "CamV4L2: tjInitDecompress failed: %s", tjGetErrorStr() );
            return;
        }
    }

    while( !m_Abort.load() )
    {
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = SELECT_TIMEOUT_US;

        fd_set fds;
        FD_ZERO( &fds );
        FD_SET( m_Fd, &fds );

        int r = ::select( m_Fd + 1, &fds, nullptr, nullptr, &tv );
        if( r < 0 )
        {
            if( errno == EINTR )
                continue;
            LogMsg( LOG_ERROR, "CamV4L2: select error: %s", strerror( errno ) );
            break;
        }
        if( r == 0 )
            continue; // timeout — loop to re-check m_Abort

        struct v4l2_buffer buf{};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if( ::ioctl( m_Fd, VIDIOC_DQBUF, &buf ) < 0 )
        {
            if( errno == EAGAIN )
                continue;
            LogMsg( LOG_ERROR, "CamV4L2: VIDIOC_DQBUF error: %s", strerror( errno ) );
            break;
        }

        if( buf.index >= (unsigned)m_Buffers.size() )
        {
            LogMsg( LOG_ERROR, "CamV4L2: DQBUF returned out-of-range index %u", buf.index );
            break;
        }

        if( buf.flags & V4L2_BUF_FLAG_ERROR )
        {
            static int64_t s_LastV4l2BufErrorWarnMs = 0;
            int64_t nowMs = GetHighResolutionTimeMs();
            if( nowMs > s_LastV4l2BufErrorWarnMs + 2000 )
            {
                s_LastV4l2BufErrorWarnMs = nowMs;
                LogMsg( LOG_WARN, "CamV4L2: V4L2 flagged buffer error index=%u bytes=%u", buf.index, buf.bytesused );
            }

            if( ::ioctl( m_Fd, VIDIOC_QBUF, &buf ) < 0 )
            {
                LogMsg( LOG_ERROR, "CamV4L2: VIDIOC_QBUF error after flagged buffer: %s", strerror( errno ) );
                break;
            }

            continue;
        }

        // throttle to ~15 fps — same as CamFrameProcessor
        int64_t timeNow = GetHighResolutionTimeMs();
        if( timeNow >= lastFrameMs + FRAME_THROTTLE_MS && m_CamCapture.canProcessCamCapture() )
        {
            lastFrameMs = timeNow;

            uint32_t rgbLen = (uint32_t)( m_Width * m_Height * 3 );
            std::shared_ptr<uint8_t> rgbData( new uint8_t[rgbLen] );
            bool converted = false;
            if( m_PixelFormat == V4L2_PIX_FMT_YUYV )
            {
                uint32_t availableBytes = buf.bytesused ? buf.bytesused : (uint32_t)m_Buffers[buf.index].length;
                if( availableBytes >= m_MinFrameBytes )
                {
                    yuyvToRgb( static_cast<const uint8_t*>( m_Buffers[buf.index].start ),
                               (int)m_BytesPerLine,
                               rgbData.get(), m_Width, m_Height );
                    converted = true;
                }
                else
                {
                    static int64_t s_LastYuyvShortWarnMs = 0;
                    int64_t nowMs = GetHighResolutionTimeMs();
                    if( nowMs > s_LastYuyvShortWarnMs + 2000 )
                    {
                        s_LastYuyvShortWarnMs = nowMs;
                        LogMsg( LOG_WARN, "CamV4L2: short YUYV frame bytes=%u expected>=%u", availableBytes, m_MinFrameBytes );
                    }
                }
            }
            else if( m_PixelFormat == V4L2_PIX_FMT_MJPEG )
            {
                converted = mjpegToRgb( jpegDecoder,
                                        static_cast<const uint8_t*>( m_Buffers[buf.index].start ),
                                        (unsigned long)buf.bytesused,
                                        rgbData.get(), m_Width, m_Height );

                if( !converted )
                {
                    static int64_t s_LastMjpgDecodeWarnMs = 0;
                    int64_t nowMs = GetHighResolutionTimeMs();
                    if( nowMs > s_LastMjpgDecodeWarnMs + 2000 )
                    {
                        s_LastMjpgDecodeWarnMs = nowMs;
                        LogMsg( LOG_WARN, "CamV4L2: MJPG decode failed bytes=%u", buf.bytesused );
                    }
                }
            }

            if( converted )
            {
                m_CamCapture.getCamProcessor().processCamCapture( m_Width, m_Height, rgbData, (int)rgbLen );
            }
        }

        // requeue the buffer so the driver can fill it again
        if( ::ioctl( m_Fd, VIDIOC_QBUF, &buf ) < 0 )
        {
            LogMsg( LOG_ERROR, "CamV4L2: VIDIOC_QBUF error: %s", strerror( errno ) );
            break;
        }
    }

    if( LogEnabled( eLogWebCam ) )
    {
        LogModule( eLogWebCam, LOG_VERBOSE, "CamV4L2: capture thread stopped" );
    }

    if( jpegDecoder )
    {
        tjDestroy( jpegDecoder );
    }
}

//============================================================================
// BT.601 full-range YUYV-to-RGB888.
// Each 4-byte YUYV block encodes 2 pixels: [Y0 U Y1 V]
void CamV4L2::yuyvToRgb( const uint8_t* yuyv, int yuyvStride, uint8_t* rgb, int width, int height )
{
    const int rgbStride = width * 3;
    for( int row = 0; row < height; ++row )
    {
        const uint8_t* src = yuyv + (size_t)row * (size_t)yuyvStride;
        uint8_t* dst = rgb + (size_t)row * (size_t)rgbStride;

        for( int x = 0; x < width; x += 2 )
        {
            int y0 = src[0];
            int u  = src[1] - 128;
            int y1 = src[2];
            int v  = src[3] - 128;
            src += 4;

            // integer approximation of BT.601 coefficients (good to ~0.5 lsb)
            int rv =  ( v * 359 ) >> 8;
            int gu = -( u *  88 ) >> 8;
            int gv = -( v * 183 ) >> 8;
            int bu =  ( u * 454 ) >> 8;

            // pixel 0
            dst[0] = (uint8_t)clamp( y0 + rv );
            dst[1] = (uint8_t)clamp( y0 + gu + gv );
            dst[2] = (uint8_t)clamp( y0 + bu );
            // pixel 1
            dst[3] = (uint8_t)clamp( y1 + rv );
            dst[4] = (uint8_t)clamp( y1 + gu + gv );
            dst[5] = (uint8_t)clamp( y1 + bu );
            dst += 6;
        }
    }
}

#endif // defined(TARGET_OS_LINUX)
