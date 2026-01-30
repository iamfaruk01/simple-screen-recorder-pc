#pragma once

#include <windows.h>
#include <string>
#include <functional>
#include <vector>
#include "WebcamDevice.hpp"

/**
 * Controller manages the Win32 UI window and user interactions.
 */
class Controller {
public:
    struct Settings {
        int width = 0;
        int height = 0;
        bool recordAudio = true;
        bool useSystemAudio = false; // false = Mic, true = System
        bool showHighlight = true;
        bool showCursor = true;
        bool useCountdown = true;
        bool useFloatingBar = true;
        bool useWebcam = false;
        bool useCustomArea = false;
        RECT customRegion = {0}; 
        POINT webcamPos = {0, 0}; // Screen coordinates of webcam preview
    };

    Controller();
    ~Controller();

    bool Create();
    void Run(); // Starts the UI message loop
    
    Settings GetSettings() const { return m_settings; }
    std::string GetSavePath() const { return m_savePath; }

    // Callbacks to talk back to our recording engine
    void SetOnStartCallback(std::function<void()> callback) { m_onStart = callback; }
    void SetOnStopCallback(std::function<void()> callback) { m_onStop = callback; }
    void SetOnPauseCallback(std::function<bool(bool)> callback) { m_onPause = callback; } // Returns success

    // Update the live preview from external source (e.g. RecordingThread)
    void SetPreviewFrame(const std::vector<uint8_t>& frame, int w, int h);
    void SetWebcamEnabled(bool enabled);

    HWND GetWebcamPreviewWindow() const { return m_hwndWebcamPreview; }
    HWND GetWindowHandle() const { return m_hwnd; }
    bool IsRecording() const { return m_isRecording; }
    bool IsPaused() const { return m_isPaused; }

    friend LRESULT CALLBACK WebcamPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend LRESULT CALLBACK CaptureIndicatorWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HWND m_hwnd = nullptr;
    HWND m_btnStart = nullptr;
    HWND m_btnPause = nullptr; // New Pause Button
    HWND m_comboRes = nullptr;
    HWND m_checkAudio = nullptr;
    HWND m_checkHighlight = nullptr;
    HWND m_checkCursor = nullptr;
    HWND m_checkCountdown = nullptr;
    HWND m_checkFloating = nullptr;
    HWND m_checkWebcam = nullptr;
    HWND m_comboArea = nullptr;
    HWND m_hwndWebcamPreview = nullptr;
    HWND m_hwndCaptureIndicator = nullptr;
    HWND m_labelTimer = nullptr;
    HWND m_labelVideo = nullptr;
    HWND m_labelMouse = nullptr;
    HWND m_labelF9Hint = nullptr;
    HWND m_btnShowFolder = nullptr;
    HWND m_labelSavePath = nullptr;
    HWND m_labelCaptureArea = nullptr;

    WebcamDevice m_webcamPreview;
    std::vector<uint8_t> m_previewFrame;
    int m_previewW = 0;
    int m_previewH = 0;

    bool m_isRecording = false;
    bool m_isPaused = false;
    bool m_isCountingDown = false;
    int m_countdownValue = 0;
    DWORD m_startTime = 0;
    DWORD m_pauseStartTime = 0;
    DWORD m_totalPausedTime = 0;
    Settings m_settings;

    std::function<void()> m_onStart;
    std::function<void()> m_onStop;
    std::function<bool(bool)> m_onPause;

    void UpdateButtonState();
    void StartCountdown();
    void UpdateTimer();
    void SwitchToFloatingBar(bool floating);
    void Relayout(int width, int height);
    void ToggleWebcamPreview(bool show);
    void CreateWebcamPreviewWindow();

    void CreateCaptureIndicatorWindow();
    void ToggleCaptureIndicator(bool show, RECT region);
    void UpdateCustomRegionFromIndicator(RECT windowRect);

    RECT m_originalRect = {0};
    bool m_isFloating = false;
    std::string m_savePath;
};
