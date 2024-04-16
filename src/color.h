#pragma once

#include "matrix.h"
#if 0
inline Vec4 RgbaToHsva(Vec4 const& rgba) {
    float r = rgba._x, g = rgba._y, b = rgba._z;
    float xMax = std::max(r, std::max(g, b));
    float xMin = std::min(r, std::min(g, b));
    float v = xMax;
    float c = xMax - xMin;
    float hue;
    if (c == 0.f) {
        hue = 0.f;
    } else if (v == r) {
        hue = 60 * fmod((g - b) / c, 6.f);
    } else if (v == g) {
        hue = 60 * (((b - r) / c) + 2.f);
    } else 
        hue = 60 * (((r - g) / c) + 4.f);
    }
    float s;
    if (v == 0.f) {
        s = 0.f;
    } else {
        s = c / v;
    }

    return Vec4(hue, s, v, rgba._w);
}

inline Vec4 HsvaToRgba(Vec4 const& hsva) {
    float h = hsva._x, s = hsva._y, v = hsva._z;
    if (h >= 360.f) { h = 0.f; }
    float c = v * s;
    float hh = h / 60.f;
    
}
#endif

inline Vec4 RgbaToHsva(Vec4 rgba) {
    float r = rgba._x, g = rgba._y, b = rgba._z;

    float h = 0.f, s = 0.f, v = 0.f;

    float min, max, delta;

    min = r < g ? r : g;
    min = min < b ? min : b;

    max = r > g ? r : g;
    max = max  > b ? max : b;

    v = max; // v
    delta = max - min;
    if (delta < 0.00001f)
    {
        return Vec4(h, s, v, rgba._w); 
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        s = (delta / max); // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        s = 0.0;
        h = 0.f;                            // its now undefined
        return Vec4(h, s, v, rgba._w);
    }
    if( r >= max )            // > is bogus, just keeps compilor happy
        h = (g - b) / delta;        // between yellow & magenta
    else
    if(g >= max )
        h = 2.0 + (b - r) / delta;  // between cyan & yellow
    else
        h = 4.0 + (r - g) / delta;  // between magenta & cyan

    h *= 60.0;                              // degrees

    if(h < 0.0)
        h += 360.0;

    return Vec4(h, s, v, rgba._w);
}


inline Vec4 HsvaToRgba(Vec4 hsva)
{
    float h = hsva._x, s = hsva._y, v = hsva._z;
    float r = 0.f, g = 0.f, b = 0.f;

    double      hh, p, q, t, ff;
    long        i;

    if(s <= 0.0) {       // < is bogus, just shuts up warnings
        r = v;
        g = v;
        b = v;
        return Vec4(r, g, b, hsva._w);
    }
    hh = h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;

    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
    default:
        r = v;
        g = p;
        b = q;
        break;
    }
    return Vec4(r, g, b, hsva._w);     
}
