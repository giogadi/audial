#pragma once

#include "enums/ColorPreset.h"

inline Vec3 ToColor3(ColorPreset preset) {
    switch (preset) {
        case ColorPreset::White: return Vec3(1.f, 1.f, 1.f);
        case ColorPreset::Black: return Vec3(0.f, 0.f, 0.f);
        case ColorPreset::Red: return Vec3(1.f, 0.f, 0.f);
        case ColorPreset::Orange: return Vec3(1.f, 0.647f, 0.f);
        case ColorPreset::Yellow: return Vec3(1.f, 0.f, 1.f);
        case ColorPreset::Green: return Vec3(0.f, 1.f, 0.f);
        case ColorPreset::Blue: return Vec3(0.f, 0.f, 1.f);
        case ColorPreset::Magenta: return Vec3(1.f, 0.f, 1.f);
        case ColorPreset::Cyan: return Vec3(0.f, 1.f, 1.f);
        case ColorPreset::Count: assert(false); return Vec3();
    }
}

inline Vec4 ToColor4(ColorPreset preset) {
    Vec3 color3 = ToColor3(preset);
    return Vec4(color3._x, color3._y, color3._z, 1.f);
}
