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

#include "CamJavaClient.h"

#include "CamCapture.h"
#include "CamProcessor.h"

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxJava.h>
#include <CoreLib/VxJni.h>
#include <CoreLib/VxThread.h>
#include <CoreLib/VxTime.h>

#include <iostream>

#define CAM_CAPTURE_CLASS_NAME "org/nolimitconnect/nolimitconnect/Camera2Service"

namespace {
CamJavaClient* g_CamClient = nullptr;
CamJavaClient* GetCamJavaClient() {
    return g_CamClient;
}

typedef struct CamServiceMethod {
    jmethodID methodID;
    const char* methodName;
    const char* methodSigniture;
} CamServiceMethod;

const int CAM_GET_IDS_IDX = 0;
const int CAM_IS_BACKFACING_IDX = 1;
const int CAM_START_CAPTURE_IDX = 2;
const int CAM_STOP_CAPTURE_IDX = 3;
const int CAM_GET_VALUE_IDX = 4;
CamServiceMethod g_CamMethods[] {
    {0, "getCameraIdList", "()[Ljava/lang/String;"},
    {0, "isCameraBackFacing", "(Ljava/lang/String;)Z"},
    {0, "startCameraCapture", "(Ljava/lang/String;)Z"},
    {0, "stopCameraCapture", "()V"},
    {0, "getValue", "()I"}
};

JNIEnv* g_CamEnv = nullptr;
jobject g_CamObj = nullptr;
unsigned int g_CamServiceThreadId = 0;
bool g_CamServiceReady = false;

static std::vector<std::pair<unsigned int, JNIEnv*>> g_JavaEnvList;

bool HasCamMethod( int methodIdx, const char* funcName )
{
    if( methodIdx < 0 || methodIdx >= (int)(sizeof(g_CamMethods)/sizeof(CamServiceMethod)) )
    {
        LogMsg( LOG_ERROR, "%s invalid method idx %d", funcName, methodIdx );
        return false;
    }

    if( nullptr == g_CamMethods[methodIdx].methodID )
    {
        LogMsg( LOG_ERROR, "%s null methodID for %s", funcName, g_CamMethods[methodIdx].methodName );
        return false;
    }

    return true;
}

JNIEnv* GetJniEnv( void )
{
    unsigned int threadId = VxGetCurrentThreadId();
    if( threadId == g_CamServiceThreadId)
    {
        return g_CamEnv;
    }

    for(auto& pair : g_JavaEnvList)
    {
        if( pair.first == threadId)
        {
            return pair.second;
        }
    }

    JNIEnv* jniEnv = nullptr;
    if (GetJavaVM()->AttachCurrentThread(&jniEnv, NULL) == JNI_OK)
    {
        g_JavaEnvList.emplace_back(std::make_pair(threadId, jniEnv));
        return jniEnv;
    }
    else
    {
        LogMsg( LOG_ERROR, "%s AttachCurrentThread failed", __func__ );
        return nullptr;
    }
}

static inline uint8_t clamp(int value) {
    return (value < 0) ? 0 : ((value > 255) ? 255 : value);
}

void AndroidYUV420SPtoRGB(uint8_t* rgbImage, int width, int height,
                          const uint8_t* yPlane, const uint8_t* uPlane, const uint8_t* vPlane,
                          int yPixelStride, int yRowStride,
                          int uPixelStride, int uRowStride,
                          int vPixelStride, int vRowStride) {
    for (int row = 0; row < height; row++) {
        const uint8_t* pYRow = yPlane + row * yRowStride;
        const uint8_t* pURow = uPlane + (row / 2) * uRowStride;
        const uint8_t* pVRow = vPlane + (row / 2) * vRowStride;

        for (int col = 0; col < width; col++) {
            int yIndex = col * yPixelStride;
            int uvCol = (col / 2);
            int uIndex = uvCol * uPixelStride;
            int vIndex = uvCol * vPixelStride;

            int Y = pYRow[yIndex];
            int U = pURow[uIndex] - 128;
            int V = pVRow[vIndex] - 128;

            int y1024 = Y << 10;

            int R = (y1024 + 1436 * V) >> 10;
            int G = (y1024 - 352 * U - 731 * V) >> 10;
            int B = (y1024 + 1814 * U) >> 10;

            *rgbImage++ = clamp(R);
            *rgbImage++ = clamp(G);
            *rgbImage++ = clamp(B);
        }
    }
}

} // namespace

// native methods called from Java
extern "C" {

JNIEXPORT void JNICALL Java_org_nolimitconnect_nolimitconnect_Camera2Service_camServiceStarted(JNIEnv *env, jobject obj) {
    g_CamEnv = env;
    g_CamServiceThreadId = VxGetCurrentThreadId();

    // Obtain a reference to the class of the passed object (obj)
    jclass clazz = env->GetObjectClass(obj);

    if (clazz == nullptr) {
        std::cerr << "Failed to find class" << std::endl;
        return;
    }

    // Hold a reference to the Java object
    // to prevent it from being garbage collected if needed
    g_CamObj = env->NewGlobalRef(obj);
    if( nullptr == g_CamObj )
    {
        LogMsg( LOG_ERROR, "%s NewGlobalRef failed", __func__ );
        return;
    }

    // Use the global reference (you could store it for later use)

    // Don't forget to delete the global reference when you're done
    //env->DeleteGlobalRef(g_CamObj);

    for( int i = 0; i < sizeof(g_CamMethods)/sizeof(CamServiceMethod); i++)
    {
        g_CamMethods[i].methodID = env->GetMethodID(clazz, g_CamMethods[i].methodName, g_CamMethods[i].methodSigniture);
        if (g_CamMethods[i].methodID == nullptr) {
            LogMsg( LOG_ERROR, "%s Failed to find method %s", __func__, g_CamMethods[i].methodName );
            vx_assert(false);
            return;
        }
    }

    // Test is valid by calling the 'getValue' method on the passed object and get the result
    jint value = env->CallIntMethod(obj, g_CamMethods[CAM_GET_VALUE_IDX].methodID);

    LogMsg( LOG_VERBOSE, "%s Received value: %d", __func__, value );

    g_CamServiceReady = true;
    CamJavaClient* camClient = GetCamJavaClient();
    if( camClient )
    {
        camClient->onCamServiceStarted();
    }
    else
    {
        LogMsg( LOG_WARN, "%s camera service started after native CamJavaClient was destroyed", __func__ );
    }
}

JNIEXPORT void JNICALL Java_org_nolimitconnect_nolimitconnect_Camera2Service_camServiceStopped(JNIEnv *env, jobject obj) {
    g_CamServiceReady = false;
    if( nullptr != g_CamObj )
    {
        env->DeleteGlobalRef( g_CamObj );
        g_CamObj = nullptr;
    }

    for( int i = 0; i < (int)(sizeof(g_CamMethods)/sizeof(CamServiceMethod)); ++i )
    {
        g_CamMethods[i].methodID = nullptr;
    }

    g_CamEnv = nullptr;
    g_CamServiceThreadId = 0;
    g_JavaEnvList.clear();

    LogMsg( LOG_VERBOSE, "%s ", __func__ );
}

JNIEXPORT bool JNICALL Java_org_nolimitconnect_nolimitconnect_Camera2Service_canProcessCamCapture(JNIEnv *env, jobject obj) {
    CamJavaClient* camClient = GetCamJavaClient();
    if( !camClient )
    {
        return false;
    }

    return camClient->canProcessCamCapture();
}

JNIEXPORT void JNICALL Java_org_nolimitconnect_nolimitconnect_Camera2Service_camPermissionResult(JNIEnv* env, jclass clazz, jboolean granted) {
    CamJavaClient* camClient = GetCamJavaClient();
    if( !camClient )
    {
        LogMsg( LOG_WARN, "%s camera permission result arrived after native CamJavaClient was destroyed", __func__ );
        return;
    }

    camClient->onCameraPermissionResult( granted == JNI_TRUE );
}

bool GetJBufInfo( JNIEnv* env, jobject byteBuffer, uint8_t*& byteBuf )
{
    if( nullptr == env || nullptr == byteBuffer )
    {
        LogMsg( LOG_ERROR, "%s invalid JNIEnv or ByteBuffer", __func__ );
        return false;
    }

    void* bufferAddress = env->GetDirectBufferAddress( byteBuffer );
    if( nullptr == bufferAddress )
    {
        LogMsg( LOG_ERROR, "%s GetDirectBufferAddress failed", __func__ );
        return false;
    }

    const jlong bufferCapacity = env->GetDirectBufferCapacity( byteBuffer );
    if( bufferCapacity <= 0 )
    {
        LogMsg( LOG_ERROR, "%s invalid direct buffer capacity %lld", __func__, (long long)bufferCapacity );
        return false;
    }

    byteBuf = reinterpret_cast<uint8_t*>( bufferAddress );
    return true;
}

JNIEXPORT void JNICALL Java_org_nolimitconnect_nolimitconnect_Camera2Service_processCamCapture(JNIEnv *env, jobject obj,
                                                                                            int width, int height, jobject yBuf, jobject uBuf, jobject vBuf,
                                                                                            int yPixelStride, int yRowStride,
                                                                                            int uPixelStride, int uRowStride,
                                                                                            int vPixelStride, int vRowStride )
{
    CamJavaClient* camClient = GetCamJavaClient();
    if( !camClient )
    {
        return;
    }

    if( width < 10 || height < 10 || width > 10000 || height > 10000)
    {
        LogMsg( LOG_ERROR, "%s invalid param width %d height %d", __func__, width, height );
    }

    uint8_t* y = 0;
    uint8_t* u = 0;
    uint8_t* v = 0;
    if(GetJBufInfo(env, yBuf, y) && GetJBufInfo(env, uBuf, u) && GetJBufInfo(env, vBuf, v))
    {
        int dataLen = width * height * 3;
        std::shared_ptr<uint8_t> rgbData( new uint8_t[dataLen] );

        AndroidYUV420SPtoRGB( rgbData.get(),
                              width, height, y, u, v,
                              yPixelStride, yRowStride,
                              uPixelStride, uRowStride,
                              vPixelStride, vRowStride );

        camClient->processCamCapture(width, height, rgbData, dataLen);
    }
    else
    {
        LogMsg( LOG_ERROR, "%s failed to get yuv buffers", __func__ );
    }
}

} // extern "C"

//============================================================================
CamJavaClient::CamJavaClient( CamCapture& camLogic )
    : m_CamCapture( camLogic )
{
    g_CamClient = this;
}

//============================================================================
CamJavaClient::~CamJavaClient()
{
    shutdownCamCapture();
    g_CamClient = nullptr;
}

//============================================================================
void CamJavaClient::startupCamCapture( void )
{
    LogMsg( LOG_VERBOSE, "%s GUI thread id %d", __func__, VxGetCurrentThreadId() );
    if( !VxJni::callStaticVoidWithAppContext(
            CAM_CAPTURE_CLASS_NAME,
            "startCamServiceStatic",
            "(Landroid/content/Context;)V" ) )
    {
        LogMsg( LOG_ERROR, "%s startCamServiceStatic failed", __func__ );
        return;
    }

    // wait for service to be started
}

//============================================================================
void CamJavaClient::shutdownCamCapture( void )
{
    stopCamCapture();

    if( !VxJni::callStaticVoidWithAppContext(
            CAM_CAPTURE_CLASS_NAME,
            "stopCamServiceStatic",
            "(Landroid/content/Context;)V" ) )
    {
        LogMsg( LOG_WARN, "%s stopCamServiceStatic failed", __func__ );
    }
}

//============================================================================
void CamJavaClient::onCamServiceStarted( void )
{
    LogMsg( LOG_VERBOSE, "%s thread id %d", __func__, VxGetCurrentThreadId() );
    updateCameraList();
    m_CamCapture.onCamCaptureReady( true );
}

//============================================================================
void CamJavaClient::onCameraPermissionResult( bool granted )
{
    LogMsg( LOG_VERBOSE, "%s granted=%d", __func__, granted ? 1 : 0 );
    if( !granted )
    {
        LogMsg( LOG_WARN, "%s camera permission denied", __func__ );
        return;
    }

    // Re-enter startup after runtime permission grant.
    m_CamCapture.startupCamCapture();
}

//============================================================================
bool CamJavaClient::canProcessCamCapture( void )
{
    static int64_t lastTimeMs = 0;
    int64_t timeNow = GetGmtTimeMs();
    constexpr int64_t ANDROID_CAM_MIN_INTERVAL_MS = 83; // Cap at ~12 FPS for Android capture path.
    const int64_t minFrameIntervalMs = ( CamCapture::CAM_SNAPSHOT_INTERVAL_MS > ANDROID_CAM_MIN_INTERVAL_MS )
                                      ? CamCapture::CAM_SNAPSHOT_INTERVAL_MS
                                      : ANDROID_CAM_MIN_INTERVAL_MS;
    if( timeNow < lastTimeMs + minFrameIntervalMs )
    {
        //LogMsg( LOG_VERBOSE, "%s time too short %d ms", __func__, (int)(timeNow - lastTimeMs) );
        return false;
    }
    
    bool result =  m_CamCapture.canProcessCamCapture();
    if( result )
    {
        lastTimeMs = timeNow;
    }
//    else
//    {
//        LogMsg( LOG_VERBOSE, "%s CamCapture returned false", __func__ );
//    }

    return result;
}

//============================================================================
void CamJavaClient::processCamCapture( int width, int height, std::shared_ptr<uint8_t>& rgbData, int dataLen )
{
    m_CamCapture.getCamProcessor().processCamCapture( width, height, rgbData, dataLen );
}

//============================================================================
void CamJavaClient::getCameraDevices( std::vector<std::pair<bool,std::string>>& camIdList )
{
    camIdList.clear();
    for(auto camId : m_CamIdList )
    {
        bool backFacing = isBackFacing( camId );
        camIdList.emplace_back( std::make_pair(backFacing, camId) );
    }
}

//============================================================================
bool CamJavaClient::isBackFacing( std::string& camId )
{
    if( !g_CamServiceReady )
    {
        LogMsg( LOG_ERROR, "%s !g_CamServiceReady ", __func__ );
        return false;
    }

    if( nullptr == g_CamObj || !HasCamMethod( CAM_IS_BACKFACING_IDX, __func__ ) )
    {
        return false;
    }

    JNIEnv* jniEnv = GetJniEnv();
    if( nullptr == jniEnv )
    {
        LogMsg( LOG_ERROR, "%s failed to get JNI env", __func__ );
        return false;
    }

    jstring jCamId = jniEnv->NewStringUTF(camId.c_str());
    if( nullptr == jCamId )
    {
        LogMsg( LOG_ERROR, "%s NewStringUTF failed", __func__ );
        return false;
    }

    jboolean value = jniEnv->CallBooleanMethod( g_CamObj, g_CamMethods[CAM_IS_BACKFACING_IDX].methodID, jCamId );
    if( jniEnv->ExceptionCheck() )
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
        LogMsg( LOG_ERROR, "%s isCameraBackFacing threw Java exception", __func__ );
        jniEnv->DeleteLocalRef(jCamId);
        return false;
    }

    jniEnv->DeleteLocalRef(jCamId);

    return value ? true : false;
}

//============================================================================
void CamJavaClient::updateCameraList( void )
{
    m_CamIdList.clear();
    if( !g_CamServiceReady )
    {
        LogMsg( LOG_ERROR, "%s !g_CamServiceReady ", __func__ );
        return;
    }

    if( nullptr == g_CamObj || !HasCamMethod( CAM_GET_IDS_IDX, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s invalid camera service binding", __func__ );
        return;
    }

    JNIEnv* jniEnv = GetJniEnv();
    if( nullptr == jniEnv )
    {
        LogMsg( LOG_ERROR, "%s failed to get JNI env", __func__ );
        return;
    }

    jobjectArray stringArray  = (jobjectArray)jniEnv->CallObjectMethod( g_CamObj, g_CamMethods[CAM_GET_IDS_IDX].methodID );
    if( jniEnv->ExceptionCheck() )
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
        LogMsg( LOG_ERROR, "%s getCameraIdList threw Java exception", __func__ );
        return;
    }

    if (stringArray == nullptr) {
        LogMsg( LOG_ERROR, "%s string array in null", __func__ );
        return;
    }

    jsize length = jniEnv->GetArrayLength(stringArray);
    for (int i = 0; i < length; ++i) {
        jstring stringElement = (jstring)jniEnv->GetObjectArrayElement(stringArray, i);
        if( nullptr == stringElement )
        {
            continue;
        }

        const char *charPtr = jniEnv->GetStringUTFChars(stringElement, nullptr);
        if (charPtr != nullptr) {
            m_CamIdList.emplace_back(charPtr);
            jniEnv->ReleaseStringUTFChars(stringElement, charPtr);
        }
        jniEnv->DeleteLocalRef(stringElement);
    }

    jniEnv->DeleteLocalRef(stringArray);

    LogMsg( LOG_VERBOSE, "%s %zu cameras available", __func__, m_CamIdList.size() );
}

//============================================================================
bool CamJavaClient::startCamCapture( std::string camId )
{
    if( !g_CamServiceReady )
    {
        LogMsg( LOG_ERROR, "%s !g_CamServiceReady ", __func__ );
        return false;
    }

    if( camId.empty() )
    {
        LogMsg( LOG_ERROR, "%s camId.empty() ", __func__ );
        return false;
    }

    if( nullptr == g_CamObj || !HasCamMethod( CAM_START_CAPTURE_IDX, __func__ ) )
    {
        return false;
    }

    JNIEnv* jniEnv = GetJniEnv();
    if( nullptr == jniEnv )
    {
        LogMsg( LOG_ERROR, "%s failed to get JNI env", __func__ );
        return false;
    }

    jstring jCamId = jniEnv->NewStringUTF(camId.c_str());
    if( nullptr == jCamId )
    {
        LogMsg( LOG_ERROR, "%s NewStringUTF failed", __func__ );
        return false;
    }

    jboolean value = jniEnv->CallBooleanMethod( g_CamObj, g_CamMethods[CAM_START_CAPTURE_IDX].methodID, jCamId );
    if( jniEnv->ExceptionCheck() )
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
        LogMsg( LOG_ERROR, "%s startCameraCapture threw Java exception", __func__ );
        jniEnv->DeleteLocalRef(jCamId);
        return false;
    }

    jniEnv->DeleteLocalRef(jCamId);

    return value ? true : false;
}

//============================================================================
void CamJavaClient::stopCamCapture( void )
{
    if( !g_CamServiceReady )
    {
        LogMsg( LOG_ERROR, "%s !g_CamServiceReady ", __func__ );
        return;
    }

    if( nullptr == g_CamObj || !HasCamMethod( CAM_STOP_CAPTURE_IDX, __func__ ) )
    {
        return;
    }

    JNIEnv* jniEnv = GetJniEnv();
    if( nullptr == jniEnv )
    {
        LogMsg( LOG_ERROR, "%s failed to get JNI env", __func__ );
        return;
    }

    jniEnv->CallVoidMethod( g_CamObj, g_CamMethods[CAM_STOP_CAPTURE_IDX].methodID  );
    if( jniEnv->ExceptionCheck() )
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
        LogMsg( LOG_ERROR, "%s stopCameraCapture threw Java exception", __func__ );
    }
}

#endif // defined(TARGET_OS_ANDROID)
