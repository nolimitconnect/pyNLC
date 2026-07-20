//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "CamWindows.h"
#include "CamCapture.h"

#ifndef WINVER
#define WINVER 0x0601 // Targets Windows 7 or higher
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>

// 2. Core Media Foundation Headers
#include <mfapi.h>
#include <mfidl.h>

// 3. Source Reader Header (REQUIRED for IMFSourceReader and MFCreateSourceReaderFromMediaSource)
#include <mfreadwrite.h> 
#include <mfobjects.h>

#include <thread>
#include <mutex>
#include <atomic>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfuuid.lib")  // Provides IID_IMFTransform and related Media Foundation interface GUIDs
#pragma comment(lib, "strmiids.lib") // Provides IID_ICodecAPI and other COM interface GUIDs

template <class T> void SafeRelease(T** ppT) {
    if (*ppT) { (*ppT)->Release(); *ppT = NULL; }
}

// Internal implementation details hidden from the user header
class CamWindows::Impl {
public:
    CamCapture&             m_CamCapture;
    std::thread             m_captureThread;
    std::atomic<bool>       m_isRunning{ false };

    Impl(CamCapture& camCapture) : m_CamCapture(camCapture) {
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        MFStartup(MF_VERSION);
    }

    ~Impl() {
        stopThread();
        MFShutdown();
        CoUninitialize();
    }

    void stopThread() {
        m_isRunning = false;
        if (m_captureThread.joinable()) {
            m_captureThread.join();
        }
    }

    // Helper to find a specific device matching the user's string ID/Name
    HRESULT FindDeviceById(const std::string& camId, IMFMediaSource** ppSource) {
        *ppSource = NULL;
        IMFAttributes* pAttributes = NULL;
        IMFActivate** ppDevices = NULL;
        UINT32 count = 0;

        HRESULT hr = MFCreateAttributes(&pAttributes, 1);
        if (SUCCEEDED(hr)) {
            hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        }
        if (SUCCEEDED(hr)) {
            hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
        }

        if (SUCCEEDED(hr)) {
            for (UINT32 i = 0; i < count; i++) {
                WCHAR* pFriendlyName = NULL;
                UINT32 nameLen = 0;
                
                // Read the camera name property
                if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &pFriendlyName, &nameLen))) {
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pFriendlyName, -1, NULL, 0, NULL, NULL);
                    std::string currentName(size_needed - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, pFriendlyName, -1, &currentName[0], size_needed, NULL, NULL);
                    CoTaskMemFree(pFriendlyName);

                    if (currentName == camId && *ppSource == NULL) {
                        hr = ppDevices[i]->ActivateObject(IID_PPV_ARGS(ppSource));
                    }
                }
                SafeRelease(&ppDevices[i]);
            }
        }
        CoTaskMemFree(ppDevices);
        SafeRelease(&pAttributes);
        return (*ppSource) ? S_OK : E_FAIL;
    }

    // This background thread pulls samples continuously from the hardware buffer
    void CaptureLoop(IMFMediaSource* pSource) {
        IMFSourceReader* pReader = NULL;
        IMFAttributes* pReaderAttributes = NULL;
        HRESULT hr = MFCreateAttributes(&pReaderAttributes, 1);
        if (SUCCEEDED(hr)) {
            hr = pReaderAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        }
        if (SUCCEEDED(hr)) {
            hr = MFCreateSourceReaderFromMediaSource(pSource, pReaderAttributes, &pReader);
        }
        SafeRelease(&pReaderAttributes);
        if (FAILED(hr)) {
            SafeRelease(&pSource);
            return;
        }

        // Configure the source reader to downscale to 320x240 RGB24 at 15 FPS.
        IMFMediaType* pType = NULL;
        HRESULT setTypeHr = E_FAIL;
        if (SUCCEEDED(MFCreateMediaType(&pType))) {
            // With this (does the exact same thing using core types):
            pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
            MFSetAttributeSize(pType, MF_MT_FRAME_SIZE, 320, 240);
            MFSetAttributeRatio(pType, MF_MT_FRAME_RATE, 15, 1);
            setTypeHr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
            SafeRelease(&pType);
        }

        if (FAILED(setTypeHr) && SUCCEEDED(MFCreateMediaType(&pType))) {
            pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
            setTypeHr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
            SafeRelease(&pType);
        }

        if (FAILED(setTypeHr) && SUCCEEDED(MFCreateMediaType(&pType))) {
            pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            setTypeHr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
            SafeRelease(&pType);
        }

        IMFMediaType* pCurrentType = NULL;
        UINT32 frameWidth = 0;
        UINT32 frameHeight = 0;
        GUID currentSubtype = GUID_NULL;
        LONG sourceStride = 0;
        int sourceBytesPerPixel = 0;

        if (FAILED(setTypeHr) ||
            FAILED(pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType)) ||
            FAILED(pCurrentType->GetGUID(MF_MT_SUBTYPE, &currentSubtype)) ||
            FAILED(MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &frameWidth, &frameHeight)) ||
            frameWidth == 0 ||
            frameHeight == 0) {
            SafeRelease(&pCurrentType);
            SafeRelease(&pReader);
            SafeRelease(&pSource);
            return;
        }

        if (currentSubtype == MFVideoFormat_RGB24) {
            sourceBytesPerPixel = 3;
        }
        else if (currentSubtype == MFVideoFormat_RGB32) {
            sourceBytesPerPixel = 4;
        }
        else {
            SafeRelease(&pCurrentType);
            SafeRelease(&pReader);
            SafeRelease(&pSource);
            return;
        }

        if (FAILED(pCurrentType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&sourceStride))) {
            LONG defaultStride = 0;
            if (SUCCEEDED(MFGetStrideForBitmapInfoHeader(currentSubtype.Data1, frameWidth, &defaultStride))) {
                sourceStride = defaultStride;
            }
            else {
                sourceStride = (LONG)(frameWidth * sourceBytesPerPixel);
            }
        }
        SafeRelease(&pCurrentType);

        const size_t packedStride = (size_t)frameWidth * 3;
        std::vector<BYTE> localBuffer((size_t)frameWidth * (size_t)frameHeight * 3);

        while (m_isRunning) {
            DWORD streamIndex, flags;
            LONGLONG timestamp;
            IMFSample* pSample = NULL;
            if( m_CamCapture.canProcessCamCapture() == false )
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Synchronously block until a frame drops or thread receives stop signal
            hr = pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timestamp, &pSample);
            if (FAILED(hr)) break;

            if (pSample) {
                IMFMediaBuffer* pMediaBuffer = NULL;
                if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pMediaBuffer))) {
                    BYTE* pRawBuffer = NULL;
                    DWORD currentLength = 0;

                    if (SUCCEEDED(pMediaBuffer->Lock(&pRawBuffer, NULL, &currentLength))) {
                        bool copiedFrame = false;
                        const size_t absSourceStride = (size_t)(sourceStride < 0 ? -sourceStride : sourceStride);
                        const size_t minLengthNeeded = absSourceStride * (size_t)frameHeight;

                        if (packedStride <= absSourceStride && currentLength >= minLengthNeeded) {
                            const BYTE* sourceRow = pRawBuffer;
                            intptr_t rowStep = sourceStride;
                            if (sourceStride < 0) {
                                sourceRow = pRawBuffer + ((size_t)frameHeight - 1) * absSourceStride;
                            }

                            BYTE* destRow = localBuffer.data();
                            for (UINT32 row = 0; row < frameHeight; ++row) {
                                if (sourceBytesPerPixel == 3) {
                                    const BYTE* srcPixel = sourceRow;
                                    BYTE* dstPixel = destRow;
                                    for (UINT32 col = 0; col < frameWidth; ++col) {
                                        // Media Foundation RGB24 buffers are byte-ordered B, G, R.
                                        dstPixel[0] = srcPixel[2];
                                        dstPixel[1] = srcPixel[1];
                                        dstPixel[2] = srcPixel[0];
                                        srcPixel += 3;
                                        dstPixel += 3;
                                    }
                                }
                                else {
                                    const BYTE* srcPixel = sourceRow;
                                    BYTE* dstPixel = destRow;
                                    for (UINT32 col = 0; col < frameWidth; ++col) {
                                        // Media Foundation RGB32 buffers are byte-ordered B, G, R, A.
                                        dstPixel[0] = srcPixel[2];
                                        dstPixel[1] = srcPixel[1];
                                        dstPixel[2] = srcPixel[0];
                                        srcPixel += 4;
                                        dstPixel += 3;
                                    }
                                }
                                destRow += packedStride;
                                sourceRow = sourceRow + rowStep;
                            }
                            copiedFrame = true;
                        }
                        pMediaBuffer->Unlock();

                        if (copiedFrame) {
                            std::shared_ptr<uint8_t> frameData(new uint8_t[localBuffer.size()], std::default_delete<uint8_t[]>());
                            memcpy(frameData.get(), localBuffer.data(), localBuffer.size());
                            m_CamCapture.getCamProcessor().processCamCapture((int)frameWidth, (int)frameHeight, frameData, static_cast<int>(localBuffer.size()));
                        }
                    }
                    SafeRelease(&pMediaBuffer);
                }
                SafeRelease(&pSample);
            }
            // Throttling step to respect our 15 FPS goal (~66ms frame window)
            std::this_thread::sleep_for(std::chrono::milliseconds(66));
        }

        SafeRelease(&pReader);
        SafeRelease(&pSource);
    }
};

// --- Public Wrapper Implementations ---
//============================================================================
CamWindows::CamWindows(CamCapture& camCapture) : pImpl(std::make_unique<Impl>(camCapture)) {}
CamWindows::~CamWindows() = default;

//============================================================================
bool CamWindows::isCamCaptureRunning( void ) 
{
    return pImpl->m_isRunning;
}

//============================================================================
void CamWindows::getCamCaptureDevices(std::vector<std::string>& retCamList) {
    retCamList.clear();
    IMFAttributes* pAttributes = NULL;
    IMFActivate** ppDevices = NULL;
    UINT32 count = 0;

    if (FAILED(MFCreateAttributes(&pAttributes, 1))) return;
    pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    if (SUCCEEDED(MFEnumDeviceSources(pAttributes, &ppDevices, &count))) {
        for (UINT32 i = 0; i < count; i++) {
            WCHAR* pFriendlyName = NULL;
            UINT32 nameLen = 0;
            if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &pFriendlyName, &nameLen))) {
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, pFriendlyName, -1, NULL, 0, NULL, NULL);
                std::string name(size_needed - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, pFriendlyName, -1, &name[0], size_needed, NULL, NULL);
                retCamList.push_back(name);
                CoTaskMemFree(pFriendlyName);
            }
            SafeRelease(&ppDevices[i]);
        }
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pAttributes);
}

//============================================================================
bool CamWindows::cameraExists(std::string camId) {
    std::vector<std::string> cameras;
    getCamCaptureDevices(cameras);
    for (const auto& name : cameras) {
        if (name == camId) return true;
    }
    return false;
}

//============================================================================
bool CamWindows::startCamCapture(std::string camId) {
    // Prevent double-initialization
    if (pImpl->m_isRunning) return false;

    IMFMediaSource* pSource = NULL;
    if (FAILED(pImpl->FindDeviceById(camId, &pSource))) {
        return false;
    }

    pImpl->m_isRunning = true;
    // Hand over the activated COM resource pointer to the background processing loop thread
    pImpl->m_captureThread = std::thread(&Impl::CaptureLoop, pImpl.get(), pSource);
    return true;
}

//============================================================================
void CamWindows::stopCamCapture(void) {
    pImpl->stopThread();
}
