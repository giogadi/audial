#include "viewport.h"

#include <cassert>

ViewportInfo CalculateViewport(
    float aspectRatio,
    int windowWidthPx, int windowHeightPx,
    int windowWidthScreen, int windowHeightScreen) {
       
    ViewportInfo viewport;
    float _pxToScreenX = (float) windowWidthScreen / windowWidthPx;
    float _pxToScreenY = (float) windowHeightScreen / windowHeightPx;
    
    float bufAspect = (float) windowWidthPx / (float) windowHeightPx;
    if (bufAspect > aspectRatio) {
        viewport._width = aspectRatio * windowHeightPx;
        viewport._height = windowHeightPx;
        viewport._offsetX = (windowWidthPx - viewport._width) / 2;
        assert(viewport._offsetX >= 0);
    } else if (bufAspect < aspectRatio) {
        viewport._height = windowWidthPx / aspectRatio;
        viewport._width = windowWidthPx;
        viewport._offsetY = (windowHeightPx - viewport._height) / 2;
        assert(viewport._offsetX >= 0);
    } else {
        viewport._width = windowWidthPx;
        viewport._height = windowHeightPx;
    }

    viewport._screenOffsetX = viewport._offsetX * _pxToScreenX;
    // px are measured from bottom-left corner, whereas screen from top-left
    viewport._screenOffsetY = (windowHeightPx - (viewport._offsetY + viewport._height)) * _pxToScreenY;
    viewport._screenWidth = viewport._width * _pxToScreenX;
    viewport._screenHeight = viewport._height * _pxToScreenY;
        
    return viewport;
}
