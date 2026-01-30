#include "VisualEffects.hpp"
#include <cmath>
#include <algorithm>

void VisualEffects::DrawHighlight(std::vector<uint8_t>& bgraData, int width, int height, POINT mousePos, int radius, Color color) {
    if (bgraData.empty()) return;

    int r2 = radius * radius;
    float alpha = color.a / 255.0f;
    float invAlpha = 1.0f - alpha;

    int startX = std::max(0, (int)mousePos.x - radius);
    int endX = std::min(width - 1, (int)mousePos.x + radius);
    int startY = std::max(0, (int)mousePos.y - radius);
    int endY = std::min(height - 1, (int)mousePos.y + radius);

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            int dx = x - mousePos.x;
            int dy = y - mousePos.y;
            
            if (dx * dx + dy * dy <= r2) {
                int pixelPos = (y * width + x) * 4;
                bgraData[pixelPos]     = (uint8_t)(bgraData[pixelPos]     * invAlpha + color.b * alpha);
                bgraData[pixelPos + 1] = (uint8_t)(bgraData[pixelPos + 1] * invAlpha + color.g * alpha);
                bgraData[pixelPos + 2] = (uint8_t)(bgraData[pixelPos + 2] * invAlpha + color.r * alpha);
            }
        }
    }
}

void VisualEffects::DrawCursor(std::vector<uint8_t>& bgraData, int width, int height, POINT mousePos) {
    // 12x19 Standard Cursor Bitmap
    // 0 = Transparent
    // 1 = Black Border
    // 2 = White Fill
    static const int cursorW = 12;
    static const int cursorH = 19;
    static const int cursorShape[19][12] = {
        {1,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,0,0},
        {1,2,2,2,2,2,1,0,0,0,0,0},
        {1,2,2,2,2,2,2,1,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,2,2,2,1,0,0},
        {1,2,2,2,2,2,1,1,1,1,0,0},
        {1,2,2,1,2,2,1,0,0,0,0,0},
        {1,2,1,0,1,2,2,1,0,0,0,0},
        {1,1,0,0,1,2,2,1,0,0,0,0},
        {1,0,0,0,0,1,2,2,1,0,0,0},
        {0,0,0,0,0,1,2,2,1,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    };

    for (int y = 0; y < cursorH; ++y) {
        for (int x = 0; x < cursorW; ++x) {
            int pixelType = cursorShape[y][x];
            if (pixelType == 0) continue;

            int px = mousePos.x + x;
            int py = mousePos.y + y;

            if (px >= 0 && px < width && py >= 0 && py < height) {
                size_t index = (py * width + px) * 4;
                
                if (pixelType == 1) { // Black Border
                    bgraData[index] = 0; bgraData[index+1] = 0; bgraData[index+2] = 0; 
                } 
                else if (pixelType == 2) { // White Fill
                    bgraData[index] = 255; bgraData[index+1] = 255; bgraData[index+2] = 255;
                }
                // (Alpha remains 255 from capture or set clearly here if needed, but usually Capture sets it)
            }
        }
    }
}

POINT VisualEffects::GetMousePosition() {
    POINT p;
    GetCursorPos(&p);
    return p;
}

bool VisualEffects::IsLeftClicked() {
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}
