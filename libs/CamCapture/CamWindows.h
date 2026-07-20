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

#include <string>
#include <vector>
#include <memory>

class CamCapture;

class CamWindows {
public:
    CamWindows(CamCapture& camCapture);
    ~CamWindows();

    // Prevent copying to avoid multiple objects fighting for the same hardware
    CamWindows(const CamWindows&) = delete;
    CamWindows& operator=(const CamWindows&) = delete;

    // API
    void                        getCamCaptureDevices(std::vector<std::string>& retCamList);
    bool                        cameraExists(std::string camId);
    bool                        startCamCapture(std::string camId);
    void                        stopCamCapture(void);

    bool                        isCamCaptureRunning( void );

private:

    class Impl;
    std::unique_ptr<Impl>       pImpl;
};
