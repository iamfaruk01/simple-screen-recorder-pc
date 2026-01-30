#pragma once

#include <string>
#include <cstdio>
#include <vector>
#include <cstdint>

/**
 * VideoEncoder handles the live piping of raw pixel data
 * into FFmpeg to produce an MP4 file.
 */
class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();
    
    bool Start(const std::string& outputPath, int sourceWidth, int sourceHeight, int fps, 
               const std::string& audioDeviceName = "", bool isSystemAudio = false,
               int targetWidth = 0, int targetHeight = 0);
    bool WriteFrame(const std::vector<uint8_t>& bgraData);
    void Finish();

private:
    void* m_ffmpegPipe = nullptr; // Windows HANDLE
    int m_width = 0;
    int m_height = 0;
    bool m_isRunning = false;

    std::string FindFFmpeg();
};
