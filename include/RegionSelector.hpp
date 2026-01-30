#pragma once
#include <windows.h>
#include <functional>

/**
 * RegionSelector Overlay
 * Creates a semi-transparent full-screen overlay that allows drawing a selection box.
 */
class RegionSelector {
public:
    RegionSelector();
    ~RegionSelector();

    // Returns true if selection was confirmed, false if cancelled
    bool SelectRegion(RECT& outRect);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HWND m_hwnd = nullptr;
    RECT m_selection = {0};
    bool m_isSelecting = false;
    bool m_confirmed = false;
    POINT m_startPoint = {0};

    // Semi-transparent brush
    HBRUSH m_hBrushSelection = nullptr;
};
