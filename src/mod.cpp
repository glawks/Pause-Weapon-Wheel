#include "openrac/mod.hpp"
#include "openrac/games/rac1.hpp"
#include "pause_policy.hpp"

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

struct FrozenAnimation {
    int level;
    openrac::gaddr array_base;
    openrac::gaddr moby;
    std::uint64_t spawn_frame;
    std::int16_t oclass;
};

openrac::Mod* game_mod = nullptr;
bool wheel_active = false;
bool wheel_seen_this_tick = false;
bool world_gate = false;
bool freeze_animations = true;
bool keep_water = true;
bool hold_hoven_scroll = true;
bool diagnostic_log = true;
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
    if (keep_water && pause_policy::needs_update_for_water(level, oclass)) {
        if (diagnostic_log) {
            exempted_classes.insert(oclass);
        }
        return 0; // Let the water moby register the draw callback this frame.
    }

    if (freeze_animations) {
        stop_animation(moby, array_base, level, oclass);
    }
    return 1; // Skip its gameplay update, as in the original mod.
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1") {
        throw openrac::Error("Weapon Wheel Pause Test is for Ratchet & Clank 1");
    }
    if (mod.version() != "pal" && mod.version() != "ntsc") {
        throw openrac::Error("Only RAC1 PAL and NTSC-U are supported");
    }

    game_mod = &mod;
    wheel_active = false;
    wheel_seen_this_tick = false;
    world_gate = false;
    flagged_mobys.clear();
    observed_level = -1;
    observed_classes.clear();
    exempted_classes.clear();
    freeze_animations = mod.setting<bool>("freeze_animations");
    keep_water = mod.setting<bool>("keep_water");
    hold_hoven_scroll = mod.setting<bool>("hold_hoven_scroll");
    diagnostic_log = mod.setting<bool>("diagnostic_log");
    moby_array_global = mod.version() == "pal" ? kPalMobyArrayGlobal : kNtscMobyArrayGlobal;
    frame_time_global = mod.version() == "pal" ? kPalFrameTimeGlobal : kNtscFrameTimeGlobal;

    mod.hook<openrac::rac1::Hud_QuickSelect_Update>([](auto original, std::uint32_t selector) {
        wheel_active = true;
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

    try {
        mod.on_moby_update(gate_moby_update);
        world_gate = true;
    } catch (const openrac::Error&) {
        world_gate = false;
    }

    mod.on_frame([] {
        if (!wheel_seen_this_tick) {
            wheel_active = false;
            restore_animations();
            report_pause_diagnostics();
        }
        wheel_seen_this_tick = false;
    });

    mod.info(world_gate
                 ? "experimental pause: world updates gated; animation/scenery options active"
                 : "experimental pause: Ratchet only; another mod owns the world gate");
}

} // namespace

OPENRAC_MOD_CPP("weapon-wheel-pause-test", "0.2.0-test.4", load)
