#include "VideoEncoder.hpp"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <Windows.h>

VideoEncoder::VideoEncoder() : m_ffmpegPipe(nullptr), m_isRunning(false), m_width(0), m_height(0) {}

VideoEncoder::~VideoEncoder() {
    Finish();
}

std::string VideoEncoder::FindFFmpeg() {
    // 1. Check same directory as the executable (for portable distribution)
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        std::filesystem::path localFFmpeg = exeDir / "ffmpeg.exe";
        if (std::filesystem::exists(localFFmpeg)) {
            return localFFmpeg.make_preferred().string();
        }
    }
    
    // 2. Check development path
    std::filesystem::path p1 = "D:/projects/screen-recorder/node_modules/ffmpeg-static/ffmpeg.exe";
    if (std::filesystem::exists(p1)) return p1.make_preferred().string();
    
    // 3. Fall back to system PATH
    return "ffmpeg.exe";
}

bool VideoEncoder::Start(const std::string& outputPath, int sourceWidth, int sourceHeight, int fps, 
                         const std::string& audioDeviceName, bool isSystemAudio, 
                         int targetWidth, int targetHeight) {
    if (m_isRunning) return false;

    m_width = sourceWidth;
    m_height = sourceHeight;

    std::string ffmpegPath = FindFFmpeg();
    
    // BUILD THE FFMPEG COMMAND
    std::stringstream cmd;
    cmd << "\"" << "\"" << ffmpegPath << "\""
        << " -loglevel warning"
        << " -thread_queue_size 2048 -f rawvideo -pixel_format bgra"
        << " -video_size " << sourceWidth << "x" << sourceHeight
        << " -framerate " << fps
        << " -i - "; // Input 1: Video Pipe

    if (!audioDeviceName.empty()) {
        if (isSystemAudio) {
            cmd << " -thread_queue_size 2048 -f wasapi -i \"audio=" << audioDeviceName << "\" ";
        } else {
            cmd << " -thread_queue_size 2048 -f dshow -i audio=\"" << audioDeviceName << "\" ";
        }
    }

    // Apply Scaling Filters
    // Apply Scaling Filters
    // Use -2 for width/height to tell FFmpeg to maintain aspect ratio while keeping even dimensions
    if (targetWidth != 0 || targetHeight != 0) {
        std::string wStr = (targetWidth <= 0) ? "-2" : std::to_string(targetWidth);
        std::string hStr = (targetHeight <= 0) ? "-2" : std::to_string(targetHeight);
        
        cmd << " -vf \"scale=" << wStr << ":" << hStr << ":flags=bicubic\" ";
    } else {
        // Native resolution: Just ensure even dimensions for H.264
        cmd << " -vf \"scale=trunc(iw/2)*2:trunc(ih/2)*2\" ";
    }

    // Encoding Settings
    cmd << " -c:v libx264 -preset ultrafast -crf 23"
        << " -c:a aac -b:a 192k" 
        << " -pix_fmt yuv420p" 
        << " -shortest" 
        << " -y " 
        << "\"" << outputPath << "\"" << "\"";

    std::cout << "Starting FFmpeg pipe: " << cmd.str() << std::endl;

    m_ffmpegPipe = _popen(cmd.str().c_str(), "wb");
    if (!m_ffmpegPipe) return false;

    m_isRunning = true;
    return true;
}

bool VideoEncoder::WriteFrame(const std::vector<uint8_t>& bgraData) {
    if (!m_isRunning || !m_ffmpegPipe) return false;
    size_t written = fwrite(bgraData.data(), 1, bgraData.size(), m_ffmpegPipe);
    return written == bgraData.size();
}

void VideoEncoder::Finish() {
    if (m_ffmpegPipe) {
        _pclose(m_ffmpegPipe);
        m_ffmpegPipe = nullptr;
    }
    m_isRunning = false;
}
