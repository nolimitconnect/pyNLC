//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#pragma once

#include "CamProcessor.h"
#include <GuiInterface/ICamCapture.h>
#include <GuiInterface/IDefs.h>

#if defined(TARGET_OS_WINDOWS)
# include "CamWindows.h"
#elif defined(TARGET_OS_LINUX)
# include "CamV4L2.h"
# include <map>
#elif defined(TARGET_OS_ANDROID)
# include "CamJavaClient.h"
#endif // defined(TARGET_OS_ANDROID)

#include <string>

class CamJpgVideo;
class MediaProcessor;

class CamCapture : public ICamCapture
{
public:
    static const int64_t CAM_SNAPSHOT_INTERVAL_MS = 60; // 60 ms = approx 15 frames per second 30 ms = approx 30 fps
    static const int CAM_WIDTH = 320;
    static const int CAM_HEIGHT = 240;

    CamCapture();
    ~CamCapture();

    static CamCapture&          getInstance( void ){ static CamCapture camCaptureInstance; return camCaptureInstance; };

    void                        startupCamCapture( void ) override;
    void                        shutdownCamCapture( void ) override;

    void                        getCamCaptureDevices( std::vector<std::string>& deviceList ) override;
    bool                        setCamCaptureDevice( std::string camDescription ) override; // camDescription is same as camId

    // get last used or in use camera id
    std::string                 getCamId( void ) override { return m_CamId; };

    int                         rotateCurrentCamCapture( void ) override; // returns new cam rotation

    void                        setCamCaptureRotation( std::string camId, uint32_t camRotation ) override;
    uint32_t                    getCamCaptureRotation( std::string camId ) override;

 
    // start capture using the given cam id if camera is not disabled
    bool                        startCamCapture( std::string camDescription ) override;
    bool                        startCamCapture( void ) override;
    void                        stopCamCapture( void ) override;

    void                        wantCamCapture( EMediaModule mediaModule, bool wantVidCapture ) override;

    void                        setCamCaptureEnable( bool camEnable ) override;
    bool                        getCamCaptureEnable( void ) override { return m_CameraEnabled; }

    bool                        isCamCaptureAvailable( void ) override;
    bool                        isCamCaptureRequested( void ) override;
    bool                        isCamCaptureRunning( void ) override { return m_CaptureRunning; }

    int                         getCameraCount( void ) override;
    // start cam capture using next cam id if camera is not disabled
    bool                        nextCamera( void ) override;


    uint32_t                    getCurrentCamCaptureRotation( void ) { return m_CamRotation; }
    bool                        cameraExists( std::string camId );

    CamProcessor&               getCamProcessor( void ) { return m_CamProcessor; }

    void                        onCamCaptureReady( bool isReady );
    bool                        canProcessCamCapture( void );
    void                        processCamCapture( std::shared_ptr<CamJpgVideo>& jpgVideo );

    void                        updateCameraDevices( void );

    // start/stop capture using the last used cam id if camera is not disabled
    bool                        enableCamCapture( bool enableCapture );

protected:
    std::string                 selectLastUsedCamera( void ); // only called on startup

#if defined(TARGET_OS_WINDOWS)

#elif defined(TARGET_OS_LINUX)
    bool                        setV4L2Camera( const std::string& devPath );
#endif

    void                        updateCaptureRunning( bool capIsRunning );

    MediaProcessor&             m_MediaProcessor;
    CamProcessor                m_CamProcessor;

    bool                        m_WantCamInput[ eMaxMediaModule ];
    bool                        m_CameraEnabled{ false };
    bool                        m_CaptureRunning{ false };
    bool                        m_StartupRequested{ false };

    std::string                 m_CamId;

    std::vector<std::string>    m_AvailableCameras;

#if defined(TARGET_OS_ANDROID)
    CamJavaClient               m_CamJavaClient;
    std::vector<std::pair<bool,std::string>> m_CamIdList; // first = back facing, second = camId
#elif defined(TARGET_OS_WINDOWS)
    CamWindows                  m_CamWindows;
#elif defined(TARGET_OS_LINUX)
    std::map<std::string, std::string>  m_V4L2DeviceMap; // cardName -> /dev/videoN
    CamV4L2                             m_CamV4L2;
#endif // defined(TARGET_OS_ANDROID)

    uint32_t                    m_CamRotation{ 0 };
};
