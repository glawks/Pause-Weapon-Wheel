#pragma once

#include <cstdint>

namespace pause_policy {

// RAC1's visible-moby loop tests this bit before advancing an animation.
constexpr std::uint16_t kSkipAnimation = 0x0040;

// Known scenery-rendering moby classes from the RAC1 decompilation. Their
// updates register per-frame draw callbacks; skipping those updates makes
// water or textured level effects disappear. This is an experimental allowlist,
// not a promise that *all* visual effects are covered.
constexpr bool needs_update_for_water(int level, int oclass) {
    switch (level) {
    case 1:  // Novalis: ripple water and four static-strip water controllers
        return oclass == 751 || oclass == 676 || oclass == 678
               || oclass == 761 || oclass == 1225;
    case 5:  // Rilgar
        return oclass == 831;
    case 7:  // Umbris
        return oclass == 902 || oclass == 941 || oclass == 943 || oclass == 944
               || (oclass >= 948 && oclass <= 955) || oclass == 958 || oclass == 961
               || oclass == 962 || (oclass >= 967 && oclass <= 971);
    case 11: // Pokitaru
        return oclass == 1158;
    case 12: // Hoven: ripples, textured effects, reservoir, scrolling layers
        return oclass == 19 || oclass == 293 || oclass == 384 || oclass == 1901;
    case 13: // Gemlik Base
        return oclass == 1263 || oclass == 1393;
    case 14: // Oltanis: textured scenery, smoke/fire, rain, and lower liquid grid
        return oclass == 1418 || oclass == 760 || oclass == 1400
               || oclass == 1260;
    default:
        return false;
    }
}

// The Hoven renderer must keep executing to enqueue its draw, but its texture
// phases should not advance while Quick Select is holding the world still.
constexpr bool should_hold_hoven_scroll(bool wheel_active, int level,
                                       bool keep_scenery, bool hold_scroll) {
    return wheel_active && level == 12 && keep_scenery && hold_scroll;
}

// Limit a shared game-clock override to the duration of a single moby update.
// This preserves callback registration without pausing unrelated game code.
template <typename ReadTime, typename WriteTime, typename Update>
void with_stopped_frame_time(ReadTime read_time, WriteTime write_time, Update update) {
    const float saved = read_time();
    write_time(0.0f);
    try {
        update();
    } catch (...) {
        write_time(saved);
        throw;
    }
    write_time(saved);
}

constexpr std::uint16_t with_animation_stopped(std::uint16_t flags) {
    return static_cast<std::uint16_t>(flags | kSkipAnimation);
}

constexpr std::uint16_t with_animation_resumed(std::uint16_t flags) {
    return static_cast<std::uint16_t>(flags & ~kSkipAnimation);
}

} // namespace pause_policy
