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
    // The ship's transparent canopy is drawn by callbacks from classes
    // 0x212-0x215 (530-533), independently of the hull, on multiple levels.
    if (oclass >= 530 && oclass <= 533) {
        return true;
    }
    switch (level) {
    case 1:  // Novalis: ripple water and four static-strip water controllers
        return oclass == 751 || oclass == 676 || oclass == 678
               || oclass == 761 || oclass == 1225;
    case 2:  // Aridia: ocean/mud draws are enqueued by this class each frame
        return oclass == 854;
    case 5:  // Blackwater City: rising water + separate ocean grid callback
        return oclass == 831 || oclass == 879;
    case 6:  // Nebula G34: camera-selected dual-texture glass/quad callbacks
        return oclass == 1062;
    case 7:  // Umbris: rising-water grids + separate ocean draw callbacks
        return oclass == 902 || oclass == 941 || oclass == 943 || oclass == 944
               || (oclass >= 948 && oclass <= 955) || oclass == 958 || oclass == 961
               || oclass == 962 || (oclass >= 967 && oclass <= 971)
               || oclass == 1018;
    case 8:  // Batalia: water plane + independent scrolling textured scenery
        return oclass == 327 || oclass == 600 || oclass == 1649;
    case 9:  // Gaspar: level-specific ambient-stream controller queues lava drawing
        return oclass == 317;
    case 11: // Pokitaru: water tiles, ocean, and scrolling textured scenery
        return oclass == 1158 || oclass == 1111 || oclass == 1903;
    case 12: // Hoven: ripples, textured effects, reservoir, scrolling layers
        return oclass == 19 || oclass == 293 || oclass == 384 || oclass == 1901;
    case 13: // Gemlik Base
        return oclass == 1263 || oclass == 1393;
    case 14: // Oltanis: textured scenery, smoke/fire, rain, and lower liquid grid
        return oclass == 1418 || oclass == 760 || oclass == 1400
               || oclass == 1260;
    case 15: // Quartu: ripple-strip draws, including the electric-water room
        // Keep class 655's electric shock/damage/timer logic paused.
        return oclass == 1408 || oclass == 1409
               || oclass == 1565 || oclass == 1567;
    case 16: // Kalebo III: conveyor strips, ocean/horizon, reflective mesh draws
        return oclass == 471 || oclass == 1111 || oclass == 1943 || oclass == 1947
               || oclass == 1948 || oclass == 1953;
    default:
        return false;
    }
}

// Quartu's electric-water moby (655) both draws lightning and damages the
// hero. Route it through a draw-only hook while Quick Select is held, never
// through the regular scenery allowlist above. Other levels stay untouched.
constexpr bool should_route_quartu_electric_draw(bool wheel_active, bool keep_scenery,
                                                 bool world_gate, bool draw_hook_installed,
                                                 int level, int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && level == 15 && oclass == 655;
}

// The game initializes the light slot when first creating the arc geometry.
// Do not render an uninitialized effect or one whose on/off state is off.
constexpr bool can_draw_quartu_electric_arc(std::uint8_t state, std::uint32_t pvars,
                                           std::int32_t light_slot) {
    return state == 1 && pvars != 0 && light_slot >= 0;
}

// Intercepting the controller must *never* fall through to its shock/timer
// gameplay update. Only the original game draw callback is enqueued.
template <typename Original, typename Draw>
void dispatch_quartu_electric_update(bool draw_only, bool drawable,
                                    Original original, Draw draw) {
    if (!draw_only) {
        original();
    } else if (drawable) {
        draw();
    }
}

// The original draw-callback list stores a 32-bit *game code address*.
// Never pass Mod::function_at()'s native host function pointer here: the
// callback dispatcher will truncate it and try to call a nonexistent address.
constexpr std::uint32_t quartu_electric_bolt_code_address(bool pal) {
    return pal ? 0x002D9B00u : 0x002D8710u;
}

template <typename Enqueue>
void enqueue_quartu_electric_bolt(bool pal, std::uint32_t moby, Enqueue enqueue) {
    enqueue(quartu_electric_bolt_code_address(pal), moby);
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
