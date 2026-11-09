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
    }
    observed_level = -1;
    observed_classes.clear();
    exempted_classes.clear();
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
    if (keep_water && (pause_policy::needs_update_for_water(level, oclass)
                       || pause_policy::should_route_quartu_electric_draw(
                           wheel_active, keep_water, world_gate,
                           quartu_electric_draw_hook, level, oclass))) {
        if (diagnostic_log) {
            exempted_classes.insert(oclass);
        }
        // Class 655 is intercepted by the draw-only hook below; it never
        // reaches its original damaging update during Quick Select.
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
        wheel_seen_this_tick = false;
    });

    mod.info(world_gate
                 ? "weapon wheel pause: world updates gated; animation/scenery options active"
                 : "weapon wheel pause: Ratchet only; another mod owns the world gate");
}

} // namespace

OPENRAC_MOD_CPP("pause-weapon-wheel", "1.0.1", load)
