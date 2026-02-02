#include "Controller.hpp"
#include <objbase.h>
#include <dwmapi.h> // For modern Windows attributes
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#include <uxtheme.h>
#include <shlobj.h>
#include <filesystem>
#include "RegionSelector.hpp"
#include "resource.h"

Controller::Controller() {
    char path[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, path, CSIDL_MYVIDEO, TRUE)) {
        m_savePath = path;
        m_savePath += "\\Simple Screen Recorder";
        // Ensure directory exists
        std::filesystem::create_directories(m_savePath);
    } else {
        m_savePath = ".";
    }
}

Controller::~Controller() {}

bool Controller::Create() {
    const char CLASS_NAME[] = "ScreenRecorderController";

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(35, 35, 38)); // Darker, cleaner background
    wc.hIcon         = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm       = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClassEx(&wc);

    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        CLASS_NAME,
        "Simple Screen Recorder",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 980,
        NULL, NULL, GetModuleHandle(NULL), this
    );

    if (m_hwnd == NULL) return false;

    // Enable Dark Mode Title Bar (Windows 10/11)
    BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);

    // Register Hotkey: F9
    RegisterHotKey(m_hwnd, 1, 0, VK_F9);

    // Create a Modern Font (Segoe UI)
    // Create a Modern Font (Segoe UI) - Increased size
    HFONT hFont = CreateFont(-22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                             DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    int margin = 30;
    int y = 30;

    // --- Video Quality ---
    // Increased height for label (30) and moved x pos
    m_labelVideo = CreateWindow("STATIC", "VIDEO QUALITY", WS_VISIBLE | WS_CHILD, margin, y, 180, 30, m_hwnd, NULL, NULL, NULL);
    SendMessage(m_labelVideo, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Moved x to 220 to avoid overlap, increased width/height
    m_comboRes = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 220, y-5, 200, 300, m_hwnd, (HMENU)2, NULL, NULL);
    SendMessage(m_comboRes, CB_ADDSTRING, 0, (LPARAM)"Native Resolution");
    SendMessage(m_comboRes, CB_ADDSTRING, 0, (LPARAM)"Full HD (1080p)");
    SendMessage(m_comboRes, CB_ADDSTRING, 0, (LPARAM)"HD (720p)");
    SendMessage(m_comboRes, CB_ADDSTRING, 0, (LPARAM)"Standard (480p)");
    SendMessage(m_comboRes, CB_SETCURSEL, 2, 0); // Default to 720p (Index 2)
    SendMessage(m_comboRes, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 70;
    // --- Capture Area ---
    m_labelCaptureArea = CreateWindow("STATIC", "CAPTURE AREA", WS_VISIBLE | WS_CHILD, margin, y, 180, 30, m_hwnd, NULL, NULL, NULL);
    SendMessage(m_labelCaptureArea, WM_SETFONT, (WPARAM)hFont, TRUE);

    m_comboArea = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 220, y - 5, 200, 300, m_hwnd, (HMENU)15, NULL, NULL);
    SendMessage(m_comboArea, CB_ADDSTRING, 0, (LPARAM)"Full Screen");
    SendMessage(m_comboArea, CB_ADDSTRING, 0, (LPARAM)"Custom Area");
    SendMessage(m_comboArea, CB_SETCURSEL, 0, 0); // Default to Full Screen
    SendMessage(m_comboArea, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 85;
    // --- Audio Settings ---
    m_checkAudio = CreateWindow("BUTTON", "Record Audio", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, margin, y, 300, 35, m_hwnd, (HMENU)3, NULL, NULL);
    SendMessage(m_checkAudio, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(m_checkAudio, WM_SETFONT, (WPARAM)hFont, TRUE);

    /*
    y += 45;
    m_checkWebcam = CreateWindow("BUTTON", "Record Webcam", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, margin, y, 300, 35, m_hwnd, (HMENU)12, NULL, NULL);
    SendMessage(m_checkWebcam, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(m_checkWebcam, WM_SETFONT, (WPARAM)hFont, TRUE);
    */

    y += 45;
    // --- Countdown Option ---
    m_checkCountdown = CreateWindow("BUTTON", "3s Countdown", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, margin, y, 300, 35, m_hwnd, (HMENU)8, NULL, NULL);
    SendMessage(m_checkCountdown, BM_SETCHECK, BST_UNCHECKED, 0); // Default to unselected
    SendMessage(m_checkCountdown, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 45;
    // --- Floating Bar Option ---
    m_checkFloating = CreateWindow("BUTTON", "Use Floating Bar", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, margin, y, 300, 35, m_hwnd, (HMENU)11, NULL, NULL);
    SendMessage(m_checkFloating, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_checkFloating, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 70;
    // --- Mouse Effects ---
    m_labelMouse = CreateWindow("STATIC", "MOUSE EFFECTS", WS_VISIBLE | WS_CHILD, margin, y, 200, 30, m_hwnd, NULL, NULL, NULL);
    SendMessage(m_labelMouse, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 40;
    m_checkHighlight = CreateWindow("BUTTON", "Highlighter in Output Video", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, margin + 10, y, 350, 35, m_hwnd, (HMENU)6, NULL, NULL);
    SendMessage(m_checkHighlight, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_checkHighlight, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 45;
    m_checkLiveHighlight = CreateWindow("BUTTON", "Highlighter While Recording\n(Disable if lagging)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_MULTILINE, margin + 10, y, 350, 60, m_hwnd, (HMENU)16, NULL, NULL);
    SendMessage(m_checkLiveHighlight, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_checkLiveHighlight, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 70;
    m_checkCursor = CreateWindow("BUTTON", "Show Mouse Cursor", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, margin + 10, y, 350, 35, m_hwnd, (HMENU)7, NULL, NULL);
    SendMessage(m_checkCursor, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_checkCursor, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 70;
    // --- Timer Label ---
    m_labelTimer = CreateWindow("STATIC", "Time: 00:00:00", WS_VISIBLE | WS_CHILD | SS_CENTER, margin, y, 350, 35, m_hwnd, NULL, NULL, NULL);
    SendMessage(m_labelTimer, WM_SETFONT, (WPARAM)hFont, TRUE);

    y += 60;
    // --- The Big Action Button ---
    m_btnStart = CreateWindow(
        "BUTTON", "START",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        60, y, 350, 75,
        m_hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
    );
    SendMessage(m_btnStart, WM_SETFONT, (WPARAM)hFont, TRUE);

    // --- F9 Hint ---
    // Increased width to 400 and offset slightly to prevent cropping
    m_labelF9Hint = CreateWindow("STATIC", "Hotkey: [ F9 ] to Start / Stop", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, y + 85, 400, 30, m_hwnd, NULL, NULL, NULL);
    SendMessage(m_labelF9Hint, WM_SETFONT, (WPARAM)hFont, TRUE);

    // --- Save Folder Section ---
    y += 160; // Increased from 125 to avoid overlap with F9 hint
    m_labelSavePath = CreateWindow("STATIC", m_savePath.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT | SS_ENDELLIPSIS, margin, y, 280, 30, m_hwnd, NULL, NULL, NULL);
    SendMessage(m_labelSavePath, WM_SETFONT, (WPARAM)hFont, TRUE);

    m_btnShowFolder = CreateWindow("BUTTON", "Show Folder", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, margin + 290, y - 5, 120, 40, m_hwnd, (HMENU)10, NULL, NULL);
    SendMessage(m_btnShowFolder, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Initial Layout Calculation
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    Relayout(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    // --- Pause Button (Hidden by default) ---
    m_btnPause = CreateWindow(
        "BUTTON", "PAUSE",
        WS_TABSTOP | WS_CHILD | BS_OWNERDRAW,
        0, 0, 0, 0,
        m_hwnd, (HMENU)9, GetModuleHandle(NULL), NULL
    );
    SendMessage(m_btnPause, WM_SETFONT, (WPARAM)hFont, TRUE);

    ShowWindow(m_hwnd, SW_SHOW);
    return true;
}

void Controller::UpdateButtonState() {
    if (m_isCountingDown) {
        char buf[32];
        sprintf(buf, "STARTING IN %d...", m_countdownValue);
        SetWindowText(m_btnStart, buf);
    } else if (m_isRecording) {
        SetWindowText(m_btnStart, "STOP RECORDING");
    } else {
        SetWindowText(m_btnStart, "START RECORDING");
    }
    InvalidateRect(m_btnStart, NULL, TRUE); // Force redraw to update color
}

void Controller::StartCountdown() {
    m_isCountingDown = true;
    m_countdownValue = 3;
    UpdateButtonState();
    SetTimer(m_hwnd, 101, 1000, NULL);
}

void Controller::UpdateTimer() {
    if (m_isRecording) {
        DWORD now = GetTickCount();
        DWORD activeTime;
        
        if (m_isPaused) {
            activeTime = m_pauseStartTime - m_startTime - m_totalPausedTime;
        } else {
            activeTime = now - m_startTime - m_totalPausedTime;
        }

        DWORD elapsed = activeTime / 1000;
        int hours = elapsed / 3600;
        int mins = (elapsed % 3600) / 60;
        int secs = elapsed % 60;
        char buf[64];
        sprintf(buf, "%02d:%02d:%02d", hours, mins, secs);
        SetWindowText(m_labelTimer, buf);
    }
}

void Controller::SwitchToFloatingBar(bool floating) {
    if (floating == m_isFloating) return;
    m_isFloating = floating;

    if (floating) {
        // Save current position
        GetWindowRect(m_hwnd, &m_originalRect);

        // REMOVE window decorations for a true floating bar look
        LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);
        style |= WS_POPUP;
        SetWindowLong(m_hwnd, GWL_STYLE, style);

        // Resize and move to top center (Bandicam Style)
        int barW = 350;
        int barH = 65; 
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        SetWindowPos(m_hwnd, HWND_TOPMOST, (screenW - barW) / 2, 0, barW, barH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        // Hide settings
        ShowWindow(m_labelVideo, SW_HIDE);
        ShowWindow(m_comboRes, SW_HIDE);
        ShowWindow(m_checkAudio, SW_HIDE);
        ShowWindow(m_checkCountdown, SW_HIDE);
        ShowWindow(m_checkFloating, SW_HIDE);
        // ShowWindow(m_checkWebcam, SW_HIDE);
        ShowWindow(m_labelCaptureArea, SW_HIDE);
        ShowWindow(m_comboArea, SW_HIDE);
        ShowWindow(m_labelMouse, SW_HIDE);
        ShowWindow(m_checkHighlight, SW_HIDE);
        ShowWindow(m_checkLiveHighlight, SW_HIDE);
        ShowWindow(m_checkCursor, SW_HIDE);
        ShowWindow(m_btnShowFolder, SW_HIDE);
        ShowWindow(m_labelSavePath, SW_HIDE);
        ShowWindow(m_labelF9Hint, SW_HIDE);

        // Layout inside the floating bar (Compact horizontal)
        SetWindowPos(m_labelTimer, NULL, 15, 15, 120, 35, SWP_NOZORDER);
        SetWindowPos(m_btnPause, NULL, 150, 10, 80, 45, SWP_NOZORDER | SWP_SHOWWINDOW);
        SetWindowPos(m_btnStart, NULL, 240, 10, 80, 45, SWP_NOZORDER);
        
        SetWindowText(m_btnStart, ""); // Icons drawn via WM_DRAWITEM
        SetWindowText(m_btnPause, "");
    } else {
        // Restore window decorations
        LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
        style &= ~WS_POPUP;
        style |= (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);
        SetWindowLong(m_hwnd, GWL_STYLE, style);

        // Restore Layout
        ShowWindow(m_btnPause, SW_HIDE);

        // Set position back and re-apply frame
        SetWindowPos(m_hwnd, HWND_NOTOPMOST, 
            m_originalRect.left, m_originalRect.top, 
            m_originalRect.right - m_originalRect.left, 
            m_originalRect.bottom - m_originalRect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        // Re-calculate layout
        RECT cr; GetClientRect(m_hwnd, &cr);
        Relayout(cr.right - cr.left, cr.bottom - cr.top);

        SetWindowText(m_labelTimer, "Time: 00:00:00");
        SetWindowText(m_btnStart, "START");

        // Show components
        ShowWindow(m_labelVideo, SW_SHOW);
        ShowWindow(m_comboRes, SW_SHOW);
        ShowWindow(m_checkAudio, SW_SHOW);
        ShowWindow(m_checkCountdown, SW_SHOW);
        ShowWindow(m_checkFloating, SW_SHOW);
        // ShowWindow(m_checkWebcam, SW_SHOW);
        ShowWindow(m_labelCaptureArea, SW_SHOW);
        ShowWindow(m_comboArea, SW_SHOW);
        ShowWindow(m_labelMouse, SW_SHOW);
        ShowWindow(m_checkHighlight, SW_SHOW);
        ShowWindow(m_checkLiveHighlight, SW_SHOW);
        ShowWindow(m_checkCursor, SW_SHOW);
        ShowWindow(m_btnShowFolder, SW_SHOW);
        ShowWindow(m_labelSavePath, SW_SHOW);
        ShowWindow(m_labelF9Hint, SW_SHOW);
    }
}

void Controller::Relayout(int width, int height) {
    if (m_isFloating) return;

    int margin = (width - 400) / 2;
    if (margin < 20) margin = 20;
    int y = 30;

    // Center UI elements
    SetWindowPos(m_labelVideo, NULL, margin, y, 180, 30, SWP_NOZORDER);
    SetWindowPos(m_comboRes, NULL, width - margin - 200, y - 5, 200, 300, SWP_NOZORDER);

    y += 75;
    SetWindowPos(m_labelCaptureArea, NULL, margin, y, 180, 30, SWP_NOZORDER);
    SetWindowPos(m_comboArea, NULL, width - margin - 200, y - 5, 200, 300, SWP_NOZORDER);

    y += 80;
    SetWindowPos(m_checkAudio, NULL, margin, y, 300, 35, SWP_NOZORDER);
    // y += 45;
    // SetWindowPos(m_checkWebcam, NULL, margin, y, 300, 35, SWP_NOZORDER);

    y += 75;
    SetWindowPos(m_checkCountdown, NULL, margin, y, 300, 35, SWP_NOZORDER);
    y += 45;
    SetWindowPos(m_checkFloating, NULL, margin, y, 300, 35, SWP_NOZORDER);
    
    y += 70;
    SetWindowPos(m_labelMouse, NULL, margin, y, 200, 30, SWP_NOZORDER);
    y += 40;
    SetWindowPos(m_checkHighlight, NULL, margin + 10, y, 350, 35, SWP_NOZORDER);
    y += 45;
    SetWindowPos(m_checkLiveHighlight, NULL, margin + 10, y, 350, 60, SWP_NOZORDER);
    y += 70;
    SetWindowPos(m_checkCursor, NULL, margin + 10, y, 350, 35, SWP_NOZORDER);

    y += 70;
    SetWindowPos(m_labelTimer, NULL, (width - 350) / 2, y, 350, 35, SWP_NOZORDER);

    y += 60;
    SetWindowPos(m_btnStart, NULL, (width - 350) / 2, y, 350, 75, SWP_NOZORDER);

    y += 85;
    SetWindowPos(m_labelF9Hint, NULL, (width - 400) / 2, y, 400, 30, SWP_NOZORDER);

    // Anchor Save Folder Section to the BOTTOM
    int bottomY = height - 60;
    SetWindowPos(m_labelSavePath, NULL, margin, bottomY + 5, 280, 30, SWP_NOZORDER);
    SetWindowPos(m_btnShowFolder, NULL, width - margin - 120, bottomY, 120, 40, SWP_NOZORDER);
}

// Separate WndProc for the Webcam Preview window to make it draggable
LRESULT CALLBACK WebcamPreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Controller* pController = (Controller*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (uMsg) {
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            break;
        }
        case WM_NCHITTEST: {
            // Check if we are over the close button (top-right 35x35)
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            if (pt.x > rc.right - 35 && pt.y < 35) {
                return HTCLIENT; // Must be client for LBUTTONDOWN and WM_SETCURSOR to fire
            }
            return HTCAPTION; // Draggable everywhere else
        }
        case WM_LBUTTONDOWN: {
            if (pController) {
                RECT rc; GetClientRect(hwnd, &rc);
                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                if (pt.x > rc.right - 35 && pt.y < 35) {
                    if (pController->m_checkWebcam) SendMessage(pController->m_checkWebcam, BM_SETCHECK, BST_UNCHECKED, 0);
                    SendMessage(pController->GetWindowHandle(), WM_COMMAND, 12, 0);
                    return 0;
                }
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            pController = (Controller*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            RECT rc; GetClientRect(hwnd, &rc);
            if (pController && !pController->m_previewFrame.empty()) {
                BITMAPINFO bmi = {0};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = pController->m_previewW;
                bmi.bmiHeader.biHeight = -pController->m_previewH; // Top-down
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                SetStretchBltMode(hdc, HALFTONE);
                SetBrushOrgEx(hdc, 0, 0, NULL);

                StretchDIBits(hdc, 0, 0, rc.right, rc.bottom,
                              0, 0, pController->m_previewW, pController->m_previewH,
                              pController->m_previewFrame.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
                
                // Draw Close Button (X) in top-right
                HBRUSH btnBrush = CreateSolidBrush(RGB(229, 57, 53));
                RECT btnRect = { rc.right - 25, 5, rc.right - 5, 25 };
                FillRect(hdc, &btnRect, btnBrush);
                DeleteObject(btnBrush);
                
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);
                DrawText(hdc, "X", 1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                HBRUSH hBr = CreateSolidBrush(RGB(50, 50, 50));
                FillRect(hdc, &rc, hBr);
                DeleteObject(hBr);
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);
                TextOut(hdc, 10, 10, "WEBCAM LOADING...", 17);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Controller::CreateWebcamPreviewWindow() {
    if (m_hwndWebcamPreview) return;

    const char* CLASS_NAME = "WebcamPreviewWindow";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WebcamPreviewWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    int w = 240;
    int h = 180;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    m_hwndWebcamPreview = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, "Webcam Preview",
        WS_POPUP | WS_BORDER,
        screenW - w - 50, screenH - h - 100, w, h,
        m_hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SetWindowLongPtr(m_hwndWebcamPreview, GWLP_USERDATA, (LONG_PTR)this);

    // EXCLUDE from capture so it doesn't appear on the recorded screen frames
    // (Only the composited one will be in the final video)
    SetWindowDisplayAffinity(m_hwndWebcamPreview, WDA_EXCLUDEFROMCAPTURE);
}

void Controller::SetPreviewFrame(const std::vector<uint8_t>& frame, int w, int h) {
    m_previewFrame = frame;
    m_previewW = w;
    m_previewH = h;
    if (m_hwndWebcamPreview) InvalidateRect(m_hwndWebcamPreview, NULL, FALSE);
}

void Controller::SetWebcamEnabled(bool enabled) {
    ToggleWebcamPreview(enabled);
}

void Controller::ToggleWebcamPreview(bool show) {
    if (show) {
        if (!m_hwndWebcamPreview) CreateWebcamPreviewWindow();
        
        // Resize preview to match recording size (20% of screen height)
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int targetH = screenH / 5;
        // Assume 4:3 camera for initial sizing, will adjust when frame arrives
        int targetW = (targetH * 4) / 3; 
        
        // Position it at bottom right with padding, but ensure it's on screen
        int x = screenW - targetW - 50;
        int y = screenH - targetH - 100;
        
        SetWindowPos(m_hwndWebcamPreview, HWND_TOPMOST, x, y, targetW, targetH, SWP_SHOWWINDOW);

        // Only start the controller's camera if we are NOT recording
        if (!m_isRecording) {
            if (m_webcamPreview.Initialize(0)) {
                m_webcamPreview.Start();
                SetTimer(m_hwnd, 103, 33, NULL); // Preview timer (30fps)
            }
        }
        ShowWindow(m_hwndWebcamPreview, SW_SHOW);
    } else {
        KillTimer(m_hwnd, 103);
        m_webcamPreview.Stop();
        m_webcamPreview.Cleanup();
        if (m_hwndWebcamPreview) ShowWindow(m_hwndWebcamPreview, SW_HIDE);
    }
}

// --- Capture Indicator Window (Shows a border around recording area) ---
LRESULT CALLBACK CaptureIndicatorWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Controller* pCtrl = (Controller*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg) {
        case WM_NCHITTEST: {
            if (pCtrl && (pCtrl->IsRecording() || pCtrl->IsPaused())) return HTTRANSPARENT;
            
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            int border = 12; // Thick hit-test area for easier grabbing

            // Corner & Edge resizing...
            if (pt.x < border && pt.y < border) return HTTOPLEFT;
            if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
            if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;

            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;
            if (pt.y < border) return HTTOP;
            if (pt.y > rc.bottom - border) return HTBOTTOM;

            // DRAG HANDLE: Top-center area (approx 120px wide, 35px high for easier clicking)
            int mid = rc.right / 2;
            if (pt.x > mid - 60 && pt.x < mid + 60 && pt.y < 35) {
                return HTCAPTION;
            }

            // In the middle, it should be pass-through
            return HTTRANSPARENT;
        }
        case WM_LBUTTONDOWN: {
            break;
        }
        case WM_SIZE:
        case WM_MOVE: {
            if (pCtrl && !pCtrl->IsRecording() && !pCtrl->IsPaused()) {
                RECT rc;
                if (GetWindowRect(hwnd, &rc)) {
                    pCtrl->UpdateCustomRegionFromIndicator(rc);
                }
            }
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            
            // 1. Double buffer to ensure perfect transparency
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBM);

            // 2. FILL with Magenta (Transparency Key)
            HBRUSH hTrans = CreateSolidBrush(RGB(255, 0, 255));
            FillRect(memDC, &rc, hTrans);
            DeleteObject(hTrans);

            // 3. Draw a thick glowing border
            // Green if NOT recording OR if paused, Red if recording correctly
            COLORREF color = RGB(255, 50, 50); // Default Red
            if (pCtrl && (!pCtrl->IsRecording() || pCtrl->IsPaused())) {
                color = RGB(0, 120, 215); // Windows Blue
            }

            HPEN hPen = CreatePen(PS_SOLID, 4, color);
            HGDIOBJ hOldPen = SelectObject(memDC, hPen);
            SelectObject(memDC, GetStockObject(NULL_BRUSH));
            
            Rectangle(memDC, 0, 0, rc.right, rc.bottom);
            
            // Draw a 'Move Handle' at the top center
            if (pCtrl && !pCtrl->IsRecording()) {
                int mid = rc.right / 2;
                RECT handleRect = { mid - 50, 0, mid + 50, 30 };
                HBRUSH hBr = CreateSolidBrush(color);
                FillRect(memDC, &handleRect, hBr);
                DeleteObject(hBr);

                // Create a bold font for the MOVE text
                HFONT hBoldFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                HFONT hOldFont = (HFONT)SelectObject(memDC, hBoldFont);

                SetTextColor(memDC, RGB(0, 0, 0)); // Black text per user request
                SetBkMode(memDC, TRANSPARENT);
                DrawText(memDC, "MOVE", 4, &handleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(memDC, hOldFont);
                DeleteObject(hBoldFont);

                SetTextColor(memDC, color);
                DrawText(memDC, "Drag edges to resize", -1, &rc, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
            }

            SelectObject(memDC, hOldPen);
            DeleteObject(hPen);

            // Copy to screen
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            
            DeleteObject(memBM);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Controller::CreateCaptureIndicatorWindow() {
    if (m_hwndCaptureIndicator) return;

    const char* CLASS_NAME = "CaptureIndicatorWindow";
    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = CaptureIndicatorWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);

    m_hwndCaptureIndicator = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        CLASS_NAME, "Recording Indicator",
        WS_POPUP,
        0, 0, 100, 100,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    SetWindowLongPtr(m_hwndCaptureIndicator, GWLP_USERDATA, (LONG_PTR)this);

    // Use ColorKey for perfect transparency (Magenta = clear)
    SetLayeredWindowAttributes(m_hwndCaptureIndicator, RGB(255, 0, 255), 255, LWA_COLORKEY);
    
    // EXCLUDE from capture
    SetWindowDisplayAffinity(m_hwndCaptureIndicator, WDA_EXCLUDEFROMCAPTURE);
}

void Controller::UpdateCustomRegionFromIndicator(RECT windowRect) {
    // Convert indicator window rect back to actual capture region
    // Reminder: we expand by 2 in ToggleCaptureIndicator, so we shrink by 2 here
    m_settings.customRegion.left = windowRect.left + 2;
    m_settings.customRegion.top = windowRect.top + 2;
    m_settings.customRegion.right = windowRect.right - 2;
    m_settings.customRegion.bottom = windowRect.bottom - 2;
}

void Controller::ToggleCaptureIndicator(bool show, RECT region) {
    if (show) {
        if (!m_hwndCaptureIndicator) CreateCaptureIndicatorWindow();
        
        int x, y, w, h;
        if (region.right == 0 && region.bottom == 0) {
            // Full Screen - per user request, don't show border for full screen
            if (m_hwndCaptureIndicator) ShowWindow(m_hwndCaptureIndicator, SW_HIDE);
            return;
        } else {
            x = region.left; y = region.top;
            w = region.right - region.left;
            h = region.bottom - region.top;
        }

        // Expand slightly to be outside the capture area
        SetWindowPos(m_hwndCaptureIndicator, HWND_TOPMOST, x - 2, y - 2, w + 4, h + 4, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        // Force redraw to ensure correct color (Green/Red)
        InvalidateRect(m_hwndCaptureIndicator, NULL, TRUE);
    } else {
        // Only hide if we aren't in custom area mode anymore
        if (!m_settings.useCustomArea && m_hwndCaptureIndicator) {
            ShowWindow(m_hwndCaptureIndicator, SW_HIDE);
        } else if (m_hwndCaptureIndicator) {
            // Just refresh color to Green
            InvalidateRect(m_hwndCaptureIndicator, NULL, TRUE);
        }
    }
}

// --- Mouse Overlay Window (Shows highlight on screen) ---
LRESULT CALLBACK MouseOverlayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Controller* pCtrl = (Controller*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBM);

            // Fill with Magenta (Transparency Key)
            HBRUSH hTrans = CreateSolidBrush(RGB(255, 0, 255));
            FillRect(memDC, &rc, hTrans);
            DeleteObject(hTrans);

            if (pCtrl && pCtrl->m_settings.showHighlight) {
                // Draw at the center of our small window
                int cx = rc.right / 2;
                int cy = rc.bottom / 2;

                bool isClicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                COLORREF color = isClicked ? RGB(255, 50, 50) : RGB(255, 255, 0);
                int radius = isClicked ? 30 : 25;

                HBRUSH hBrush = CreateSolidBrush(color);
                HGDIOBJ hOld = SelectObject(memDC, hBrush);
                SelectObject(memDC, GetStockObject(NULL_PEN));
                Ellipse(memDC, cx - radius, cy - radius, cx + radius, cy + radius);
                SelectObject(memDC, hOld);
                DeleteObject(hBrush);
            }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBM);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Controller::CreateMouseOverlayWindow() {
    if (m_hwndMouseOverlay) return;

    const char* CLASS_NAME = "MouseOverlayWindow";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MouseOverlayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);

    m_hwndMouseOverlay = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        CLASS_NAME, "Mouse Overlay",
        WS_POPUP,
        0, 0, 100, 100,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    SetWindowLongPtr(m_hwndMouseOverlay, GWLP_USERDATA, (LONG_PTR)this);

    // Use ColorKey for perfect transparency (Magenta = clear) + Global Alpha for semi-transparency
    SetLayeredWindowAttributes(m_hwndMouseOverlay, RGB(255, 0, 255), 150, LWA_COLORKEY | LWA_ALPHA);
    
    // EXCLUDE from capture
    SetWindowDisplayAffinity(m_hwndMouseOverlay, WDA_EXCLUDEFROMCAPTURE);
}

void Controller::ToggleMouseOverlay(bool show) {
    if (show && m_settings.showLiveHighlight) {
        if (!m_hwndMouseOverlay) CreateMouseOverlayWindow();
        
        // Initial position follow
        POINT p; GetCursorPos(&p);
        int size = 120;
        SetWindowPos(m_hwndMouseOverlay, HWND_TOPMOST, p.x - size/2, p.y - size/2, size, size, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        
        SetTimer(m_hwnd, 104, 10, NULL); // 100 FPS update for perfect tracking
    } else {
        if (m_hwndMouseOverlay) ShowWindow(m_hwndMouseOverlay, SW_HIDE);
        KillTimer(m_hwnd, 104);
    }
}

void Controller::Run() {
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK Controller::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Controller* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (Controller*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (Controller*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (pThis) {
        switch (uMsg) {
            case WM_GETMINMAXINFO: {
                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                if (!pThis->m_isFloating) {
                    mmi->ptMinTrackSize.x = 460;
                    mmi->ptMinTrackSize.y = 980; // Increased to ensure bottom part is visible
                    mmi->ptMaxTrackSize.x = 700;
                    mmi->ptMaxTrackSize.y = 1100;
                }
                return 0;
            }

            case WM_SIZE: {
                if (!pThis->m_isFloating && wParam != SIZE_MINIMIZED) {
                    pThis->Relayout(LOWORD(lParam), HIWORD(lParam));
                }
                return 0;
            }

            case WM_NCHITTEST: {
                LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);
                if (pThis->m_isFloating && hit == HTCLIENT) return HTCAPTION;
                return hit;
            }

            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLORBTN: {
                HDC hdcStatic = (HDC)wParam;
                HWND hCtrl = (HWND)lParam;
                
                // Background color for the window and static text
                SetBkColor(hdcStatic, RGB(35, 35, 38));
                SetTextColor(hdcStatic, RGB(240, 240, 240)); // Light Gray Text
                return (INT_PTR)CreateSolidBrush(RGB(35, 35, 38));
            }

            case WM_DRAWITEM: {
                LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
                if (pdis->CtlID == 1 || pdis->CtlID == 9) { // m_btnStart or m_btnPause
                    HBRUSH hBrush;
                    bool isStop = (pdis->CtlID == 1 && pThis->m_isRecording);
                    bool isStart = (pdis->CtlID == 1 && !pThis->m_isRecording);
                    bool isPause = (pdis->CtlID == 9);

                    if (pdis->CtlID == 1) { // Start/Stop
                        if (pThis->m_isCountingDown) hBrush = CreateSolidBrush(RGB(255, 165, 0));
                        else if (pThis->m_isRecording) hBrush = CreateSolidBrush(RGB(229, 57, 53));
                        else hBrush = CreateSolidBrush(RGB(0, 122, 204));
                    } else { // Pause/Resume
                        hBrush = CreateSolidBrush(RGB(80, 80, 80));
                    }
                    
                    FillRect(pdis->hDC, &pdis->rcItem, hBrush);
                    
                    // Draw Icons
                    RECT rc = pdis->rcItem;
                    int w = rc.right - rc.left;
                    int h = rc.bottom - rc.top;
                    int cx = rc.left + w / 2;
                    int cy = rc.top + h / 2;

                    if (isStop) {
                        // Square (Stop)
                        RECT sq = { cx - 10, cy - 10, cx + 10, cy + 10 };
                        FillRect(pdis->hDC, &sq, (HBRUSH)GetStockObject(WHITE_BRUSH));
                    } else if (isStart && !pThis->m_isCountingDown && !pThis->m_isFloating) {
                        // Just text for main start button
                        char text[64];
                        GetWindowText(pdis->hwndItem, text, 64);
                        SetTextColor(pdis->hDC, RGB(255, 255, 255));
                        SetBkMode(pdis->hDC, TRANSPARENT);
                        HFONT hFont = (HFONT)SendMessage(pdis->hwndItem, WM_GETFONT, 0, 0);
                        SelectObject(pdis->hDC, hFont);
                        DrawText(pdis->hDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    } else if (isPause || pThis->m_isCountingDown) {
                        if (pThis->m_isCountingDown) {
                             // Show countdown number
                             char buf[16]; sprintf(buf, "%d", pThis->m_countdownValue);
                             SetTextColor(pdis->hDC, RGB(255, 255, 255));
                             SetBkMode(pdis->hDC, TRANSPARENT);
                             DrawText(pdis->hDC, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                        } else if (pThis->m_isPaused) {
                            // Triangle (Resume/Play)
                            POINT tri[3] = { {cx - 8, cy - 10}, {cx - 8, cy + 10}, {cx + 12, cy} };
                            SelectObject(pdis->hDC, GetStockObject(WHITE_BRUSH));
                            Polygon(pdis->hDC, tri, 3);
                        } else {
                            // Two bars (Pause)
                            RECT r1 = { cx - 8, cy - 10, cx - 2, cy + 10 };
                            RECT r2 = { cx + 2, cy - 10, cx + 8, cy + 10 };
                            FillRect(pdis->hDC, &r1, (HBRUSH)GetStockObject(WHITE_BRUSH));
                            FillRect(pdis->hDC, &r2, (HBRUSH)GetStockObject(WHITE_BRUSH));
                        }
                    }

                    if (pdis->itemState & ODS_SELECTED) InvertRect(pdis->hDC, &pdis->rcItem);
                    DeleteObject(hBrush);
                    return TRUE;
                }
                break;
            }

            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case 1: // Start/Stop Button
                        if (pThis->m_isCountingDown) break;

                        if (!pThis->m_isRecording) {
                            // Preparation to start
                            pThis->m_settings.recordAudio = (SendMessage(pThis->m_checkAudio, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            pThis->m_settings.showHighlight = (SendMessage(pThis->m_checkHighlight, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            pThis->m_settings.showLiveHighlight = (SendMessage(pThis->m_checkLiveHighlight, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            pThis->m_settings.showCursor = (SendMessage(pThis->m_checkCursor, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            pThis->m_settings.useCountdown = (SendMessage(pThis->m_checkCountdown, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            pThis->m_settings.useFloatingBar = (SendMessage(pThis->m_checkFloating, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            pThis->m_settings.useWebcam = (SendMessage(pThis->m_checkWebcam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                            
                            int areaSel = (int)SendMessage(pThis->m_comboArea, CB_GETCURSEL, 0, 0);
                            pThis->m_settings.useCustomArea = (areaSel == 1);

                            if (pThis->m_settings.useWebcam) {
                                // Stop the UI's preview camera so the recording engine can take it
                                pThis->m_webcamPreview.Stop();
                                pThis->m_webcamPreview.Cleanup();
                                KillTimer(hwnd, 103);
                                
                                RECT rc;
                                GetWindowRect(pThis->m_hwndWebcamPreview, &rc);
                                pThis->m_settings.webcamPos.x = rc.left;
                                pThis->m_settings.webcamPos.y = rc.top;
                            }

                            if (pThis->m_settings.useCustomArea) {
                                // Area is already selected via button, logic simplified
                            } else {
                                pThis->m_settings.customRegion = {0}; // Reset
                                int sel = (int)SendMessage(pThis->m_comboRes, CB_GETCURSEL, 0, 0);
                                if (sel == 0) { pThis->m_settings.width = 0; pThis->m_settings.height = 0; }
                                else if (sel == 1) { pThis->m_settings.width = -1; pThis->m_settings.height = 1080; }
                                else if (sel == 2) { pThis->m_settings.width = -1; pThis->m_settings.height = 720; }
                                else if (sel == 3) { pThis->m_settings.width = -1; pThis->m_settings.height = 480; }
                            }

                            if (pThis->m_settings.useCountdown) {
                                pThis->StartCountdown();
                            } else {
                                pThis->m_isRecording = true;
                                pThis->m_startTime = GetTickCount();
                                pThis->UpdateButtonState();
                                if (pThis->m_onStart) pThis->m_onStart();
                                SetTimer(hwnd, 102, 1000, NULL); // Recording timer
                                if (pThis->m_settings.useFloatingBar) pThis->SwitchToFloatingBar(true);
                                
                                // Show the recording area indicator
                                pThis->ToggleCaptureIndicator(true, pThis->m_settings.customRegion);

                                // Show the mouse feedback overlay
                                pThis->ToggleMouseOverlay(true);
                            }
                        } else {
                            // Stop recording
                            KillTimer(hwnd, 102);
                            pThis->m_isRecording = false;
                            pThis->m_isPaused = false;
                            pThis->UpdateButtonState();
                            if (pThis->m_onStop) pThis->m_onStop();
                            pThis->SwitchToFloatingBar(false);
                            
                            // Set color back to Green immediately
                            pThis->ToggleCaptureIndicator(true, pThis->m_settings.customRegion);

                            // Hide the mouse feedback overlay
                            pThis->ToggleMouseOverlay(false);

                            // Restart UI preview camera if it was enabled
                            if (pThis->m_settings.useWebcam) {
                                pThis->ToggleWebcamPreview(true);
                            }
                        }
                        break;

                    case 2: // Resolution Combo
                        // No longer resetting area here as they are separate sections
                        break;

                    case 9: // Pause Button
                        if (pThis->m_isRecording && !pThis->m_isCountingDown) {
                            pThis->m_isPaused = !pThis->m_isPaused;
                            if (pThis->m_isPaused) {
                                pThis->m_pauseStartTime = GetTickCount();
                            } else {
                                pThis->m_totalPausedTime += (GetTickCount() - pThis->m_pauseStartTime);
                            }
                            if (pThis->m_onPause) pThis->m_onPause(pThis->m_isPaused);
                            
                            // Immediately refresh border color
                            pThis->ToggleCaptureIndicator(true, pThis->m_settings.customRegion);
                            
                            InvalidateRect(pThis->m_btnPause, NULL, TRUE);
                        }
                        break;

                    case 10: // Open Folder
                        ShellExecuteA(NULL, "open", pThis->m_savePath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
                        break;

                    case 11: // Floating Bar Checkbox
                    {
                        bool enable = (SendMessage(pThis->m_checkFloating, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        pThis->m_settings.useFloatingBar = enable;
                        pThis->SwitchToFloatingBar(enable);
                        break;
                    }

                    case 12: // Webcam Checkbox
                        pThis->ToggleWebcamPreview(SendMessage(pThis->m_checkWebcam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        break;

                    case 16: // Live Highlight Checkbox
                    {
                        bool enable = (SendMessage(pThis->m_checkLiveHighlight, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        pThis->m_settings.showLiveHighlight = enable;
                        if (pThis->m_isRecording) {
                            pThis->ToggleMouseOverlay(enable);
                        }
                        break;
                    }

                    case 15: // Area Combo
                    {
                        if (HIWORD(wParam) == CBN_SELCHANGE) {
                            int sel = (int)SendMessage(pThis->m_comboArea, CB_GETCURSEL, 0, 0);
                            if (sel == 1) { // Custom Area
                                RegionSelector selector;
                                ShowWindow(hwnd, SW_HIDE);
                                if (selector.SelectRegion(pThis->m_settings.customRegion)) {
                                    pThis->m_settings.useCustomArea = true;
                                    // Show the indicator immediately after selection
                                    pThis->ToggleCaptureIndicator(true, pThis->m_settings.customRegion);
                                } else {
                                    pThis->m_settings.useCustomArea = false;
                                    SendMessage(pThis->m_comboArea, CB_SETCURSEL, 0, 0); // Revert to Full Screen
                                    pThis->ToggleCaptureIndicator(false, { 0 });
                                }
                                ShowWindow(hwnd, SW_SHOW);
                            } else {
                                pThis->m_settings.useCustomArea = false;
                                pThis->m_settings.customRegion = { 0 };
                                pThis->ToggleCaptureIndicator(false, { 0 });
                            }
                        }
                        break;
                    }
                }
                break;

            case WM_HOTKEY:
                if (wParam == 1) { // F9
                    if (!pThis->m_isRecording) {
                        // If minimized, always ensure floating bar is enabled for hotkey start
                        if (IsIconic(hwnd)) {
                            pThis->m_settings.useFloatingBar = true;
                            SendMessage(pThis->m_checkFloating, BM_SETCHECK, BST_CHECKED, 0);
                        }
                    }
                    SendMessage(hwnd, WM_COMMAND, 1, 0);
                }
                break;

            case WM_TIMER:
                if (wParam == 101) { // Countdown timer
                    pThis->m_countdownValue--;
                    if (pThis->m_countdownValue <= 0) {
                        KillTimer(hwnd, 101);
                        pThis->m_isCountingDown = false;
                        pThis->m_isRecording = true;
                        pThis->m_startTime = GetTickCount();
                        pThis->UpdateButtonState();
                        if (pThis->m_onStart) pThis->m_onStart();
                        SetTimer(hwnd, 102, 1000, NULL); // Start recording timer
                        if (pThis->m_settings.useFloatingBar) pThis->SwitchToFloatingBar(true);

                        // Show the mouse feedback overlay
                        pThis->ToggleMouseOverlay(true);
                    } else {
                        pThis->UpdateButtonState();
                    }
                } else if (wParam == 102) { // Recording timer
                    pThis->UpdateTimer();
                } else if (wParam == 103) { // Webcam Preview timer
                    if (pThis->m_webcamPreview.GetFrame(pThis->m_previewFrame, pThis->m_previewW, pThis->m_previewH)) {
                        // Dynamically adjust window aspect ratio to match camera
                        RECT rc; GetWindowRect(pThis->m_hwndWebcamPreview, &rc);
                        int curW = rc.right - rc.left;
                        int curH = rc.bottom - rc.top;
                        int expectedW = (int)((float)pThis->m_previewW / pThis->m_previewH * curH);
                        if (abs(curW - expectedW) > 5) {
                            int screenW = GetSystemMetrics(SM_CXSCREEN);
                            int screenH = GetSystemMetrics(SM_CYSCREEN);
                            int newX = rc.left;
                            int newY = rc.top;
                            
                            // If resizing would push it off-screen, shift it back
                            if (newX + expectedW > screenW) newX = screenW - expectedW - 20;
                            if (newY + curH > screenH) newY = screenH - curH - 20;
                            if (newX < 0) newX = 20;
                            if (newY < 0) newY = 20;

                            SetWindowPos(pThis->m_hwndWebcamPreview, NULL, newX, newY, expectedW, curH, SWP_NOZORDER);
                        }
                        InvalidateRect(pThis->m_hwndWebcamPreview, NULL, FALSE);
                    }
                } else if (wParam == 104) { // Mouse Overlay update
                    if (pThis->m_hwndMouseOverlay && pThis->m_settings.showHighlight) {
                        POINT p; GetCursorPos(&p);
                        int size = 120;
                        // Move the window to the cursor position
                        SetWindowPos(pThis->m_hwndMouseOverlay, HWND_TOPMOST, p.x - size/2, p.y - size/2, 0, 0, 
                                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                        InvalidateRect(pThis->m_hwndMouseOverlay, NULL, FALSE);
                    }
                }
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
