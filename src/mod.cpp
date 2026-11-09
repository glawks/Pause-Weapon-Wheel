#include "openrac/mod.hpp"
#include "openrac/games/rac1.hpp"
#include "pause_legacy_shade.hpp"
#include "pause_policy.hpp"
#include "pause_veil.hpp"

#include <atomic>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace {

constexpr openrac::gaddr kPalMobyArrayGlobal = 0x00160018u;
constexpr openrac::gaddr kNtscMobyArrayGlobal = 0x0015FF18u;
constexpr openrac::gaddr kPalFrameTimeGlobal = 0x0015EE7Cu;
constexpr openrac::gaddr kNtscFrameTimeGlobal = 0x0015ED7Cu;
constexpr openrac::gaddr kMobySize = 0x100u;
constexpr openrac::gaddr kMobyFlags = 0x34u;
constexpr openrac::gaddr kMobySpawnFrame = 0x38u;
constexpr openrac::gaddr kMobyClass = 0xA6u;
constexpr openrac::gaddr kMobyState = 0x20u;
constexpr openrac::gaddr kMobyPvars = 0x78u;
constexpr openrac::gaddr kQuartuElectricLightSlot = 0x120u;
constexpr openrac::gaddr kVendorRingPtr = 0x0Cu;
constexpr openrac::gaddr kVendorGlowStrength = 0x90u;
constexpr openrac::gaddr kNanotechOwnerPtr = 0x0Cu;
constexpr openrac::gaddr kNanotechBobOffset = 0x00u;
constexpr openrac::gaddr kNanotechBobPhase = 0x04u;
constexpr openrac::gaddr kMobyYaw = 0x48u;
constexpr openrac::gaddr kPairedTargetSteeringVelocity = 0x38u;

// Game-code addresses from the RAC1 PAL and NTSC-U decompilations. Both
// callbacks are resolved for the current loaded level; nothing is patched in
// OpenRAC itself. The electrical bolt draw function uses initialized geometry
// but does not execute the controller's shock/timer update.
constexpr openrac::gaddr kPalAddDrawCallback = 0x001F49B0u;
constexpr openrac::gaddr kNtscAddDrawCallback = 0x001F4600u;
// This native pointer type is ONLY for checking that the draw function exists.
// The game's callback queue takes two 32-bit guest addresses, not host pointers.
using GameDrawEffectFn = void (*)(void*);
using GameAddDrawCallbackFn = void (*)(std::uint32_t, std::uint32_t);

struct FrozenAnimation {
    int level;
    openrac::gaddr array_base;
    openrac::gaddr moby;
    std::uint64_t spawn_frame;
    std::int16_t oclass;
};

openrac::Mod* game_mod = nullptr;
bool wheel_active = false;
std::atomic<bool> wheel_visible_for_overlay{false};
bool wheel_seen_this_tick = false;
bool world_gate = false;
bool quartu_electric_draw_hook = false;
bool quartu_electric_first_draw_logged = false;
bool quartu_electric_missing_draw_warned = false;
bool vendor_draw_hook = false;
bool vendor_first_draw_logged = false;
bool vendor_missing_draw_warned = false;
bool nanotech_draw_hook = false;
bool nanotech_first_draw_logged = false;
bool nanotech_missing_draw_warned = false;
bool nanotech_missing_clocks_warned = false;
bool nanotech_first_particle_refresh_logged = false;
bool paired_target_effect_hook = false;
bool paired_target_first_effect_logged = false;
bool paired_target_missing_clocks_warned = false;
pause_policy::VendorVisualPhaseLock vendor_phases;
pause_policy::VendorBrightnessHold vendor_brightness;
bool freeze_animations = true;
bool keep_water = true;
bool hold_hoven_scroll = true;
bool diagnostic_log = true;
bool soften_background = true;
int background_softening_strength = 30;
using GameDrawRectFn = void (*)(std::int32_t, std::int32_t, std::int32_t,
                                std::int32_t, std::uint64_t);
GameDrawRectFn legacy_draw_rect = nullptr;
bool legacy_draw_rect_warning_logged = false;
bool legacy_first_draw_logged = false;
openrac::gaddr moby_array_global = 0;
openrac::gaddr frame_time_global = 0;
std::vector<FrozenAnimation> flagged_mobys;
int observed_level = -1;
std::set<int> observed_classes;
std::set<int> exempted_classes;
int vendor_visual_draw_count = 0;
int nanotech_attached_glow_count = 0;
int nanotech_visual_draw_count = 0;

void log_class_list(const char* label, const std::set<int>& classes) {
    std::string line = std::string("pause diagnostic ") + label + ":";
    for (const auto oclass : classes) {
        const auto entry = " " + std::to_string(oclass);
        if (line.size() + entry.size() > 220) {
            game_mod->info(line);
            line = std::string("pause diagnostic ") + label + " (cont):";
        }
        line += entry;
    }
    game_mod->info(line);
}

void report_pause_diagnostics() {
    if (diagnostic_log && observed_level >= 0) {
        game_mod->info("pause diagnostic: level=" + std::to_string(observed_level)
                       + ", seen=" + std::to_string(observed_classes.size())
                       + " distinct moby classes, exempted="
                       + std::to_string(exempted_classes.size()));
        log_class_list("seen", observed_classes);
        log_class_list("exempted", exempted_classes);
        game_mod->info("pause diagnostic visual updates: vendor_draw="
                       + std::to_string(vendor_visual_draw_count)
                       + ", nanotech_halo=" + std::to_string(nanotech_attached_glow_count)
                       + ", nanotech_draw_only=" + std::to_string(nanotech_visual_draw_count));
    }
    observed_level = -1;
    observed_classes.clear();
    exempted_classes.clear();
    vendor_visual_draw_count = 0;
    nanotech_attached_glow_count = 0;
    nanotech_visual_draw_count = 0;
}

// OpenRAC 1.12 does not define openrac_overlay or Mod::on_overlay. Keep the
// standalone mod buildable with that SDK; 1.14+ adds the optional renderer.
#if OPENRAC_MOD_API_MINOR >= 14
// The standard mod overlay is drawn after the game's HUD. Leave an elliptical
// opening around Quick Select so its icons stay bright; shade only the rest.
// The renderer API exposes rectangles, not a sampled-texture blur operation.
void draw_background_softening(void*, const openrac_overlay* overlay) {
    if (!wheel_visible_for_overlay.load(std::memory_order_relaxed)
        || overlay == nullptr || overlay->rect == nullptr) {
        return;
    }
    pause_veil::for_each_rect(background_softening_strength,
                             [&](float x, float y, float w, float h, std::uint32_t rgba) {
                                 overlay->rect(overlay->context, x, y, w, h, rgba);
                             });
}
#endif

// The game's active moby array can move between levels and after a respawn.
// Read the global through the SDK's relocation facility instead of caching
// an address across a level load.
openrac::gaddr current_moby_array() {
    if (game_mod == nullptr || game_mod->loaded_level() < 0) {
        return 0;
    }
    const auto location = game_mod->relocate(moby_array_global);
    return location != 0 ? game_mod->memory().read<openrac::gaddr>(location) : 0;
}

openrac::gaddr moby_at(int slot, openrac::gaddr array_base) {
    // The gate supplies game-owned slots, but avoid arithmetic on a bad slot.
    if (slot < 0 || slot >= 32768 || array_base == 0) {
        return 0;
    }
    return array_base + static_cast<openrac::gaddr>(slot) * kMobySize;
}

bool is_moby_in_active_array(openrac::gaddr address, openrac::gaddr array_base) {
    return array_base != 0 && address >= array_base
           && (address - array_base) % kMobySize == 0
           && (address - array_base) / kMobySize < 32768;
}

void stop_animation(openrac::gaddr moby, openrac::gaddr array_base, int level,
                    std::int16_t oclass) {
    auto memory = game_mod->memory();
    const auto flags = memory.read<std::uint16_t>(moby + kMobyFlags);
    if ((flags & pause_policy::kSkipAnimation) != 0) {
        return; // The game already owned this bit: never clear it later.
    }

    flagged_mobys.push_back(FrozenAnimation{
        level, array_base, moby,
        memory.read<std::uint64_t>(moby + kMobySpawnFrame), oclass
    });
    memory.write<std::uint16_t>(moby + kMobyFlags,
                                pause_policy::with_animation_stopped(flags));
}

void restore_animations() {
    if (game_mod == nullptr || flagged_mobys.empty()) {
        return;
    }

    const int level = game_mod->loaded_level();
    const auto array_base = current_moby_array();
    auto memory = game_mod->memory();
    for (const auto& saved : flagged_mobys) {
        // Never alter a different level's moby or a new occupant of the slot.
        if (level != saved.level || array_base != saved.array_base
            || memory.read<std::uint64_t>(saved.moby + kMobySpawnFrame) != saved.spawn_frame
            || memory.read<std::int16_t>(saved.moby + kMobyClass) != saved.oclass) {
            continue;
        }
        const auto flags = memory.read<std::uint16_t>(saved.moby + kMobyFlags);
        memory.write<std::uint16_t>(saved.moby + kMobyFlags,
                                    pause_policy::with_animation_resumed(flags));
    }
    flagged_mobys.clear();
}

int gate_moby_update(void*, int slot, int phase) {
    if (phase != OPENRAC_MOBY_BEFORE || !wheel_active) {
        return 0;
    }

    const auto array_base = current_moby_array();
    const auto moby = moby_at(slot, array_base);
    if (moby == 0) {
        // The original mod skipped updates even when the game exposes no array.
        return 1;
    }

    const auto oclass = game_mod->memory().read<std::int16_t>(moby + kMobyClass);
    const int level = game_mod->loaded_level();
    if (diagnostic_log) {
        if (observed_level != level) {
            observed_classes.clear();
            exempted_classes.clear();
            observed_level = level;
        }
        observed_classes.insert(oclass);
    }
    const auto state = oclass == 803
        ? game_mod->memory().read<std::uint8_t>(moby + kMobyState) : 0u;
    const bool nanotech_draw_only = pause_policy::should_route_nanotech_draw(
        wheel_active, keep_water, world_gate, nanotech_draw_hook, oclass);
    const bool paired_target_effects = pause_policy::should_route_paired_swingshot_glow(
        wheel_active, keep_water, world_gate, paired_target_effect_hook, oclass, state);
    const bool vendor_draw_only = pause_policy::should_route_vendor_draw(
        wheel_active, keep_water, world_gate, vendor_draw_hook, oclass);
    if (keep_water && (pause_policy::needs_update_for_water(level, oclass)
                       || vendor_draw_only
                       || nanotech_draw_only || paired_target_effects
                       || pause_policy::should_route_quartu_electric_draw(
                           wheel_active, keep_water, world_gate,
                           quartu_electric_draw_hook, level, oclass))) {
        if (diagnostic_log) {
            exempted_classes.insert(oclass);
        }
        // Class 655 and the class-806 health orb use draw-only hooks.
        // Class 803 instead runs just enough of its effect update to emit
        // the yellow link particles with its motion clocks stopped.
        if (vendor_draw_only || nanotech_draw_only || paired_target_effects) {
            // The visible moby animation can advance earlier in the game
            // loop. Always hold it even if general animation freeze is off.
            stop_animation(moby, array_base, level, oclass);
        }
        return 0;
    }

    if (freeze_animations) {
        stop_animation(moby, array_base, level, oclass);
    }
    return 1; // Skip its gameplay update, as in the original mod.
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1") {
        throw openrac::Error("Weapon Wheel Pause is for Ratchet & Clank 1");
    }
    if (mod.version() != "pal" && mod.version() != "ntsc") {
        throw openrac::Error("Only RAC1 PAL and NTSC-U are supported");
    }

    game_mod = &mod;
    wheel_active = false;
    wheel_visible_for_overlay.store(false, std::memory_order_relaxed);
    legacy_draw_rect = nullptr;
    legacy_draw_rect_warning_logged = false;
    legacy_first_draw_logged = false;
    wheel_seen_this_tick = false;
    world_gate = false;
    quartu_electric_draw_hook = false;
    quartu_electric_first_draw_logged = false;
    quartu_electric_missing_draw_warned = false;
    vendor_draw_hook = false;
    vendor_first_draw_logged = false;
    vendor_missing_draw_warned = false;
    nanotech_draw_hook = false;
    nanotech_first_draw_logged = false;
    nanotech_missing_draw_warned = false;
    nanotech_missing_clocks_warned = false;
    nanotech_first_particle_refresh_logged = false;
    paired_target_effect_hook = false;
    paired_target_first_effect_logged = false;
    paired_target_missing_clocks_warned = false;
    vendor_phases.reset();
    vendor_brightness.reset();
    flagged_mobys.clear();
    observed_level = -1;
    observed_classes.clear();
    exempted_classes.clear();
    freeze_animations = mod.setting<bool>("freeze_animations");
    keep_water = mod.setting<bool>("keep_water");
    hold_hoven_scroll = mod.setting<bool>("hold_hoven_scroll");
    diagnostic_log = mod.setting<bool>("diagnostic_log");
    soften_background = mod.setting<bool>("soften_background");
    background_softening_strength = mod.setting<int>("background_softening_strength");
    moby_array_global = mod.version() == "pal" ? kPalMobyArrayGlobal : kNtscMobyArrayGlobal;
    frame_time_global = mod.version() == "pal" ? kPalFrameTimeGlobal : kNtscFrameTimeGlobal;

    mod.hook<openrac::rac1::Hud_QuickSelect_Update>([](auto original, std::uint32_t selector) {
        wheel_active = true;
        wheel_visible_for_overlay.store(true, std::memory_order_relaxed);
        wheel_seen_this_tick = true;
        original(selector); // Keep the real weapon wheel responsive.
    });

    mod.hook<openrac::rac1::Hero_Update>([](auto original) {
        if (!wheel_active) {
            original();
        }
    });

    // Class 1901 draws Hoven's scrolling layered surface. Skipping its update
    // removes the surface, but running it normally advances the UV phases.
    // Its own update uses frame_delta_time for phase changes and then queues
    // its draw callback, so stop the clock only during this one function call.
    try {
        mod.hook<openrac::rac1::Moby_Class1901_Update>([](auto original, std::uint32_t moby) {
            if (!pause_policy::should_hold_hoven_scroll(
                    wheel_active, game_mod->loaded_level(), keep_water, hold_hoven_scroll)) {
                original(moby);
                return;
            }

            const auto address = game_mod->relocate(frame_time_global);
            if (address == 0) {
                original(moby); // Fall back to drawing normally if relocation failed.
                return;
            }
            auto memory = game_mod->memory();
            pause_policy::with_stopped_frame_time(
                [&] { return memory.read<float>(address); },
                [&](float value) { memory.write<float>(address, value); },
                [&] { original(moby); });
        });
        mod.info("Hoven scroll-hold hook registered (class 1901)");
    } catch (const openrac::Error&) {
        mod.warn("Hoven scroll-hold hook unavailable; water will still scroll");
    }

    // Quartu class 655 draws lightning, but its ordinary update also changes
    // shock timers, triggers audio, and damages Ratchet. Intercept the update
    // during Quick Select and only enqueue the game's already-initialized
    // bolt draw callback. Do not invoke original() while the world is paused.
    try {
        mod.hook<openrac::rac1::Moby_Class655_Update>(
            [](auto original, std::uint32_t moby) {
                const bool draw_only = pause_policy::should_route_quartu_electric_draw(
                    wheel_active, keep_water, world_gate,
                    quartu_electric_draw_hook, game_mod->loaded_level(), 655);
                bool drawable = false;
                if (draw_only && moby != 0) {
                    auto memory = game_mod->memory();
                    const auto state = memory.read<std::uint8_t>(moby + kMobyState);
                    const auto pvars = memory.read<std::uint32_t>(moby + kMobyPvars);
                    const auto light_slot = pvars == 0 ? -1
                        : memory.read<std::int32_t>(pvars + kQuartuElectricLightSlot);
                    drawable = pause_policy::can_draw_quartu_electric_arc(
                        state, pvars, light_slot);
                }

                pause_policy::dispatch_quartu_electric_update(
                    draw_only, drawable,
                    [&] { original(moby); },
                    [&] {
                        const bool pal = game_mod->version() == "pal";
                        const auto add = game_mod->function_at<GameAddDrawCallbackFn>(
                            pal ? kPalAddDrawCallback : kNtscAddDrawCallback);
                        const auto draw = game_mod->function_at<GameDrawEffectFn>(
                            pause_policy::quartu_electric_bolt_code_address(pal));
                        if (add == nullptr || draw == nullptr) {
                            if (!quartu_electric_missing_draw_warned) {
                                game_mod->warn("Quartu electric bolt draw function unavailable; shock update remains frozen");
                                quartu_electric_missing_draw_warned = true;
                            }
                            return;
                        }
                        // The game stores code addresses in its callback list.
                        // Passing `draw` (a native pointer) truncates it into
                        // an invalid guest target and causes a port call error.
                        pause_policy::enqueue_quartu_electric_bolt(pal, moby, add);
                        if (!quartu_electric_first_draw_logged) {
                            game_mod->info("Quartu electric bolt visual-only draw enqueued while gameplay is paused");
                            quartu_electric_first_draw_logged = true;
                        }
                    });
            });
        quartu_electric_draw_hook = true;
        mod.info("Quartu electric bolt draw-only hook registered (class 655)");
    } catch (const openrac::Error&) {
        mod.warn("Quartu class 655 hook unavailable; electric bolts remain paused");
    }

    // Class 11 is the Gadgetron vendor on every planet. Its normal update
    // draws the blue beam/letters but also pulses the glow, rotates the ring,
    // checks proximity and can open the vendor menu. Keep all of that logic
    // paused and only enqueue its already-initialized visual draw function.
    // The beam renderer also reads a proximity-controlled glow strength from
    // the vendor's pvars: hold it at full brightness and restore it on exit.
    // Unlike Hoven's water, zero frame time is insufficient: the renderer
    // increments its animation phases by fixed amounts.
    try {
        mod.hook<openrac::rac1::Moby_Vendor_Update>(
            [](auto original, std::uint32_t moby) {
                const bool draw_only = pause_policy::should_route_vendor_draw(
                    wheel_active, keep_water, world_gate, vendor_draw_hook, 11);
                bool drawable = false;
                std::uint32_t pvars = 0;
                if (draw_only && moby != 0) {
                    auto memory = game_mod->memory();
                    const auto state = memory.read<std::uint8_t>(moby + kMobyState);
                    pvars = memory.read<std::uint32_t>(moby + kMobyPvars);
                    const auto ring = pvars == 0 ? 0u
                        : memory.read<std::uint32_t>(pvars + kVendorRingPtr);
                    const auto array_base = current_moby_array();
                    drawable = pause_policy::can_draw_vendor_effects(state, pvars, ring)
                        && is_moby_in_active_array(moby, array_base)
                        && is_moby_in_active_array(ring, array_base)
                        && memory.read<std::int16_t>(ring + kMobyClass) == 1143;
                }

                pause_policy::dispatch_vendor_update(
                    draw_only, drawable,
                    [&] { original(moby); },
                    [&] {
                        const bool pal = game_mod->version() == "pal";
                        const auto add = game_mod->function_at<GameAddDrawCallbackFn>(
                            pal ? kPalAddDrawCallback : kNtscAddDrawCallback);
                        const auto draw = game_mod->function_at<GameDrawEffectFn>(
                            pause_policy::vendor_effect_code_address(pal));
                        const auto phase_addresses = pause_policy::vendor_phase_addresses(pal);
                        const auto sparkle_address = game_mod->relocate(phase_addresses.sparkle_uv);
                        const auto glow_address = game_mod->relocate(phase_addresses.glow);
                        if (add == nullptr || draw == nullptr
                            || sparkle_address == 0 || glow_address == 0) {
                            if (!vendor_missing_draw_warned) {
                                game_mod->warn("Gadgetron visual-only draw unavailable; vendor gameplay stays paused");
                                vendor_missing_draw_warned = true;
                            }
                            return;
                        }

                        auto memory = game_mod->memory();
                        vendor_phases.capture(
                            game_mod->loaded_level(),
                            memory.read<float>(sparkle_address),
                            memory.read<float>(glow_address));
                        vendor_brightness.capture(
                            game_mod->loaded_level(), current_moby_array(), moby,
                            memory.read<std::uint64_t>(moby + kMobySpawnFrame),
                            pvars, memory.read<float>(pvars + kVendorGlowStrength));
                        // The queued renderer reads these values later this
                        // frame, so they must stay in guest memory until draw.
                        memory.write<float>(pvars + kVendorGlowStrength, 1.0f);
                        memory.write<float>(glow_address,
                            pause_policy::vendor_full_glow_alpha_phase());
                        // The callback list stores the 32-bit guest address.
                        pause_policy::enqueue_vendor_effects(pal, moby, add);
                        ++vendor_visual_draw_count;
                        if (!vendor_first_draw_logged) {
                            game_mod->info("Gadgetron vendor bright stationary beam/letters draw enqueued");
                            vendor_first_draw_logged = true;
                        }
                    });
            });
        vendor_draw_hook = true;
        mod.info("Gadgetron vendor draw-only hook registered (class 11)");
    } catch (const openrac::Error&) {
        mod.warn("Gadgetron class 11 hook unavailable; vendor visuals remain paused");
    }

    // Nanotech boxes (501) create a class-806 orb. Draw-only rendering omits
    // the particles that form its bright purple halo. While still attached
    // to a live crate (state 1), its original update cannot collect health;
    // run that update with motion clocks and bobbing frozen to renew the
    // halo. Detached orbs (state 2) remain strictly draw-only, as their
    // original update can home in on Ratchet and heal him.
    try {
        mod.hook<openrac::rac1::Moby_NanotechBall_Update>(
            [](auto original, std::uint32_t moby) {
                const bool draw_only = pause_policy::should_route_nanotech_draw(
                    wheel_active, keep_water, world_gate, nanotech_draw_hook, 806);
                bool drawable = false;
                bool emit_attached = false;
                std::uint32_t pvars = 0;
                if (draw_only && moby != 0) {
                    auto memory = game_mod->memory();
                    const auto state = memory.read<std::uint8_t>(moby + kMobyState);
                    pvars = memory.read<std::uint32_t>(moby + kMobyPvars);
                    const auto owner = pvars == 0 ? 0u
                        : memory.read<std::uint32_t>(pvars + kNanotechOwnerPtr);
                    const auto array_base = current_moby_array();
                    if (is_moby_in_active_array(owner, array_base)) {
                        const auto owner_class = memory.read<std::int16_t>(owner + kMobyClass);
                        drawable = pause_policy::can_draw_nanotech_orb(
                            state, pvars, owner, owner_class);
                        emit_attached = pause_policy::can_emit_attached_nanotech_glow(
                            state, pvars, owner, owner_class,
                            memory.read<std::uint8_t>(owner + kMobyState));
                    }
                }

                const auto clocks = pause_policy::paired_swingshot_motion_clock_addresses(
                    game_mod->version() == "pal");
                const auto time_addr = emit_attached
                    ? game_mod->relocate(clocks.frame_time) : 0u;
                const auto sq_addr = emit_attached
                    ? game_mod->relocate(clocks.frame_time_squared) : 0u;
                if (emit_attached && (time_addr == 0 || sq_addr == 0)) {
                    emit_attached = false;
                    if (!nanotech_missing_clocks_warned) {
                        game_mod->warn("Nanotech particle clocks unavailable; drawing orb without refreshing its halo");
                        nanotech_missing_clocks_warned = true;
                    }
                }

                pause_policy::dispatch_nanotech_visual_update(
                    draw_only, drawable, emit_attached,
                    [&] { original(moby); },
                    [&] {
                        const bool pal = game_mod->version() == "pal";
                        const auto add = game_mod->function_at<GameAddDrawCallbackFn>(
                            pal ? kPalAddDrawCallback : kNtscAddDrawCallback);
                        const auto draw = game_mod->function_at<GameDrawEffectFn>(
                            pause_policy::nanotech_orb_code_address(pal));
                        if (add == nullptr || draw == nullptr) {
                            if (!nanotech_missing_draw_warned) {
                                game_mod->warn("Nanotech glow draw unavailable; health pickup remains paused");
                                nanotech_missing_draw_warned = true;
                            }
                            return;
                        }
                        pause_policy::enqueue_nanotech_orb(pal, moby, add);
                        ++nanotech_visual_draw_count;
                        if (!nanotech_first_draw_logged) {
                            game_mod->info("Nanotech orb visual-only draw enqueued during Quick Select");
                            nanotech_first_draw_logged = true;
                        }
                    },
                    [&] {
                        auto memory = game_mod->memory();
                        pause_policy::with_preserved_motion_pair(
                            [&] { return memory.read<float>(pvars + kNanotechBobOffset); },
                            [&](float value) {
                                memory.write<float>(pvars + kNanotechBobOffset, value);
                            },
                            [&] { return memory.read<float>(pvars + kNanotechBobPhase); },
                            [&](float value) {
                                memory.write<float>(pvars + kNanotechBobPhase, value);
                            },
                            [&] {
                                pause_policy::with_stopped_motion_clocks(
                                    [&] { return memory.read<float>(time_addr); },
                                    [&](float value) { memory.write<float>(time_addr, value); },
                                    [&] { return memory.read<float>(sq_addr); },
                                    [&](float value) { memory.write<float>(sq_addr, value); },
                                    [&] { original(moby); });
                            });
                        ++nanotech_attached_glow_count;
                        if (!nanotech_first_particle_refresh_logged) {
                            game_mod->info("Nanotech crate halo particles refreshed with orb motion held");
                            nanotech_first_particle_refresh_logged = true;
                        }
                    });
            });
        nanotech_draw_hook = true;
        mod.info("Nanotech attached-halo / detached draw-only hook registered (class 806)");
    } catch (const openrac::Error&) {
        mod.warn("Nanotech orb hook unavailable; health boxes stay paused without glow");
    }

    // A paired yellow Swingshot target is a class-803 moby whose state-1
    // update replenishes the glowing particle connection. It has no separate
    // draw callback. Allow only that update under zero movement clocks so
    // the target cannot steer toward Ratchet while the wheel is open.
    // Existing free particles may still move/fade; in-game testing is needed.
    try {
        mod.hook<openrac::rac1::Moby_SwingshotVersatarget_Update>(
            [](auto original, std::uint32_t moby) {
                if (moby == 0) {
                    original(moby);
                    return;
                }
                const auto memory = game_mod->memory();
                const auto oclass = memory.read<std::int16_t>(moby + kMobyClass);
                const auto state = memory.read<std::uint8_t>(moby + kMobyState);
                const bool preserve_glow = pause_policy::should_route_paired_swingshot_glow(
                    wheel_active, keep_water, world_gate, paired_target_effect_hook,
                    oclass, state);
                if (!preserve_glow) {
                    original(moby);
                    return;
                }

                const auto pvars = memory.read<std::uint32_t>(moby + kMobyPvars);
                if (pvars == 0) {
                    return; // A target in state 1 should always be initialized.
                }

                const auto clocks = pause_policy::paired_swingshot_motion_clock_addresses(
                    game_mod->version() == "pal");
                const auto time_addr = game_mod->relocate(clocks.frame_time);
                const auto sq_addr = game_mod->relocate(clocks.frame_time_squared);
                if (time_addr == 0 || sq_addr == 0) {
                    if (!paired_target_missing_clocks_warned) {
                        game_mod->warn("Paired Swingshot motion clocks unavailable; target stays frozen without particles");
                        paired_target_missing_clocks_warned = true;
                    }
                    return;
                }
                auto clock_memory = game_mod->memory();
                pause_policy::with_preserved_motion_pair(
                    [&] { return clock_memory.read<float>(moby + kMobyYaw); },
                    [&](float value) { clock_memory.write<float>(moby + kMobyYaw, value); },
                    [&] { return clock_memory.read<float>(pvars + kPairedTargetSteeringVelocity); },
                    [&](float value) {
                        clock_memory.write<float>(pvars + kPairedTargetSteeringVelocity, value);
                    },
                    [&] {
                        pause_policy::with_stopped_motion_clocks(
                            [&] { return clock_memory.read<float>(time_addr); },
                            [&](float value) { clock_memory.write<float>(time_addr, value); },
                            [&] { return clock_memory.read<float>(sq_addr); },
                            [&](float value) { clock_memory.write<float>(sq_addr, value); },
                            [&] { original(moby); });
                    });
                if (!paired_target_first_effect_logged) {
                    game_mod->info("Paired Swingshot glow particle update held at zero motion time");
                    paired_target_first_effect_logged = true;
                }
            });
        paired_target_effect_hook = true;
        mod.info("Paired Swingshot yellow-glow hook registered (class 803)");
    } catch (const openrac::Error&) {
        mod.warn("Paired Swingshot hook unavailable; yellow target particles remain paused");
    }

    try {
        mod.on_moby_update(gate_moby_update);
        world_gate = true;
    } catch (const openrac::Error&) {
        world_gate = false;
    }

    if (soften_background) {
        bool modern_overlay_registered = false;
#if OPENRAC_MOD_API_MINOR >= 14
        modern_overlay_registered = mod.on_overlay(draw_background_softening);
        if (modern_overlay_registered) {
            mod.info("background softening overlay registered (OpenRAC API 1.14)");
        } else {
            mod.info("OpenRAC overlay API unavailable; trying RAC1's own screen-rectangle renderer");
        }
#endif
        if (!modern_overlay_registered) {
            // API 1.12 has no port overlay, but RAC1 itself has a coloured
            // screen-rectangle draw routine. Insert its packet just before
            // the quick-select HUD draws, so wheel icons remain on top.
            // Optional and isolated: if the hook is taken, pause still works.
            try {
                mod.hook<openrac::rac1::Hud_QuickSelect_Draw>(
                    [](auto original, std::uint32_t panel) -> std::int32_t {
                        if (wheel_active && soften_background) {
                            if (legacy_draw_rect == nullptr) {
                                legacy_draw_rect = game_mod->function_at<GameDrawRectFn>(
                                    pause_legacy_shade::game_rect_address(
                                        game_mod->version() == "pal"));
                                if (legacy_draw_rect == nullptr
                                    && !legacy_draw_rect_warning_logged) {
                                    game_mod->warn("RAC1 screen-rectangle function unavailable; background softening skipped");
                                    legacy_draw_rect_warning_logged = true;
                                }
                            }
                            if (pause_legacy_shade::should_draw(
                                    wheel_active, soften_background,
                                    legacy_draw_rect != nullptr)) {
                                legacy_draw_rect(
                                    0, pause_legacy_shade::kPictureHeight,
                                    0, pause_legacy_shade::kPictureWidth,
                                    pause_legacy_shade::colour_for_strength(
                                        background_softening_strength));
                                if (!legacy_first_draw_logged) {
                                    game_mod->info("RAC1 background tint: first draw call made");
                                    legacy_first_draw_logged = true;
                                }
                            }
                        }
                        return original(panel);
                    });
                mod.info("RAC1 HUD-background tint hook registered (API 1.12 compatible)");
            } catch (const openrac::Error&) {
                mod.warn("RAC1 HUD tint hook unavailable; world pause remains active");
            }
        }
    }

    mod.on_frame([] {
        if (!wheel_seen_this_tick) {
            wheel_active = false;
            wheel_visible_for_overlay.store(false, std::memory_order_relaxed);
            restore_animations();
            report_pause_diagnostics();
        }

        if (vendor_brightness.active) {
            const int level = game_mod->loaded_level();
            const auto array_base = current_moby_array();
            if (!wheel_active || !keep_water || !world_gate || !vendor_draw_hook
                || vendor_brightness.level != level
                || vendor_brightness.array_base != array_base) {
                // Restoring the glow on a reused slot would corrupt an
                // unrelated moby, so compare all original identifiers.
                if (vendor_brightness.level == level
                    && vendor_brightness.array_base == array_base
                    && is_moby_in_active_array(vendor_brightness.moby, array_base)) {
                    auto memory = game_mod->memory();
                    const auto old_moby = vendor_brightness.moby;
                    if (memory.read<std::int16_t>(old_moby + kMobyClass) == 11) {
                        vendor_brightness.restore(
                            level, array_base, old_moby,
                            memory.read<std::uint64_t>(old_moby + kMobySpawnFrame),
                            memory.read<std::uint32_t>(old_moby + kMobyPvars),
                            [&](float glow) {
                                memory.write<float>(vendor_brightness.pvars
                                                    + kVendorGlowStrength, glow);
                            });
                    }
                }
                vendor_brightness.reset();
            }
        }

        if (vendor_phases.active) {
            const int level = game_mod->loaded_level();
            if (vendor_phases.level != level) {
                vendor_phases.reset();
            } else {
                const auto addresses = pause_policy::vendor_phase_addresses(
                    game_mod->version() == "pal");
                const auto sparkle = game_mod->relocate(addresses.sparkle_uv);
                const auto glow = game_mod->relocate(addresses.glow);
                if (sparkle == 0 || glow == 0) {
                    vendor_phases.reset();
                } else {
                    auto memory = game_mod->memory();
                    vendor_phases.restore(level, [&](float uv, float phase) {
                        memory.write<float>(sparkle, uv);
                        memory.write<float>(glow,
                            wheel_active && keep_water && world_gate && vendor_draw_hook
                                ? pause_policy::vendor_full_glow_alpha_phase()
                                : phase);
                    });
                    if (!wheel_active || !keep_water || !world_gate || !vendor_draw_hook) {
                        vendor_phases.reset();
                    }
                }
            }
        }
        wheel_seen_this_tick = false;
    });

    mod.info(world_gate
                 ? "weapon wheel pause: world updates gated; animation/scenery options active"
                 : "weapon wheel pause: Ratchet only; another mod owns the world gate");
}

} // namespace

OPENRAC_MOD_CPP("pause-weapon-wheel", "1.0.1", load)
