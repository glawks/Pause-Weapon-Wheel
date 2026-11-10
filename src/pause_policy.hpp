#pragma once

#include <array>
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
    case 17: // Veldin Orbit: water ripple strips and reflective glass meshes
        // The ripple-strip update queues a water mesh in state 1; the
        // reflection mobys only queue their mesh callbacks. No gameplay
        // controller or electric-water hazard is exempted here.
        return oclass == 1405 || oclass == 1406 || oclass == 1407 || oclass == 1426
               || oclass == 1907 || oclass == 1908 || oclass == 1910
               || oclass == 1911 || oclass == 1913 || oclass == 1918;
    default:
        return false;
    }
}

// Veldin Orbit's yellow doorway spark field (class 78) and exterior air-flow
// veil (class 1378) both register textured-quad callbacks on each update.
// Their normal updates also spawn particles, manage linked mobys, or modify
// Ratchet's updraft state. Never exempt those full gameplay updates.
constexpr bool should_route_veldin_orbit_doorway_draw(bool wheel_active,
                                                      bool keep_scenery, bool world_gate,
                                                      bool draw_hook_installed,
                                                      int level, int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && level == 17 && (oclass == 78 || oclass == 1378);
}

// The spark emitter draws in state 1 (emitting) or state 2 (fading). The
// air-flow veil draws only in active state 1. Both use their initialized pvars.
constexpr bool can_draw_veldin_orbit_doorway(int oclass, std::uint8_t state,
                                            std::uint32_t pvars) {
    return pvars != 0 && ((oclass == 78 && (state == 1 || state == 2))
                          || (oclass == 1378 && state == 1));
}

// Original level-17 guest draw routines, corroborated by both decompilations:
// PAL func_L15_002A4318 / func_L10_002EA578, and NTSC-U counterparts.
constexpr std::uint32_t veldin_orbit_doorway_draw_code_address(bool pal, int oclass) {
    if (oclass == 78) {
        return pal ? 0x002A9AD0u : 0x002A8808u;
    }
    if (oclass == 1378) {
        return pal ? 0x002EB288u : 0x002E9798u;
    }
    return 0u;
}

template <typename Enqueue>
void enqueue_veldin_orbit_doorway(bool pal, int oclass, std::uint32_t moby,
                                 Enqueue enqueue) {
    enqueue(veldin_orbit_doorway_draw_code_address(pal, oclass), moby);
}

// Kalebo III's purple horizontal doorway wall is a group of class-552
// linked-pair ribbons. Its state-1 leader update registers the draw callback
// and maintains sound; state 0 initializes/links mobys, so never run it
// during a pause. Flag 0x2 marks linked followers, not group leaders.
constexpr bool should_route_kalebo_wall_ribbons(bool wheel_active,
                                                 bool keep_scenery, bool world_gate,
                                                 bool effect_hook_installed,
                                                 int level, int oclass,
                                                 std::uint8_t state,
                                                 std::uint16_t flags) {
    return wheel_active && keep_scenery && world_gate && effect_hook_installed
           && level == 16 && oclass == 552 && state == 1 && (flags & 0x2u) == 0;
}

// Drek's Fleet (Veldin Orbit) class 99 creates red scanning beams. Its normal
// update both registers draw_fleet_beam_quad and performs collision/hit tests,
// so exempt it only when a draw-only hook can bypass the gameplay update.
constexpr bool should_route_fleet_laser_draw(bool wheel_active,
                                            bool keep_scenery, bool world_gate,
                                            bool draw_hook_installed,
                                            int level, int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && level == 17 && oclass == 99;
}

// State 0 has not calculated the beam endpoints, and state 3 is disabled.
// The active states need allocated pvars and a nonzero beam width.
constexpr bool can_draw_fleet_laser(std::uint8_t state, std::uint32_t pvars,
                                   float width) {
    return (state == 1 || state == 2) && pvars != 0 && width > 0.0f;
}

// Guest callback addresses from the PAL/NTSC-U Veldin Orbit decompilation.
constexpr std::uint32_t fleet_laser_draw_code_address(bool pal) {
    return pal ? 0x002AA880u : 0x002A95B8u;
}

template <typename Enqueue>
void enqueue_fleet_laser(bool pal, std::uint32_t moby, Enqueue enqueue) {
    enqueue(fleet_laser_draw_code_address(pal), moby);
}

// Drek's Fleet's distant blue-white beam and purple planet impact glow are
// rendered by class 1772, not the separate class-99 corridor laser hazard.
// Its ordinary update changes UV animation and may change the hero's fall
// state, so draw from initialized pvars without running the gameplay update.
constexpr bool should_route_fleet_backdrop_draw(bool wheel_active,
                                                bool keep_scenery, bool world_gate,
                                                bool draw_hook_installed,
                                                int level, int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && level == 17 && oclass == 1772;
}

// State 0 still needs to calculate the anchor and distant endpoint.
constexpr bool can_draw_fleet_backdrop(std::uint8_t state, std::uint32_t pvars) {
    return state == 1 && pvars != 0;
}

// The original level-17 renderer draws both scrolling beam quads and all
// five impact-glow quads. These are guest code addresses, not host pointers.
constexpr std::uint32_t fleet_backdrop_draw_code_address(bool pal) {
    return pal ? 0x002F4E30u : 0x002F32F0u;
}

template <typename Enqueue>
void enqueue_fleet_backdrop(bool pal, std::uint32_t moby, Enqueue enqueue) {
    enqueue(fleet_backdrop_draw_code_address(pal), moby);
}

// Final Veldin (level 18), not Veldin Orbit: class 1392 queues the green
// Trespasser barrier's layered glow quads. Its normal update also checks a
// linked pad and can change collision, state and sound; only keep its draw.
constexpr bool should_route_veldin_final_barrier_draw(bool wheel_active,
                                                      bool keep_scenery, bool world_gate,
                                                      bool draw_hook_installed,
                                                      int level, int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && level == 18 && oclass == 1392;
}

// State 0 initializes opacity; state 2 is deactivated and fades out.
constexpr bool can_draw_veldin_final_barrier(std::uint8_t state, std::uint32_t pvars) {
    return state == 1 && pvars != 0;
}

constexpr std::uint32_t veldin_final_barrier_draw_code_address(bool pal) {
    return pal ? 0x002F33D8u : 0x002F1FD8u;
}

template <typename Enqueue>
void enqueue_veldin_final_barrier(bool pal, std::uint32_t moby, Enqueue enqueue) {
    enqueue(veldin_final_barrier_draw_code_address(pal), moby);
}

// Final Veldin's class-1402 lift/shaft renders three different textured
// meshes. Its normal update also moves the lift and linked mobys, changes
// commands and advances the texture's scroll offset. Queue only the existing
// mesh renderer while the world is paused; never allow the gameplay update.
constexpr bool should_route_veldin_final_lift_draw(bool wheel_active,
                                                   bool keep_scenery, bool world_gate,
                                                   bool draw_hook_installed,
                                                   int level, int oclass) {
    return wheel_active && keep_scenery && world_gate && draw_hook_installed
           && level == 18 && oclass == 1402;
}

// States 2/3 have selected an initial height. State 0 has not initialized
// the lift, while state 1 has not yet chosen a position for its mesh.
constexpr bool can_draw_veldin_final_lift(std::uint8_t state, std::uint32_t pvars,
                                         std::int32_t mesh_mode) {
    return (state == 2 || state == 3) && pvars != 0
           && mesh_mode >= 0 && mesh_mode <= 2;
}

// Guest code addresses verified against NTSC-U Lombyte and PAL RAC1 decomp.
// Mode is the selector at lift pvars+0x14; modes 0/1/2 draw 25/38/22 quads.
constexpr std::uint32_t veldin_final_lift_draw_code_address(bool pal,
                                                            std::int32_t mesh_mode) {
    switch (mesh_mode) {
    case 0: return pal ? 0x002F3A80u : 0x002F2620u;
    case 1: return pal ? 0x002F3C28u : 0x002F27C8u;
    case 2: return pal ? 0x002F3DD0u : 0x002F2970u;
    default: return 0u;
    }
}

template <typename Enqueue>
void enqueue_veldin_final_lift(bool pal, std::int32_t mesh_mode,
                               std::uint32_t moby, Enqueue enqueue) {
    const auto callback = veldin_final_lift_draw_code_address(pal, mesh_mode);
    if (callback != 0u) {
        enqueue(callback, moby);
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

// The real vendor update sets up ring geometry as well as submitting its
// draw. Reuse that setup in its non-interactive state-1 branch while paused.
// Its only transition out of state 1 is a proximity test every eight ticks;
// make the frame count non-multiple-of-eight for this one call, and restore
// the real state/counter immediately afterward. State 2's shop input checks
// are never reached. State 1 subtracts 0.1 from the ring glow: seed it at
// 1.1 so that the renderer still observes full strength (1.0).
template <typename ReadState, typename WriteState, typename ReadFrame,
          typename WriteFrame, typename WriteGlow, typename Update>
void with_vendor_visual_state(ReadState read_state, WriteState write_state,
                              ReadFrame read_frame, WriteFrame write_frame,
                              WriteGlow write_glow, Update update) {
    const std::uint8_t original_state = read_state();
    const std::uint32_t original_frame = read_frame();
    write_state(1u);
    write_frame(original_frame | 1u);
    write_glow(1.1f);
    try {
        update();
    } catch (...) {
        write_frame(original_frame);
        write_state(original_state);
        write_glow(1.0f);
        throw;
    }
    write_frame(original_frame);
    write_state(original_state);
    write_glow(1.0f);
}

// vendor_update writes two 4-float ring quaternions and recalculates the
// class-1143 sphere's scale even with motion clocks stopped. Put the original
// pose AND size back before rebuilding its cached matrix so the floating
// vendor does not visibly shrink when Quick Select opens.
template <typename Read, typename Write, typename ReadScale,
          typename WriteScale, typename Update, typename Rebuild>
void with_vendor_ring_pose_held(Read read, Write write,
                                ReadScale read_scale, WriteScale write_scale,
                                Update update, Rebuild rebuild) {
    float saved[8]{};
    for (int i = 0; i < 8; ++i) {
        saved[i] = read(i);
    }
    const float saved_scale = read_scale();
    const auto restore = [&] {
        for (int i = 0; i < 8; ++i) {
            write(i, saved[i]);
        }
        write_scale(saved_scale);
        rebuild();
    };
    try {
        update();
    } catch (...) {
        restore();
        throw;
    }
    restore();
}

// The game's MobyBuildMatrix routine is shared across overlays, but its
// *guest address* changes for every level. Never pass a level-00 address to
// function_at() while another overlay is loaded: it can return a callable
// proxy that logs "no function of the port" when invoked. NTSC-U addresses
// are from Lombyte's US shared-function map (FUN_L00_00250df8); PAL addresses
// are from rac1-decomp's PAL overlay map (func_L00_00251E30).
constexpr std::uint32_t vendor_matrix_rebuild_code_address(bool pal, int level) {
    constexpr std::array<std::uint32_t, 19> ntsc = {
        0x00250DF8u, 0x00265BD8u, 0x00253468u, 0x0023F968u, 0x00243B30u,
        0x0027B060u, 0x0025FAB8u, 0x00279820u, 0x0025B2F8u, 0x00270780u,
        0x00243870u, 0x00276C28u, 0x00269B38u, 0x0025CA80u, 0x002592F8u,
        0x00240460u, 0x0024C4D0u, 0x0024B5B0u, 0x002530E0u
    };
    constexpr std::array<std::uint32_t, 19> pal_addresses = {
        0x00251E30u, 0x00266B88u, 0x00254440u, 0x00240908u, 0x00244AE8u,
        0x0027C090u, 0x00260AC0u, 0x0027A7D8u, 0x0025C2A8u, 0x002716B8u,
        0x00244828u, 0x00277BD0u, 0x0026AAF0u, 0x0025DA38u, 0x0025A330u,
        0x00241428u, 0x0024D470u, 0x0024C660u, 0x002540A8u
    };
    if (level < 0 || level >= static_cast<int>(ntsc.size())) {
        return 0;
    }
    return pal ? pal_addresses[static_cast<std::size_t>(level)]
               : ntsc[static_cast<std::size_t>(level)];
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

// State 0 is uninitialized; states 3/4 are collecting/finished. Once an orb
// detaches in state 2, its own pvars are sufficient for the renderer even
// if the destroyed crate has been removed from the active moby array.
constexpr bool can_draw_nanotech_orb(std::uint8_t state, std::uint32_t pvars,
                                    std::uint32_t owner, int owner_oclass) {
    return pvars != 0 && (state == 2 || (state == 1 && owner != 0
                                        && owner_oclass == 501));
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

// State 2 contains the same orb/particle renderer as state 1, but also a
// health-pickup branch. Only refresh it after the primary orb pointer has
// been validated so that pointer can be masked for the scoped update.
constexpr bool can_emit_detached_nanotech_glow(std::uint8_t state,
                                               std::uint32_t pvars,
                                               std::uint32_t primary,
                                               int primary_class) {
    return state == 2 && pvars != 0 && primary != 0 && primary_class == 806;
}

// Read-only diagnostic for identifying which gate rejects a paused class-806
// orb. This never grants an update or mutates a guest pointer/state.
enum class NanotechPauseProbe : std::uint8_t {
    NotDrawable,
    AttachedRefresh,
    AttachedFallback,
    DetachedRefresh,
    DetachedNoSlot,
    DetachedNoPrimary,
    DetachedOutsideArray,
    DetachedWrongClass,
    Count
};

constexpr NanotechPauseProbe nanotech_pause_probe(bool drawable,
                                                  std::uint8_t state,
                                                  bool attached_refresh,
                                                  std::uint32_t primary_slot,
                                                  std::uint32_t primary,
                                                  bool primary_active,
                                                  int primary_class) {
    if (!drawable) return NanotechPauseProbe::NotDrawable;
    if (state == 1) return attached_refresh
        ? NanotechPauseProbe::AttachedRefresh : NanotechPauseProbe::AttachedFallback;
    if (state != 2) return NanotechPauseProbe::NotDrawable;
    if (primary_slot == 0) return NanotechPauseProbe::DetachedNoSlot;
    if (primary == 0) return NanotechPauseProbe::DetachedNoPrimary;
    if (!primary_active) return NanotechPauseProbe::DetachedOutsideArray;
    if (primary_class != 806) return NanotechPauseProbe::DetachedWrongClass;
    return NanotechPauseProbe::DetachedRefresh;
}

// Keep all temporary guest-memory changes scoped to one original update,
// including an exception from the original (which must never leave the
// primary-orb pointer cleared or the orb in a moved position).
template <std::size_t N, typename Read, typename Write, typename Update>
void with_restored_guest_words(const std::array<std::uint32_t, N>& addresses,
                               Read read, Write write, Update update) {
    std::array<std::uint32_t, N> saved{};
    for (std::size_t i = 0; i < N; ++i) {
        saved[i] = read(addresses[i]);
    }
    const auto restore = [&] {
        for (std::size_t i = 0; i < N; ++i) {
            write(addresses[i], saved[i]);
        }
    };
    try {
        update();
    } catch (...) {
        restore();
        throw;
    }
    restore();
}

// The game's Nanotech update accesses these overlay globals directly.
// NTSC-U D_L00_0015F63C is NOT computed by applying core.lit's relocation
// delta to the PAL primary_nanotech_ball_moby address.
constexpr std::uint32_t nanotech_primary_moby_global(bool pal) {
    return pal ? 0x0015F71Cu : 0x0015F63Cu;
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

// Original Nanotech glow sphere renderer (func_L00_002DACC0). Its code
// address changes with each loaded overlay. These per-level addresses come
// from rac1-decomp's PAL/US functions.tsv shared-function mappings.
constexpr std::uint32_t nanotech_orb_code_address(bool pal, int level) {
    constexpr std::array<std::uint32_t, 19> ntsc = {
        0x002D9810u, 0x00301C00u, 0x002E4280u, 0x002D2B20u, 0x002DB9A0u,
        0x0030B820u, 0x002E80C0u, 0x00304180u, 0x002FB390u, 0x002FAAC0u,
        0x002CBC00u, 0x00301F58u, 0x002F9A70u, 0x002FC650u, 0x002F5B70u,
        0x002DD048u, 0x002D83C0u, 0x002DB4F0u, 0x002E39A0u
    };
    constexpr std::array<std::uint32_t, 19> pal_addresses = {
        0x002DACC0u, 0x00302FD8u, 0x002E56B8u, 0x002D3EE8u, 0x002DCD80u,
        0x0030CC78u, 0x002E94F0u, 0x00305560u, 0x002FC850u, 0x002FBE70u,
        0x002CCFC0u, 0x00303328u, 0x002FAE50u, 0x002FDAE8u, 0x002F6FF8u,
        0x002DE438u, 0x002D9788u, 0x002DC9C8u, 0x002E4D90u
    };
    if (level < 0 || level >= static_cast<int>(ntsc.size())) {
        return 0;
    }
    return pal ? pal_addresses[static_cast<std::size_t>(level)]
               : ntsc[static_cast<std::size_t>(level)];
}

template <typename Enqueue>
void enqueue_nanotech_orb(bool pal, int level, std::uint32_t moby, Enqueue enqueue) {
    const auto address = nanotech_orb_code_address(pal, level);
    if (address != 0) {
        enqueue(address, moby);
    }
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
