#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>
#include "ScreenCapture.hpp"
#include "VideoEncoder.hpp"
#include "VisualEffects.hpp"
#include "AudioCapture.hpp"
#include "Controller.hpp"
#include <filesystem>
#include <string>
#include "WebcamDevice.hpp"

// Global state
std::atomic<bool> g_isRecording(false);
std::atomic<bool> g_isPaused(false);
std::atomic<bool> g_shouldExit(false);
Controller::Settings g_currentSettings;
std::string g_saveDirectory = ".";
Controller* g_uiPtr = nullptr;

namespace fs = std::filesystem;

std::string GetNextRecordingFilename() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "recording_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".mp4";
    
    fs::path dir(g_saveDirectory);
    fs::path fullPath = dir / ss.str();
    
    return fullPath.string();
}

/**
 * Helper to composite webcam frame over screen frame
 */
void CompositeWebcam(std::vector<uint8_t>& screenBuf, int sW, int sH, 
                     const std::vector<uint8_t>& webBuf, int wW, int wH,
                     POINT webcamPos, RECT customRegion) {
    if (webBuf.empty()) return;

    // Fixed PIP size: 20% of screen height
    int targetH = sH / 5;
    int targetW = (int)((float)wW / wH * targetH);
    
    // Position: Convert screen coordinates to relative capture coordinates
    // If customRegion is full screen (0,0,0,0), then left/top are 0.
    int startX = webcamPos.x - customRegion.left;
    int startY = webcamPos.y - customRegion.top;

    // Basic Nearest Neighbor Downscaling for simplicity & speed
    for (int y = 0; y < targetH; y++) {
        for (int x = 0; x < targetW; x++) {
            // Screen coordinates
            int screenX = startX + x;
            int screenY = startY + y;

            if (screenX < 0 || screenY < 0 || screenX >= sW || screenY >= sH) continue;

            // Map to webcam coordinates
            int srcX = x * wW / targetW;
            int srcY = y * wH / targetH;

            int dstIdx = (screenY * sW + screenX) * 4;
            int srcIdx = (srcY * wW + srcX) * 4;

            // Direct copy without alpha-blending for speed
            uint8_t* d = &screenBuf[dstIdx];
            const uint8_t* s = &webBuf[srcIdx];
            d[0] = s[0]; // B
            d[1] = s[1]; // G
            d[2] = s[2]; // R
            d[3] = 255;  // A
        }
    }
}

/**
 * The Recording Engine Thread
 */
void RecordingThread() {
    ScreenCapture capture;
    VideoEncoder encoder;
    AudioCapture audio;
    WebcamDevice webcam;

    if (!capture.Initialize()) {
        std::cerr << "Capture initialization failed!" << std::endl;
        return;
    }

    std::vector<uint8_t> frameBuffer;
    int screenWidth, screenHeight;
    int fps = 30;
    auto frameDuration = std::chrono::microseconds(1000000 / fps);

    while (!g_shouldExit) {
        if (g_isRecording) {
            std::string outputPath = GetNextRecordingFilename();
            std::cout << "\nStarting Recording: " << outputPath << std::endl;

            // 1. Setup Audio
            std::string micName = "";
            if (g_currentSettings.recordAudio) {
                if (audio.Initialize(g_currentSettings.useSystemAudio)) {
                    micName = g_currentSettings.useSystemAudio ? "" : audio.GetDeviceName();
                    audio.Start();
                }
            }

            // 1.5 Setup Webcam
            if (g_currentSettings.useWebcam) {
                if (webcam.Initialize(0)) {
                    webcam.Start();
                }
            }

            // 2. Start Encoder
            capture.SetRegion(g_currentSettings.customRegion);
            capture.CaptureFrame(frameBuffer, screenWidth, screenHeight);
            if (!encoder.Start(outputPath, screenWidth, screenHeight, fps, 
                          micName, g_currentSettings.useSystemAudio,
                          g_currentSettings.width, g_currentSettings.height)) {
                std::cerr << "Failed to start Video Encoder!" << std::endl;
                g_isRecording = false;
                if (g_currentSettings.recordAudio) audio.Stop();
                if (g_currentSettings.useWebcam) webcam.Stop();
                audio.Cleanup();
                webcam.Cleanup();
                continue;
            }

            int frameCount = 0;
            auto startTime = std::chrono::steady_clock::now();

            while (g_isRecording) {
                auto nextFrameTime = startTime + (frameDuration * (frameCount + 1));

                if (g_isPaused) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    startTime += std::chrono::milliseconds(10);
                    continue;
                }

                // Try to acquire next frame. If it fails (no screen change), 
                // we'll reuse the previous frameBuffer to maintain steady FPS.
                int capturedW, capturedH;
                capture.CaptureFrame(frameBuffer, capturedW, capturedH);

                // Effects
                if (g_currentSettings.showHighlight || g_currentSettings.showCursor) {
                    POINT mousePos = VisualEffects::GetMousePosition();
                    
                    // OFFSET mouse position relative to the captured area start
                    POINT origin = capture.GetCaptureOrigin();
                    mousePos.x -= origin.x;
                    mousePos.y -= origin.y;

                    if (g_currentSettings.showHighlight) {
                        bool isClicked = VisualEffects::IsLeftClicked();
                        VisualEffects::Color color = isClicked ? VisualEffects::Color{255, 0, 0, 150} : VisualEffects::Color{255, 255, 0, 100};
                        VisualEffects::DrawHighlight(frameBuffer, screenWidth, screenHeight, mousePos, isClicked ? 30 : 25, color);
                    }
                    if (g_currentSettings.showCursor) {
                        VisualEffects::DrawCursor(frameBuffer, screenWidth, screenHeight, mousePos);
                    }
                }

                // Webcam
                if (g_currentSettings.useWebcam) {
                    // Dynamic update of webcam position if window is moved
                    if (g_uiPtr && g_uiPtr->GetWebcamPreviewWindow()) {
                        RECT rc;
                        GetWindowRect(g_uiPtr->GetWebcamPreviewWindow(), &rc);
                        g_currentSettings.webcamPos.x = rc.left;
                        g_currentSettings.webcamPos.y = rc.top;
                    }

                    std::vector<uint8_t> webFrame;
                    int wW, wH;
                    if (webcam.GetFrame(webFrame, wW, wH)) {
                        CompositeWebcam(frameBuffer, screenWidth, screenHeight, webFrame, wW, wH, 
                                       g_currentSettings.webcamPos, g_currentSettings.customRegion);
                        if (g_uiPtr) g_uiPtr->SetPreviewFrame(webFrame, wW, wH);
                    }
                }

                encoder.WriteFrame(frameBuffer);
                frameCount++;

                std::this_thread::sleep_until(nextFrameTime);
            }

            if (g_currentSettings.recordAudio) audio.Stop();
            if (g_currentSettings.useWebcam) webcam.Stop();
            encoder.Finish();
            audio.Cleanup();
            webcam.Cleanup();
            std::cout << "\nRecording saved." << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    capture.Cleanup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    Controller ui;
    g_uiPtr = &ui;

    ui.SetOnStartCallback([&ui]() {
        g_currentSettings = ui.GetSettings();
        g_saveDirectory = ui.GetSavePath();
        g_isRecording = true;
    });

    ui.SetOnStopCallback([]() {
        g_isRecording = false;
        g_isPaused = false;
    });

    ui.SetOnPauseCallback([](bool paused) {
        g_isPaused = paused;
        return true;
    });

    std::thread engine(RecordingThread);

    if (!ui.Create()) {
        std::cerr << "Failed to create UI!" << std::endl;
        g_shouldExit = true;
        engine.join();
        return -1;
    }

    ui.Run();

    g_shouldExit = true;
    g_isRecording = false;
    engine.join();

    return 0;
}
