#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace pause_veil {

// OpenRAC's mod overlay uses a 512x448 picture independent of window size.
constexpr float kPictureWidth = 512.0f;
constexpr float kPictureHeight = 448.0f;

// The Quick Select wheel sits to the left of the centre of the RAC1 picture.
// Keep a generous elliptical area around all eight slots untouched.
constexpr float kWheelX = 92.0f;
constexpr float kWheelY = 220.0f;
constexpr float kWheelRadiusX = 90.0f;
constexpr float kWheelRadiusY = 111.0f;
constexpr std::uint32_t kVeilColourRgb = 0x121B2Au;

// This is a feathered translucent shade, NOT an image-space blur: the SDK's
// standard overlay exposes coloured rectangles, not a sampled framebuffer.
inline std::uint8_t alpha_at(float x, float y, int strength_percent) {
    const int strength = std::clamp(strength_percent, 0, 60);
    if (strength == 0) {
        return 0;
    }

    const float dx = (x - kWheelX) / kWheelRadiusX;
    const float dy = (y - kWheelY) / kWheelRadiusY;
    const float ellipse_radius = std::sqrt(dx * dx + dy * dy);
    if (ellipse_radius <= 1.0f) {
        return 0;
    }

    // Over roughly 24% of the wheel radius, fade the shade in smoothly.
    const float t = std::clamp((ellipse_radius - 1.0f) / 0.24f, 0.0f, 1.0f);
    const float feather = t * t * (3.0f - 2.0f * t);
    const int alpha = static_cast<int>(std::lround(strength * 2.55f * feather));
    // Fewer alpha bands give the overlay renderer fewer rectangles to draw.
    return static_cast<std::uint8_t>(((alpha + 4) / 8) * 8);
}

template <class Draw>
void for_each_rect(int strength_percent, Draw draw) {
    if (strength_percent <= 0) {
        return;
    }

    // Merge horizontally adjacent samples with the same alpha. This produces
    // a feathered wheel-shaped cutout without thousands of full-size quads.
    constexpr int kStep = 4;
    constexpr int kWidth = static_cast<int>(kPictureWidth);
    constexpr int kHeight = static_cast<int>(kPictureHeight);
    for (int y = 0; y < kHeight; y += kStep) {
        int run_begin = 0;
        int previous_alpha = -1;
        for (int x = 0; x <= kWidth; x += kStep) {
            const int next_alpha = x == kWidth
                                       ? -1
                                       : alpha_at(static_cast<float>(x + kStep / 2),
                                                  static_cast<float>(y + kStep / 2),
                                                  strength_percent);
            if (next_alpha == previous_alpha) {
                continue;
            }
            if (previous_alpha > 0) {
                const auto rgba = (kVeilColourRgb << 8)
                                  | static_cast<std::uint32_t>(previous_alpha);
                draw(static_cast<float>(run_begin), static_cast<float>(y),
                     static_cast<float>(x - run_begin), static_cast<float>(kStep), rgba);
            }
            previous_alpha = next_alpha;
            run_begin = x;
        }
    }
}

} // namespace pause_veil
