#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <vector>
#include <wrl/client.h>
#include <thread>
#include <mutex>
#include <atomic>

using Microsoft::WRL::ComPtr;

class WebcamDevice {
public:
    WebcamDevice();
    ~WebcamDevice();

    bool Initialize(int deviceIndex = 0);
    void Start();
    void Stop();
    
    // Reads the latest frame into BGRA buffer
    // Returns true if a new frame was acquired
    bool GetFrame(std::vector<uint8_t>& outBuffer, int& width, int& height);

    void Cleanup();

private:
    ComPtr<IMFSourceReader> m_pReader;
    bool m_initialized = false;
    bool m_isStreaming = false;
    int m_width = 0;
    int m_height = 0;

    std::vector<uint8_t> m_lastFrame; // Cache last frame for steady composite
    std::mutex m_frameMutex;
    std::thread m_worker;
    std::atomic<bool> m_stopWorker{false};
    
    void CaptureLoop();
    bool SetupSourceReader(IMFMediaSource* pSource);
    HRESULT NormalizeFormat(IMFMediaType* pType);
};
