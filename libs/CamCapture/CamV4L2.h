#pragma once
//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
// Direct V4L2 camera capture — bypasses Qt Multimedia entirely.
// Enumerate /dev/videoN devices, open one, stream YUYV frames, convert to
// RGB24, and hand them to CamProcessor exactly as CamFrameProcessor did.
//============================================================================

#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class CamCapture;

// CamV4L2 replaces the Qt Multimedia camera capture code when compiling on Linux
// This is to avoid the Qt Multimedia bug where on Ubuntu running in a virtualbox the camera image is scrambled 
// and only partial QVideoFrame is delivered.  
// The V4L2 code is based on the example code at https://www.kernel.org/doc/html/v4.19/media/uapi/v4l/capture.c.html
class CamV4L2
{
public:
    explicit CamV4L2( CamCapture& camLogic );
    ~CamV4L2();

    // Fill 'devices' with (cardName, devPath) for every V4L2 capture node found.
    static void enumerateDevices( std::vector<std::pair<std::string, std::string>>& devices );

    // Open devPath, negotiate capture format at the requested resolution (driver may adjust),
    // allocate mmap buffers, start streaming and the capture thread.
    bool                        openDevice( const std::string& devPath, int desiredWidth, int desiredHeight );

    // Stop streaming and close the device.  Safe to call if already closed.
    void                        closeDevice();

    bool                        isOpen()   const { return m_Fd >= 0; }
    int                         getWidth() const { return m_Width; }
    int                         getHeight()const { return m_Height; }

private:
    void                        captureThreadFunc();

    bool                        initMmap();
    void                        uninitMmap();
    bool                        startStreaming();
    void                        stopStreaming();
    bool                        supportsFormat( uint32_t pixelFormat );

    // Convert a YUYV frame with source stride to RGB888.
    static void                 yuyvToRgb( const uint8_t* yuyv, int yuyvStride, uint8_t* rgb, int width, int height );
    static int                  clamp( int v ) { return v < 0 ? 0 : (v > 255 ? 255 : v); }


    CamCapture&                 m_CamCapture;
    int                         m_Fd{ -1 };
    int                         m_Width{ 0 };
    int                         m_Height{ 0 };
    uint32_t                    m_PixelFormat{ 0 };
    uint32_t                    m_BytesPerLine{ 0 };
    uint32_t                    m_MinFrameBytes{ 0 };

    struct MmapBuffer {
        void*  start{ nullptr };
        size_t length{ 0 };
    };
    std::vector<MmapBuffer>     m_Buffers;

    std::atomic<bool>           m_Abort{ false };
    std::thread                 m_CaptureThread;
};
