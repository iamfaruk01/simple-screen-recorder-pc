#include "AudioCapture.hpp"
#include <iostream>
#include <comdef.h>
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "Ole32.lib")

AudioCapture::AudioCapture() {}

AudioCapture::~AudioCapture() {
    Cleanup();
}

bool AudioCapture::Initialize(bool isLoopback) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    
    // 1. Get Device Enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, 
                          __uuidof(IMMDeviceEnumerator), (void**)&m_enumerator);
    if (FAILED(hr)) return false;

    // 2. Get Default Audio Endpoint (Input for Mic, Output for System)
    EDataFlow flow = isLoopback ? eRender : eCapture;
    hr = m_enumerator->GetDefaultAudioEndpoint(flow, eConsole, &m_device);
    if (FAILED(hr)) return false;

    // 3. Activate Audio Client
    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr)) return false;

    // 4. Get Mix Format
    hr = m_audioClient->GetMixFormat(&m_pwfx);
    if (FAILED(hr)) return false;

    // 5. Initialize Client (Add Loopback flag if needed)
    DWORD flags = isLoopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 10000000, 0, m_pwfx, nullptr);
    if (FAILED(hr)) return false;

    // 6. Get Capture Client
    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
    if (FAILED(hr)) return false;

    m_initialized = true;
    std::cout << "Audio initialized: " << m_pwfx->nSamplesPerSec << "Hz, " 
              << m_pwfx->nChannels << " channels" << std::endl;
    
    return true;
}

std::string AudioCapture::GetDeviceName() const {
    if (!m_device) return "default";

    IPropertyStore* pProps = nullptr;
    HRESULT hr = m_device->OpenPropertyStore(STGM_READ, &pProps);
    if (FAILED(hr)) return "default";

    PROPVARIANT varName;
    PropVariantInit(&varName);

    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
    std::string name = "default";
    if (SUCCEEDED(hr)) {
        std::wstring wname = varName.pwszVal;
        name = std::string(wname.begin(), wname.end());
    }

    PropVariantClear(&varName);
    pProps->Release();
    return name;
}

bool AudioCapture::Start() {
    if (!m_initialized) return false;
    return SUCCEEDED(m_audioClient->Start());
}

bool AudioCapture::GetAudioSamples(std::vector<int16_t>& outSamples) {
    if (!m_captureClient) return false;

    UINT32 packetLength = 0;
    HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
    
    outSamples.clear();

    while (packetLength != 0) {
        BYTE* pData;
        UINT32 numFramesAvailable;
        DWORD flags;

        hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        if (FAILED(hr)) break;

        // Convert the raw data to 16-bit samples
        // Note: m_pwfx->wBitsPerSample might be 32 (Float), we convert to 16-bit for FFmpeg
        int bytesPerFrame = m_pwfx->nBlockAlign;
        int channels = m_pwfx->nChannels;

        for (UINT32 i = 0; i < numFramesAvailable; ++i) {
            float* pFloatData = (float*)(pData + (i * bytesPerFrame));
            // Mix channels to Mono or keep Stereo? Let's just take the first channel for now or simple mix
            for(int c=0; c < channels; ++c) {
                 // WASAPI shared mode is usually float. Convert float to int16
                 float sample = ((float*)pData)[i * channels + c];
                 int16_t s16 = (int16_t)(sample * 32767.0f);
                 outSamples.push_back(s16);
            }
        }

        hr = m_captureClient->ReleaseBuffer(numFramesAvailable);
        hr = m_captureClient->GetNextPacketSize(&packetLength);
    }

    return !outSamples.empty();
}

void AudioCapture::Stop() {
    if (m_audioClient) m_audioClient->Stop();
}

void AudioCapture::Cleanup() {
    if (m_captureClient) m_captureClient->Release();
    if (m_audioClient) m_audioClient->Release();
    if (m_device) m_device->Release();
    if (m_enumerator) m_enumerator->Release();
    if (m_pwfx) CoTaskMemFree(m_pwfx);
    CoUninitialize();
}
