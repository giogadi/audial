#pragma once

struct ViewportInfo {
    // values in GL framebuffer pixels
    // offset is the position of the bottom-left corner of rectangle
    int _offsetX = 0;
    int _offsetY = 0;
    int _width = 0;
    int _height = 0;

    // offset is from top-left of rectangle. in screen coordinates.
    int _screenOffsetX = 0;
    int _screenOffsetY = 0;
    int _screenWidth = 0;
    int _screenHeight = 0;
};

ViewportInfo CalculateViewport(float aspectRatio, int windowWidthPx, int windowHeightPx, int windowWidthScreen, int windowHeightScreen);
