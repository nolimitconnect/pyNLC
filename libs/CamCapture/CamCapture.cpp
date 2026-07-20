//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "CamCapture.h"

#if defined(TARGET_OS_WINDOWS)
#include "CamWindows.h"
#endif // defined(TARGET_OS_WINDOWS)

#include <P2PEngine/P2PEngine.h>
#include <MediaProcessor/MediaProcessor.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/IGlobalDb.h>
#include <CoreLib/VxJni.h>

//============================================================================
CamCapture::CamCapture()
    : m_MediaProcessor( GetPtoPEngine().getMediaProcessor() )
    , m_CamProcessor(*this)
#if defined(TARGET_OS_ANDROID)
    , m_CamJavaClient( *this )
#elif defined(TARGET_OS_WINDOWS)
    , m_CamWindows( *this )
#elif defined(TARGET_OS_LINUX)
    , m_CamV4L2( *this )
#else
    echo "Unsupported platform"
#endif // defined(TARGET_OS_ANDROID)
{
    memset( m_WantCamInput, 0, sizeof( m_WantCamInput ) );
}

//============================================================================
CamCapture::~CamCapture() {
#if defined(TARGET_OS_ANDROID)
    shutdownCamCapture();
#elif defined(TARGET_OS_LINUX)
    m_CamV4L2.closeDevice();
#else
    m_CamWindows.stopCamCapture();
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
bool CamCapture::isCamCaptureRequested( void )
{
    bool isRequested{ false };
    for( int i = 0; i < eMaxMediaModule; i++ )
    {
        if( m_WantCamInput[i] )
        {
            isRequested = true;
            break;
        }
    }

    return isRequested;
}

//============================================================================
void CamCapture::startupCamCapture( void )
{
    if( m_StartupRequested )
    {
        return;
    }


#if defined(TARGET_OS_ANDROID)    
    LogModule( eLogWebCam, LOG_VERBOSE, "CamCapture::%s Android camera startup requested", __func__ );

    if( !VxJni::hasPermission( "android.permission.CAMERA" ) )
    {
        LogMsg( LOG_WARN, "%s requesting CAMERA permission", __func__ );
        if( !VxJni::requestPermission( "android.permission.CAMERA", 1001 ) )
        {
            LogMsg( LOG_ERROR, "%s failed to request CAMERA permission", __func__ );
        }

        return;
    }
#endif // defined(TARGET_OS_ANDROID)

    m_StartupRequested = true;

#if defined(TARGET_OS_ANDROID)
    m_CamJavaClient.startupCamCapture();
    // onCamCaptureReady will get called after camera service starts
#elif defined(TARGET_OS_LINUX)
    onCamCaptureReady( true );
#else
    onCamCaptureReady( true );
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
void CamCapture::shutdownCamCapture( void )
{
    m_StartupRequested = false;
    enableCamCapture( false );

#if defined(TARGET_OS_ANDROID)
    m_CamJavaClient.shutdownCamCapture();
#elif defined(TARGET_OS_LINUX)
    m_CamV4L2.closeDevice();
#else
    m_CamWindows.stopCamCapture();
#endif // defined(TARGET_OS_ANDROID)

    if( m_CaptureRunning )
    {
        updateCaptureRunning( false );
    }
}

//============================================================================
void CamCapture::onCamCaptureReady( bool isReady )
{
    updateCameraDevices();

    m_CamId = selectLastUsedCamera();
    if( m_CamId.empty() )
    {
        LogMsg( LOG_WARN, "%s NO AVAILABLE CAMERAS", __func__ );
    }
    else
    {
        LogMsg( LOG_VERBOSE, "%s last used camera %s", __func__, m_CamId.c_str() );
        m_CamRotation = IGlobalDb::getIGlobalDb().getCamRotation( m_CamId );
        if( isReady && isCamCaptureRequested() && !VxIsAppShuttingDown() && !isCamCaptureRunning() )
        {
            // there may have been requests for cam capture before camera was ready, so start capture if there are requests and camera is ready
            // on android the delay before capture ready is expecially long
            startCamCapture( m_CamId );
        }
    }
}

//============================================================================
std::string CamCapture::selectLastUsedCamera( void )
{
    std::string camId = IGlobalDb::getIGlobalDb().getCamSourceId();
    if( !camId.empty() && cameraExists( camId ) )
    {
        return camId;
    }

    if( !m_AvailableCameras.empty() )
    {
        camId = m_AvailableCameras.front();
        IGlobalDb::getIGlobalDb().setCamSourceId( camId );
        return camId;
    }

    return "";
}

//============================================================================
bool CamCapture::canProcessCamCapture( void )
{
    if( VxIsAppShuttingDown() || !isCamCaptureRequested() || m_CamProcessor.isStalled() )
    {
        if( isCamCaptureRequested() )
        {
            if( LogEnabled( eLogWebCam ) )LogModule( eLogWebCam, LOG_WARN, "CamCapture::%s cannot process cam que rgb %zu jpg %zu", __func__, m_CamProcessor.getRgbQueueSize(), m_CamProcessor.getJpgQueueSize() );
        }

        return false;
    }

    return true;
}

//============================================================================
void CamCapture::processCamCapture( std::shared_ptr<CamJpgVideo>& jpgVideo )
{
    m_MediaProcessor.processCamCaptureJpgVideo( jpgVideo );
}

//============================================================================
void CamCapture::updateCameraDevices( void )
{
#if defined(TARGET_OS_ANDROID)
    if( !m_StartupRequested )
    {
        startupCamCapture();
        return;
    }

    m_CamJavaClient.getCameraDevices( m_CamIdList );

    m_AvailableCameras.clear();
    for( auto device : m_CamIdList )
    {
        m_AvailableCameras.emplace_back( device.second );
    }
#elif defined(TARGET_OS_LINUX)
    m_V4L2DeviceMap.clear();
    std::vector<std::pair<std::string, std::string>> devices;
    CamV4L2::enumerateDevices( devices );
    for( const auto& dev : devices )
    {
        m_V4L2DeviceMap[dev.first] = dev.second;
    }

    m_AvailableCameras.clear();
    for( const auto& dev : m_V4L2DeviceMap )
    {
        m_AvailableCameras.emplace_back( dev.first );
    }
#else
    m_AvailableCameras.clear();
    m_CamWindows.getCamCaptureDevices( m_AvailableCameras );
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
void CamCapture::getCamCaptureDevices( std::vector<std::string>& retCamList )
{
    retCamList = m_AvailableCameras;
}

//============================================================================
bool CamCapture::startCamCapture( std::string camId )
{
    if( camId.empty() )
    {
        LogMsg( LOG_ERROR, "%s camId.empty()", __func__ );
        return false;
    }

    stopCamCapture(); // stop previous capture if running

    if( !setCamCaptureDevice( camId ) )
    {
        return false;
    }
    
    return enableCamCapture( true );
}

//============================================================================
bool CamCapture::setCamCaptureDevice( std::string camDescription )
{
    if( camDescription.empty() )
    {
        return false;
    }

    for( const auto& device : m_AvailableCameras )
    {
        if( device == camDescription )
        {
            m_CamId = device;
            IGlobalDb::getIGlobalDb().setCamSourceId( m_CamId );
            m_CamRotation = IGlobalDb::getIGlobalDb().getCamRotation( m_CamId );
            return true;
        }
    }

    return false;
}

//============================================================================
bool CamCapture::startCamCapture( void )
{
    return enableCamCapture( true );
}

//============================================================================
void CamCapture::stopCamCapture( void )
{
    enableCamCapture( false );
}

//============================================================================
#if defined(TARGET_OS_LINUX)
bool CamCapture::setV4L2Camera( const std::string& devPath )
{
    if( !m_CameraEnabled )
    {
        LogModule( eLogWebCam, LOG_VERBOSE, "CamCapture::%s called but cam is disabled", __func__ );
        return false;
    }

    m_CamV4L2.closeDevice();
    bool opened = m_CamV4L2.openDevice( devPath, CAM_WIDTH, CAM_HEIGHT );
    if( !opened )
    {
        LogMsg( LOG_ERROR, "%s failed opening %s", __func__, devPath.c_str() );
    }

    return opened;
}
#endif

//============================================================================
void CamCapture::wantCamCapture( EMediaModule mediaModule, bool wantVidCapture )
{
    if( VxIsAppShuttingDown() )
    {
        return;
    }

    bool isWanted = isCamCaptureRequested();
    m_WantCamInput[mediaModule] = wantVidCapture;
    bool isWantedAfter = isCamCaptureRequested();
    if( isWanted != isWantedAfter )
    {
        enableCamCapture( isWantedAfter );
    }
}

//============================================================================
bool CamCapture::cameraExists( std::string camId )
{
    if( camId.empty() )
    {
        return false;
    }

    for( auto& device : m_AvailableCameras )
    {
        if( device == camId )
        {
            return true;
        }
    }

    return false;
}

//============================================================================
void CamCapture::setCamCaptureEnable( bool camEnable )
{
    if( camEnable == m_CameraEnabled )
    {
        return;
    }

    m_CameraEnabled = camEnable;
    if( !m_CameraEnabled )
    {
         enableCamCapture( false );
    }
    else
    {
        if( isCamCaptureRequested() )
        {
            enableCamCapture( true );
        }
    }

    IGlobalDb::getIGlobalDb().setCamEnable( camEnable );
    IToGui::getIToGui().toGuiCamCaptureEnable( camEnable );
}

//============================================================================
bool CamCapture::nextCamera( void )
{
    updateCameraDevices();
    if( getCameraCount() < 2 )
    {
        return false;
    }

    if( !cameraExists( getCamId() ) )
    {
        return startCamCapture( m_AvailableCameras.front() );
    }

    std::string camId = getCamId();
    int curCamIdx = 0;
    int iterCamIdx = 0;
    for( auto& device : m_AvailableCameras )
    {
        if( device == camId )
        {
            curCamIdx = iterCamIdx;
        }

        iterCamIdx++;
    }

    int nextCamIdx = (curCamIdx + 1) % m_AvailableCameras.size();
    camId = m_AvailableCameras[nextCamIdx];
    if( cameraExists( camId ) )
    {
        return startCamCapture( camId );
    }
   
    return false;
}

//============================================================================
int CamCapture::getCameraCount( void )
{
    return (int)m_AvailableCameras.size();
}

//============================================================================
bool CamCapture::isCamCaptureAvailable( void )
{
    return getCameraCount();
}

//============================================================================
bool CamCapture::enableCamCapture( bool enable )
{
    if( !enable )
    {
    #if defined(TARGET_OS_ANDROID)
        m_CamJavaClient.stopCamCapture();
    #elif defined(TARGET_OS_LINUX)
        m_CamV4L2.closeDevice();
    #elif defined(TARGET_OS_WINDOWS)
        m_CamWindows.stopCamCapture();
    #endif // defined(TARGET_OS_ANDROID)

        if( m_CaptureRunning != enable )
        {
            updateCaptureRunning( enable );
        }

        return false;
    }

    if( m_CamId.empty() )
    {
        LogMsg( LOG_ERROR, "%s m_CamId.empty()", __func__ );
        return false;
    }

    if( !m_CameraEnabled )
    {
        LogMsg( LOG_ERROR, "%s !m_CameraEnabled", __func__ );
        return false;
    }

    if( !isCamCaptureRequested() )
    {
        LogMsg( LOG_ERROR, "%s !isCamCaptureRequested()", __func__ );
        return false;
    }


#if defined(TARGET_OS_ANDROID)
    if( !m_StartupRequested )
    {
        startupCamCapture();
        LogMsg( LOG_INFO, "%s requested camera capture before Android camera service was ready", __func__ );
        return false;
    }

    if( m_CamId.empty() )
    {
        LogMsg( LOG_WARN, "%s waiting for Android camera service to provide camera list", __func__ );
        return false;
    }

    LogModule( eLogWebCam, LOG_VERBOSE, "CamCapture::%s starting Android camera capture camId=%s", __func__, m_CamId.c_str() );
    bool capRunning = m_CamJavaClient.startCamCapture( m_CamId );
    LogModule( eLogWebCam, LOG_VERBOSE, "CamCapture::%s Android camera capture start result=%d", __func__, capRunning );
    if( m_CaptureRunning != capRunning )
    {
        updateCaptureRunning( capRunning );
    }

    return capRunning;
#elif defined(TARGET_OS_LINUX)
    auto devIter = m_V4L2DeviceMap.find( m_CamId );
    if( devIter == m_V4L2DeviceMap.end() )
    {
        LogMsg( LOG_DEBUG, "%s camera %s NOT available", __func__, m_CamId.c_str() );
        return false;
    }

    bool capRunning = setV4L2Camera( devIter->second );
    if( !capRunning )
    {
        // Some webcams expose multiple /dev/video* nodes; try other nodes as fallback.
        for( const auto& entry : m_V4L2DeviceMap )
        {
            if( entry.first == m_CamId )
            {
                continue;
            }

            if( setV4L2Camera( entry.second ) )
            {
                m_CamId = entry.first;
                IGlobalDb::getIGlobalDb().setCamSourceId( m_CamId );
                capRunning = true;
                LogMsg( LOG_WARN, "%s fallback camera selected %s", __func__, m_CamId.c_str() );
                break;
            }
        }
    }

    if( m_CaptureRunning != capRunning )
    {
        updateCaptureRunning( capRunning );
    }

    return capRunning;
#elif defined(TARGET_OS_WINDOWS)
    if( m_CamId.empty() )
    {
        LogMsg( LOG_ERROR, "%s m_CamId.empty()", __func__ );
        return false;
    }

    bool camStarted = m_CamWindows.startCamCapture( m_CamId );

     if( !m_CaptureRunning && camStarted )
     {
         updateCaptureRunning( true );
     }

    return camStarted;
#else
    LogMsg( LOG_ERROR, "%s unsupported platform", __func__ );
    return false;
#endif // defined(TARGET_OS_ANDROID)

    LogMsg( LOG_DEBUG, "%s camera %s NOT available", __func__, m_CamId.c_str() );
    return false;
}

//============================================================================
void CamCapture::updateCaptureRunning( bool capIsRunning )
{
    if( m_CaptureRunning != capIsRunning )
    {
        m_CaptureRunning = capIsRunning;
        if( !VxIsAppShuttingDown() )
        {
            IToGui::getIToGui().toGuiCamCaptureRunning( capIsRunning );
        }   
    }
}

//============================================================================
void CamCapture::setCamCaptureRotation( std::string camId, uint32_t camRotation )
{
    IGlobalDb::getIGlobalDb().setCamRotation( camId, camRotation );
    if( camId == m_CamId )
    {
        m_CamRotation = camRotation;
    }
}

//============================================================================
uint32_t CamCapture::getCamCaptureRotation( std::string camId )
{
    if( camId == m_CamId )
    {
        return m_CamRotation;
    }
    
    return IGlobalDb::getIGlobalDb().getCamRotation( camId );
}

//============================================================================
int CamCapture::rotateCurrentCamCapture( void )
{
    m_CamRotation += 90;
    if( m_CamRotation >= 360 )
    {
        m_CamRotation = 0;
    }

    IGlobalDb::getIGlobalDb().setCamRotation( m_CamId, m_CamRotation );
    return m_CamRotation;
}