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

#include <GuiInterface/IDefs.h>

class ICamCapture
{
public:
    static ICamCapture&			getICamCapture( void );

    virtual void                startupCamCapture( void ) = 0;
    virtual void                shutdownCamCapture( void ) = 0;

    virtual void                getCamCaptureDevices( std::vector<std::string>& deviceList ) = 0;

    virtual bool                setCamCaptureDevice( std::string camDescription ) = 0; // camDescription is same as camId
    // get last used or in use camera id
    virtual std::string         getCamId( void ) = 0;

    // this startCamCapture calls setCamCaptureDevice then startCamCapture
    virtual bool                startCamCapture( std::string camId ) = 0;
    virtual bool                startCamCapture( void ) = 0;
    virtual void                stopCamCapture( void ) = 0;

    virtual void                wantCamCapture( EMediaModule mediaModule, bool wantVidCapture ) = 0;

    virtual int                 rotateCurrentCamCapture( void ) = 0; // returns new cam rotation

    virtual void				setCamCaptureRotation( std::string camId, uint32_t camRotation ) = 0;
	virtual uint32_t			getCamCaptureRotation( std::string camId ) = 0;
  
    virtual void                setCamCaptureEnable( bool camEnable ) = 0;
    virtual bool                getCamCaptureEnable( void ) = 0;

    virtual bool                isCamCaptureAvailable( void ) = 0;
    virtual bool                isCamCaptureRequested( void ) = 0;
    virtual bool                isCamCaptureRunning( void ) = 0;

    virtual int                 getCameraCount( void ) = 0;
    // start cam capture using next cam id if camera is not disabled
    virtual bool                nextCamera( void ) = 0;

};
