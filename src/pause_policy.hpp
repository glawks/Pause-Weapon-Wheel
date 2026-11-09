#pragma once

#include <cstdint>

namespace pause_policy {

// RAC1's visible-moby loop tests this bit before advancing an animation.
constexpr std::uint16_t kSkipAnimation = 0x0040;

// Known scenery-rendering moby classes from the RAC1 decompilation. Their
// updates register per-frame draw callbacks; skipping those updates makes
// water or textured level effects disappear. This is a conservative allowlist,
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

// The shared Gadgetron vendor (class 11) queues a draw callback in states
// 1/2, but its ordinary update also moves its rings and may open the shop.
// Route only the drawing while Quick Select owns the world gate. Class 1143,
// the vendor's ring, stays on the ordinary skipped-update path.
constexpr bool should_route_vendor_draw(bool wheel_active, bool keep_scenery,
                                        bool world_gate, bool draw_hook_installed,
                                        int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && oclass == 11;
}

// A vendor in state 0 has not spawned its ring yet; state 3 is its menu.
constexpr bool can_draw_vendor_effects(std::uint8_t state, std::uint32_t pvars,
                                      std::uint32_t ring) {
    return (state == 1 || state == 2) && pvars != 0 && ring != 0;
}

// The vendor renderer scales its beam by pvars+0x90. Its glow quad grows as
// phase increases but also loses alpha; phase zero is a point, not a visible
// glow. The callback itself adds 0.01, so seed a bright, nonzero-sized phase.
constexpr float vendor_full_glow_alpha_phase() {
    return 0.34f; // Draws at 0.35: bright and visibly expanded.
}

// Keep the vendor's original glow value for when Quick Select closes. Guard
// writes using the same level/array/slot/spawn/pvars identity so a recycled
// moby cannot inherit the preceding vendor's brightness.
struct VendorBrightnessHold {
    bool active = false;
    int level = -1;
    std::uint32_t array_base = 0;
    std::uint32_t moby = 0;
    std::uint64_t spawn_frame = 0;
    std::uint32_t pvars = 0;
    float saved_glow = 0.0f;

    void capture(int current_level, std::uint32_t current_array,
                 std::uint32_t current_moby, std::uint64_t current_spawn,
                 std::uint32_t current_pvars, float current_glow) {
        if (!active || level != current_level || array_base != current_array
            || moby != current_moby || spawn_frame != current_spawn
            || pvars != current_pvars) {
            active = true;
            level = current_level;
            array_base = current_array;
            moby = current_moby;
            spawn_frame = current_spawn;
            pvars = current_pvars;
            saved_glow = current_glow;
        }
    }

    template <typename Write>
    bool restore(int current_level, std::uint32_t current_array,
                 std::uint32_t current_moby, std::uint64_t current_spawn,
                 std::uint32_t current_pvars, Write write) const {
        if (!active || level != current_level || array_base != current_array
            || moby != current_moby || spawn_frame != current_spawn
            || pvars != current_pvars) {
            return false;
        }
        write(saved_glow);
        return true;
    }

    void reset() { active = false; }
};

template <typename Original, typename Draw>
void dispatch_vendor_update(bool draw_only, bool drawable, Original original, Draw draw) {
    if (!draw_only) {
        original();
    } else if (drawable) {
        draw();
    }
}

// These are guest code addresses of draw_vendor_effects, not native host
// pointers. NTSC-U's counterpart is verified in OpenRAC's carried source.
constexpr std::uint32_t vendor_effect_code_address(bool pal) {
    return pal ? 0x002A62D0u : 0x002A5040u;
}

template <typename Enqueue>
void enqueue_vendor_effects(bool pal, std::uint32_t moby, Enqueue enqueue) {
    enqueue(vendor_effect_code_address(pal), moby);
}

// A Nanotech crate (501) owns a separate glowing orb moby (806). The orb's
// full update follows the crate and can collect health, so never allow the
// original gameplay update just to preserve its visual draw callback.
constexpr bool should_route_nanotech_draw(bool wheel_active, bool keep_scenery,
                                         bool world_gate, bool draw_hook_installed,
                                         int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && oclass == 806;
}

// State 0 is uninitialized; states 3/4 are collecting/finished. The draw
// renderer reads the owner matrix even after the crate breaks (state 2).
constexpr bool can_draw_nanotech_orb(std::uint8_t state, std::uint32_t pvars,
                                    std::uint32_t owner, int owner_oclass) {
    return (state == 1 || state == 2) && pvars != 0 && owner != 0
           && owner_oclass == 501;
}

// Attached to a live crate (state 1), the retail update follows the crate
// and emits the short-lived particles that form the visible purple halo.
// Detached state 2 must NEVER execute its update while paused: that update
// can home in on Ratchet and trigger a health collection.
constexpr bool can_emit_attached_nanotech_glow(std::uint8_t state,
                                               std::uint32_t pvars,
                                               std::uint32_t owner,
                                               int owner_oclass,
                                               std::uint8_t owner_state) {
    return state == 1 && pvars != 0 && owner != 0
           && owner_oclass == 501 && owner_state < 0x80;
}

template <typename Original, typename Draw, typename Emit>
void dispatch_nanotech_visual_update(bool draw_only, bool drawable,
                                     bool emit_attached, Original original,
                                     Draw draw, Emit emit) {
    if (!draw_only) {
        original();
    } else if (drawable) {
        if (emit_attached) {
            emit(); // Original update queues its own draw: never queue twice.
        } else {
            draw();
        }
    }
}

template <typename Original, typename Draw>
void dispatch_nanotech_update(bool draw_only, bool drawable,
                              Original original, Draw draw) {
    if (!draw_only) {
        original();
    } else if (drawable) {
        draw();
    }
}

// Original Nanotech glow sphere renderer. PAL: func_L00_002DACC0;
// NTSC-U: FUN_L00_002D9810 from OpenRAC's carried source.
constexpr std::uint32_t nanotech_orb_code_address(bool pal) {
    return pal ? 0x002DACC0u : 0x002D9810u;
}

template <typename Enqueue>
void enqueue_nanotech_orb(bool pal, std::uint32_t moby, Enqueue enqueue) {
    enqueue(nanotech_orb_code_address(pal), moby);
}

// Two yellow Swingshot spheres are one class-803 target. Its state-1 update
// emits short-lived particles that form the glowing connection, but also
// steers the targets to face Ratchet. Keep it rendering with zero motion
// clocks; other states/classes remain subject to the world pause gate.
constexpr bool should_route_paired_swingshot_glow(bool wheel_active,
                                                  bool keep_scenery, bool world_gate,
                                                  bool effect_hook_installed,
                                                  int oclass, std::uint8_t state) {
    return wheel_active && keep_scenery && world_gate && effect_hook_installed
           && oclass == 803 && state == 1;
}

struct PairedSwingshotClocks {
    std::uint32_t frame_time;
    std::uint32_t frame_time_squared;
};

constexpr PairedSwingshotClocks paired_swingshot_motion_clock_addresses(bool pal) {
    return pal ? PairedSwingshotClocks{0x0015EE6Cu, 0x0015EE70u}
               : PairedSwingshotClocks{0x0015ED6Cu, 0x0015ED70u};
}


struct VendorPhaseAddresses {
    std::uint32_t sparkle_uv;
    std::uint32_t glow;
};

// The vendor's draw callback advances these globals unconditionally, even
// with zero frame delta. Restore the held values once per frame while paused.
constexpr VendorPhaseAddresses vendor_phase_addresses(bool pal) {
    return pal ? VendorPhaseAddresses{0x0016145Cu, 0x00161460u}
               : VendorPhaseAddresses{0x00161394u, 0x00161398u};
}

struct VendorVisualPhaseLock {
    bool active = false;
    int level = -1;
    float sparkle_uv = 0.0f;
    float glow = 0.0f;

    void capture(int current_level, float current_sparkle_uv, float current_glow) {
        if (!active || level != current_level) {
            active = true;
            level = current_level;
            sparkle_uv = current_sparkle_uv;
            glow = current_glow;
        }
    }

    template <typename Write>
    bool restore(int current_level, Write write) const {
        if (!active || level != current_level) {
            return false;
        }
        write(sparkle_uv, glow);
        return true;
    }

    void reset() {
        active = false;
        level = -1;
    }
};

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

// Restore both clocks even on errors. The zeroes are scoped to exactly one
// target update, leaving other game subsystems' timing unaffected.
template <typename ReadTime, typename WriteTime, typename ReadSquared,
          typename WriteSquared, typename Update>
void with_stopped_motion_clocks(ReadTime read_time, WriteTime write_time,
                                ReadSquared read_squared, WriteSquared write_squared,
                                Update update) {
    with_stopped_frame_time(read_time, write_time, [&] {
        with_stopped_frame_time(read_squared, write_squared, update);
    });
}

// Freeze the paired target's yaw and steering inertia even if the retail
// steering helper modifies them while particle effects are replenished.
template <typename ReadYaw, typename WriteYaw, typename ReadSteer,
          typename WriteSteer, typename Update>
void with_preserved_motion_pair(ReadYaw read_yaw, WriteYaw write_yaw,
                                ReadSteer read_steer, WriteSteer write_steer,
                                Update update) {
    const float yaw = read_yaw();
    const float steering = read_steer();
    try {
        update();
    } catch (...) {
        write_yaw(yaw);
        write_steer(steering);
        throw;
    }
    write_yaw(yaw);
    write_steer(steering);
}

constexpr std::uint16_t with_animation_stopped(std::uint16_t flags) {
    return static_cast<std::uint16_t>(flags | kSkipAnimation);
}

constexpr std::uint16_t with_animation_resumed(std::uint16_t flags) {
    return static_cast<std::uint16_t>(flags & ~kSkipAnimation);
}

} // namespace pause_policy
