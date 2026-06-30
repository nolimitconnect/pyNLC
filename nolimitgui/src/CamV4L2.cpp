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
#include "CamLogic.h"
#include "CamProcessor.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxElapseTimer.h>

#include <QByteArray>
#include <QImage>

#include <cerrno>
#include <cstring>
#include <map>
#include <memory>
#include <string>

#include <fcntl.h>
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
}

//============================================================================
CamV4L2::CamV4L2( CamLogic& camLogic )
    : m_CamLogic( camLogic )
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

    char chosenFourcc[5]{};
    formatToFourcc( m_PixelFormat, chosenFourcc );

    LogMsg( LOG_INFO, "CamV4L2::openDevice: opened %s %dx%d %s (requested %dx%d)",
        devPath.c_str(), m_Width, m_Height, chosenFourcc, desiredWidth, desiredHeight );

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
        m_CaptureThread.join();

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

        // throttle to ~15 fps — same as CamFrameProcessor
        int64_t timeNow = GetHighResolutionTimeMs();
        if( timeNow >= lastFrameMs + FRAME_THROTTLE_MS && m_CamLogic.canProcessCamCapture() )
        {
            lastFrameMs = timeNow;

            uint32_t rgbLen = (uint32_t)( m_Width * m_Height * 3 );
            std::shared_ptr<uint8_t> rgbData( new uint8_t[rgbLen] );
            bool converted = false;
            if( m_PixelFormat == V4L2_PIX_FMT_YUYV )
            {
                yuyvToRgb( static_cast<const uint8_t*>( m_Buffers[buf.index].start ),
                           rgbData.get(), m_Width, m_Height );
                converted = true;
            }
            else if( m_PixelFormat == V4L2_PIX_FMT_MJPEG )
            {
                QByteArray jpgBytes( reinterpret_cast<const char*>( m_Buffers[buf.index].start ), (int)buf.bytesused );
                QImage jpgImage = QImage::fromData( jpgBytes, "JPG" );
                if( !jpgImage.isNull() )
                {
                    QImage rgbImage = jpgImage.convertToFormat( QImage::Format_RGB888 );
                    if( !rgbImage.isNull() )
                    {
                        if( rgbImage.width() == m_Width && rgbImage.height() == m_Height )
                        {
                            if( rgbImage.bytesPerLine() == m_Width * 3 )
                            {
                                memcpy( rgbData.get(), rgbImage.bits(), rgbLen );
                            }
                            else
                            {
                                uint8_t* dest = rgbData.get();
                                for( int row = 0; row < m_Height; ++row )
                                {
                                    const uint8_t* src = rgbImage.constScanLine( row );
                                    memcpy( dest, src, (size_t)m_Width * 3 );
                                    dest += (size_t)m_Width * 3;
                                }
                            }
                            converted = true;
                        }
                    }
                }

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
                m_CamLogic.getCamProcessor().processCamCapture( m_Width, m_Height, rgbData, (int)rgbLen );
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
}

//============================================================================
// BT.601 full-range YUYV-to-RGB888.
// Each 4-byte YUYV block encodes 2 pixels: [Y0 U Y1 V]
void CamV4L2::yuyvToRgb( const uint8_t* yuyv, uint8_t* rgb, int width, int height )
{
    int pixelPairs = ( width * height ) / 2;
    const uint8_t* src = yuyv;
    uint8_t*       dst = rgb;

    for( int i = 0; i < pixelPairs; ++i )
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

#endif // defined(TARGET_OS_LINUX)
