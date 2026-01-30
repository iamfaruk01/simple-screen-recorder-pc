#include "RegionSelector.hpp"
#include <algorithm>

RegionSelector::RegionSelector() {
    const char CLASS_NAME[] = "RegionSelectorOverlay";
    
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_CROSS); // Crosshair for drawing
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent background

    RegisterClass(&wc);

    // Create a full-screen layered window
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "Select Area",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        NULL, NULL, GetModuleHandle(NULL), this
    );

    // Start nearly invisible (1/255) but solid so we can catch the first mouse click.
    // We don't use LWA_COLORKEY yet because it would make the window click-through.
    SetLayeredWindowAttributes(m_hwnd, 0, 1, LWA_ALPHA);
}

RegionSelector::~RegionSelector() {
    if (m_hwnd) DestroyWindow(m_hwnd);
}

bool RegionSelector::SelectRegion(RECT& outRect) {
    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // Modal message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            m_confirmed = false;
            break;
        }
        
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            if (m_selection.right > m_selection.left && m_selection.bottom > m_selection.top) {
                m_confirmed = true;
                break;
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Check if window was closed
        if (!IsWindow(m_hwnd)) break;
        if (m_confirmed) break;
    }

    ShowWindow(m_hwnd, SW_HIDE);

    if (m_confirmed) {
        outRect = m_selection;
        return true;
    }
    return false;
}

LRESULT CALLBACK RegionSelector::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RegionSelector* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (RegionSelector*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (RegionSelector*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        switch (uMsg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                RECT client; GetClientRect(hwnd, &client);
                
                // 1. Double buffer to ensure no smearing
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBM = CreateCompatibleBitmap(hdc, client.right, client.bottom);
                SelectObject(memDC, memBM);

                // 2. FILL the entire window with Magenta (the transparency key)
                // This makes the window "disappear" except for what we draw next
                HBRUSH transBrush = CreateSolidBrush(RGB(255, 0, 255));
                FillRect(memDC, &client, transBrush);
                DeleteObject(transBrush);

                // 3. Draw ONLY the Selection Border (No dimming)
                if (pThis->m_isSelecting) {
                    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
                    HGDIOBJ hOldPen = SelectObject(memDC, hPen);
                    SelectObject(memDC, GetStockObject(NULL_BRUSH));

                    Rectangle(memDC, pThis->m_selection.left, pThis->m_selection.top, pThis->m_selection.right, pThis->m_selection.bottom);
                    
                    // Dimensions Label
                    char text[64];
                    int w = pThis->m_selection.right - pThis->m_selection.left;
                    int h = pThis->m_selection.bottom - pThis->m_selection.top;
                    sprintf(text, " %d x %d ", w, h);
                    
                    SetBkMode(memDC, OPAQUE);
                    SetBkColor(memDC, RGB(0, 120, 215));
                    SetTextColor(memDC, RGB(255, 255, 255));
                    TextOut(memDC, pThis->m_selection.left, max(0, pThis->m_selection.top - 25), text, (int)strlen(text));

                    SelectObject(memDC, hOldPen);
                    DeleteObject(hPen);
                }

                // Copy to screen
                BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);
                
                DeleteObject(memBM);
                DeleteDC(memDC);
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_LBUTTONDOWN: {
                pThis->m_isSelecting = true;
                pThis->m_startPoint.x = LOWORD(lParam);
                pThis->m_startPoint.y = HIWORD(lParam);
                pThis->m_selection.left = pThis->m_startPoint.x;
                pThis->m_selection.top = pThis->m_startPoint.y;
                pThis->m_selection.right = pThis->m_startPoint.x + 1;
                pThis->m_selection.bottom = pThis->m_startPoint.y + 1;

                // Important: Switch to ColorKey mode so the window becomes transparent,
                // and use SetCapture so we keep receiving mouse events despite being click-through.
                SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 255, LWA_COLORKEY);
                SetCapture(hwnd);
                
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            case WM_MOUSEMOVE: {
                if (pThis->m_isSelecting) {
                    int x = LOWORD(lParam);
                    int y = HIWORD(lParam);

                    pThis->m_selection.left = min(pThis->m_startPoint.x, x);
                    pThis->m_selection.top = min(pThis->m_startPoint.y, y);
                    pThis->m_selection.right = max(pThis->m_startPoint.x, x);
                    pThis->m_selection.bottom = max(pThis->m_startPoint.y, y);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                return 0;
            }

            case WM_LBUTTONUP: {
                if (pThis->m_isSelecting) {
                    pThis->m_isSelecting = false;
                    ReleaseCapture();
                    
                    // Auto-confirm on release? Or wait for Enter? 
                    // Let's auto-confirm for speed like lightshot
                    pThis->m_confirmed = true;
                }
                return 0;
            }

            case WM_ERASEBKGND:
                return 1; // Prevent flickering
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
