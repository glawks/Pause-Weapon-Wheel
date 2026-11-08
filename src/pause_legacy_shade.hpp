#pragma once

#include <algorithm>
#include <cstdint>

namespace pause_legacy_shade {

// Executable draw_rect_overlay / DrawRectOverlay_FiiiiUl in RAC1 NTSC and PAL.
// These are game code addresses, not host pointers. Look up the translated
// function through OpenRAC's mod API before calling it.
constexpr std::uint32_t kNtscDrawRectAddress = 0x001F52A0u;
constexpr std::uint32_t kPalDrawRectAddress = 0x001F5650u;
constexpr int kPictureWidth = 512;
constexpr int kPictureHeight = 448;

inline std::uint32_t game_rect_address(bool pal) {
    return pal ? kPalDrawRectAddress : kNtscDrawRectAddress;
}

// RAC1's colour packet uses ABGR with alpha 0x80 as full opacity. A black
// panel at 30% strength is 0x26000000. Draw before the wheel's own icons.
inline std::uint32_t colour_for_strength(int percent) {
    const auto clamped = std::clamp(percent, 0, 60);
    const auto alpha = (clamped * 128 + 50) / 100;
    return static_cast<std::uint32_t>(alpha) << 24;
}

inline bool should_draw(bool wheel_active, bool enabled, bool draw_function_available) {
    return wheel_active && enabled && draw_function_available;
}

} // namespace pause_legacy_shade
