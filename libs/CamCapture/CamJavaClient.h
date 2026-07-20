#pragma once
//============================================================================
// Copyright (C) 2025 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#if defined(TARGET_OS_ANDROID)

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CamCapture;

class CamJavaClient
{
public:
    explicit CamJavaClient( CamCapture& camLogic );
    ~CamJavaClient();

    void                        startupCamCapture( void );
    void                        shutdownCamCapture( void );

    void                        getCameraDevices( std::vector<std::pair<bool,std::string>>& camIdList );

    void                        onCamServiceStarted( void );
    void                        onCameraPermissionResult( bool granted );
    bool                        canProcessCamCapture( void );
    void                        processCamCapture( int width, int height, std::shared_ptr<uint8_t>& rgbData, int dataLen );

    bool                        startCamCapture( std::string camId );
    void                        stopCamCapture( void );

protected:
    void                        updateCameraList( void );
    bool                        isBackFacing( std::string& camId );

    CamCapture&                 m_CamCapture;

    std::vector<std::string>    m_CamIdList;

};

#endif // defined(TARGET_OS_ANDROID)
