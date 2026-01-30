#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <string>
#include <cstdint>

/**
 * AudioCapture uses Windows WASAPI to capture the default 
 * microphone/input device.
 */
class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool Initialize(bool isLoopback = false);
    bool Start();
    
    // Returns the Windows "Friendly Name" of the mic
    std::string GetDeviceName() const;

    // Reads captured audio samples into the buffer
    bool GetAudioSamples(std::vector<int16_t>& outSamples);
    
    void Stop();
    void Cleanup();

private:
    IMMDeviceEnumerator* m_enumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;
    
    WAVEFORMATEX* m_pwfx = nullptr;
    bool m_initialized = false;
};
