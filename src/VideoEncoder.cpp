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
        
        // Check dist folder if running from build
        std::filesystem::path distFFmpeg = exeDir / ".." / "dist" / "SimpleScreenRecorder" / "ffmpeg.exe";
        if (std::filesystem::exists(distFFmpeg)) {
            return distFFmpeg.make_preferred().string();
        }
    }
    
    // 2. Check development path in project root
    std::filesystem::path p1 = "d:/projects/simple-screen-recorder/simple-screen-recorder-pc/dist/SimpleScreenRecorder/ffmpeg.exe";
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
    cmd << "\"" << ffmpegPath << "\""
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

    if (targetWidth != 0 || targetHeight != 0) {
        std::string wStr = (targetWidth <= 0) ? "-2" : std::to_string(targetWidth);
        std::string hStr = (targetHeight <= 0) ? "-2" : std::to_string(targetHeight);
        cmd << " -vf \"scale=" << wStr << ":" << hStr << ":flags=bicubic\" ";
    } else {
        cmd << " -vf \"scale=trunc(iw/2)*2:trunc(ih/2)*2\" ";
    }

    cmd << " -c:v libx264 -preset ultrafast -crf 23"
        << " -c:a aac -b:a 192k" 
        << " -pix_fmt yuv420p" 
        << " -shortest" 
        << " -y " 
        << "\"" << outputPath << "\"";

    std::string cmdStr = cmd.str();
    std::cout << "Starting FFmpeg: " << cmdStr << std::endl;

    // --- Modern Win32 Process Implementation (Hides Console) ---
    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0)) return false;
    SetHandleInformation(hPipeWrite, HANDLE_FLAG_INHERIT, 0); // Don't inherit write end

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = hPipeRead;
    si.hStdOutput = NULL; // We don't need stdout
    si.hStdError = NULL;  // We don't need stderr
    si.wShowWindow = SW_HIDE; // HIDDEN!

    PROCESS_INFORMATION pi = { 0 };
    BOOL success = CreateProcessA(NULL, (LPSTR)cmdStr.c_str(), NULL, NULL, TRUE, 
                                  CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!success) {
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return false;
    }

    CloseHandle(hPipeRead); // Child has its end
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    m_ffmpegPipe = (void*)hPipeWrite;
    m_isRunning = true;
    return true;
}

bool VideoEncoder::WriteFrame(const std::vector<uint8_t>& bgraData) {
    if (!m_isRunning || !m_ffmpegPipe) return false;
    DWORD written;
    BOOL success = WriteFile((HANDLE)m_ffmpegPipe, bgraData.data(), (DWORD)bgraData.size(), &written, NULL);
    return success && written == bgraData.size();
}

void VideoEncoder::Finish() {
    if (m_ffmpegPipe) {
        CloseHandle((HANDLE)m_ffmpegPipe);
        m_ffmpegPipe = nullptr;
    }
    m_isRunning = false;
}
