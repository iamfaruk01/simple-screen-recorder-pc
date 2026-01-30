#pragma once

#define NOMINMAX
#include <windows.h>
#include <vector>
#include <cstdint>

/**
 * VisualEffects provides functions to draw on raw video frames.
 */
class VisualEffects {
public:
    struct Color {
        uint8_t r, g, b, a;
    };

    /**
     * Draws a semi-transparent circle at the given position
     */
    static void DrawHighlight(std::vector<uint8_t>& bgraData, int width, int height, POINT mousePos, int radius, Color color);

    /**
     * Draws a professional cursor shape
     */
    static void DrawCursor(std::vector<uint8_t>& bgraData, int width, int height, POINT mousePos);

    /**
     * Gets the current mouse position relative to the screen
     */
    static POINT GetMousePosition();

    /**
     * Checks if the left mouse button is currently pressed
     */
    static bool IsLeftClicked();
};
