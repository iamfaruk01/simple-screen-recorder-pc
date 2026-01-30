#include "WebcamDevice.hpp"
#include <iostream>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")

template <class T> void SafeRelease(T **ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

WebcamDevice::WebcamDevice() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    MFStartup(MF_VERSION);
}

WebcamDevice::~WebcamDevice() {
    Cleanup();
    MFShutdown();
    CoUninitialize();
}

void WebcamDevice::Cleanup() {
    Stop();
    if (m_pReader) {
        m_pReader.Reset();
    }
    m_initialized = false;
}

bool WebcamDevice::Initialize(int deviceIndex) {
    Cleanup();

    IMFAttributes* pAttributes = NULL;
    IMFActivate** ppDevices = NULL;
    UINT32 count = 0;

    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return false;

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) { SafeRelease(&pAttributes); return false; }

    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr) || count == 0) {
        SafeRelease(&pAttributes);
        return false;
    }

    if (deviceIndex >= (int)count) deviceIndex = 0;

    IMFMediaSource* pSource = NULL;
    hr = ppDevices[deviceIndex]->ActivateObject(__uuidof(IMFMediaSource), (void**)&pSource);
    
    // Cleanup list
    for (DWORD i = 0; i < count; i++) SafeRelease(&ppDevices[i]);
    CoTaskMemFree(ppDevices);
    SafeRelease(&pAttributes);

    if (FAILED(hr)) return false;

    if (SetupSourceReader(pSource)) {
        m_initialized = true;
        SafeRelease(&pSource);
        return true;
    }

    SafeRelease(&pSource);
    return false;
}

bool WebcamDevice::SetupSourceReader(IMFMediaSource* pSource) {
    IMFAttributes* pAttributes = NULL;
    MFCreateAttributes(&pAttributes, 1);
    pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

    HRESULT hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &m_pReader);
    SafeRelease(&pAttributes);
    if (FAILED(hr)) return false;

    // Select first stream
    hr = NormalizeFormat(NULL); 
    
    // Get actual format
    IMFMediaType* pType = NULL;
    hr = m_pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType);
    if (SUCCEEDED(hr)) {
        MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, (UINT32*)&m_width, (UINT32*)&m_height);
        SafeRelease(&pType);
    }

    return true;
}

HRESULT WebcamDevice::NormalizeFormat(IMFMediaType* pType) {
    // Force RGB32 (X8R8G8B8) or ARGB32 for easy compositing
    IMFMediaType* pNativeType = NULL;
    HRESULT hr = MFCreateMediaType(&pNativeType);
    hr = pNativeType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    hr = pNativeType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    
    hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pNativeType);
    SafeRelease(&pNativeType);
    return hr;
}

void WebcamDevice::Start() {
    if (!m_initialized || m_isStreaming) return;
    m_isStreaming = true;
    m_stopWorker = false;
    m_worker = std::thread(&WebcamDevice::CaptureLoop, this);
}

void WebcamDevice::Stop() {
    if (!m_isStreaming) return;
    m_isStreaming = false;
    m_stopWorker = true;
    if (m_worker.joinable()) m_worker.join();
}

bool WebcamDevice::GetFrame(std::vector<uint8_t>& outBuffer, int& width, int& height) {
    if (!m_initialized) return false;

    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (!m_lastFrame.empty()) {
        width = m_width;
        height = m_height;
        outBuffer = m_lastFrame;
        return true;
    }

    return false;
}

void WebcamDevice::CaptureLoop() {
    while (!m_stopWorker) {
        DWORD streamIndex, flags;
        LONGLONG llTimeStamp;
        IMFSample* pSample = NULL;

        HRESULT hr = m_pReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &streamIndex, &flags, &llTimeStamp, &pSample
        );

        if (FAILED(hr)) break;

        if (pSample) {
            IMFMediaBuffer* pBuffer = NULL;
            if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer))) {
                BYTE* pData = NULL;
                DWORD cbLength = 0;
                if (SUCCEEDED(pBuffer->Lock(&pData, NULL, &cbLength))) {
                    {
                        std::lock_guard<std::mutex> lock(m_frameMutex);
                        m_lastFrame.assign(pData, pData + cbLength);
                    }
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        } else {
            // No sample? Wait a bit to avoid CPU spin
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
