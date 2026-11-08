# Experimental Weapon Wheel Pause for OpenRAC RAC1

An **experimental weapon-wheel pause mod** for OpenRAC's Ratchet & Clank 1 (RAC1). It is still undergoing in-game testing.

The mod stops Ratchet and skips world moby updates while Quick Select is open. It also provides configurable options:

1. **Freeze enemy animations (default ON):** Set the game's `0x40` animation-skip flag on mobys whose updates are blocked. Clear only the bit this mod set when the wheel closes, and only if the same object is still in the same level/slot.
2. **Keep water and scenery drawing (default ON):** Let known visual-effect controllers execute the updates that register their render callbacks every frame. Version `0.2.0-test.3` added **Hoven class 1901** (scrolling texture layers) to classes 19, 293 and 384, and retains Oltanis class 1418. The large Hoven surface was confirmed visible again with that exemption.
3. **Freeze Hoven water scrolling (default ON, NEW in `0.2.0-test.4`):** Temporarily set `frame_delta_time` to zero *only while class 1901's update is executing*, then restore its exact previous value. That update advances the UV phase using frame time and then queues its draw callback; this test aims to keep the surface visible and stop its scrolling without freezing the rest of the game clock. Source: [OpenRAC Hoven scrolling layers](https://github.com/OpenRAC/OpenRAC/blob/develop/games/rac1/ntsc/src/overlays/l12/mobys/scrolling_texture_layers.c).
4. **Log pause diagnostics (default ON):** Record the game's reported level, all moby class IDs observed during the pause and which were allowed to update. The log is written when the wheel closes to help diagnose scenery that still disappears.

The Hoven class 1901 visibility fix was **confirmed in-game**. The `test.4` clock override is **not yet verified in-game**. It addresses only the scrolling layers, not other animated water ripples, snowfall or scenery on other planets.

## Testing

Use a backed-up save and test somewhere safe, away from a mission or timed hazard.

Install the built Windows x64 package through OpenRAC's Mods tab. In the mod's settings, try:

| Test | Freeze enemy animations | Keep water and scenery drawing | What it isolates |
|---|---|---|---|
| A | Off | Off | Basic pause behaviour (baseline) |
| B | On | Off | Animation change only |
| C | Off | On | Water-rendering change only |
| D | On | On | Both changes |

Restart the game when changing mod settings. On each test, open the weapon wheel while an enemy is mid-animation and near water. Check whether the animation stops on the current frame, water remains visible, enemies still cannot move or harm Ratchet, and both animation and AI resume after closing the wheel.

Please report the planet, disc region, OpenRAC version, and any missing objects, moving platforms, projectiles or crashes. The water allowlist is **not comprehensive for every rendering dependency**; allowing those updates can also allow their associated water/platform motion to progress during the pause.

For the Hoven-specific test, leave enemy animation freeze, scenery drawing and Hoven water scroll freeze **ON**, then open the wheel near the turquoise surface. Compare `Freeze Hoven water scrolling` ON vs OFF (restart between setting changes). Both should keep the water visible; only ON should hold the scrolling texture still. Also verify enemies remain frozen and the water resumes moving normally after closing the wheel. The game's **Logs** (launcher, under that game's saves) should contain `Hoven scroll-hold hook registered (class 1901)`; if you instead see `hook unavailable`, please share that line.

## Windows x64 build

This repository includes `.github/workflows/windows.yml`. Push the folder's contents to a GitHub repository and select **Actions → windows-x64-test → Run workflow** (or push to `main`/`master`). Once it passes, download the `weapon-wheel-pause-test-windows-x86_64` artifact. Inside it is the OpenRAC-compatible mod ZIP.

The workflow uses a Windows-capable OpenRAC branch. It may need updating if that branch is removed. Alternatively, with CMake, Ninja, MSVC and an OpenRAC SDK >= 1.12 already installed:

```powershell
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DOpenRACMod_DIR="C:/path/to/OpenRAC/sdk/cmake"
cmake --build build-win --parallel
```

This source archive is **not a precompiled Windows mod**. The binary still needs to be built before OpenRAC can load it.

## Developer notes

- Source: `src/mod.cpp`; isolated allowlist/flag rules: `src/pause_policy.hpp`.
- Quick policy checks: `g++ -std=c++17 -Isrc tests/policy_test.cpp -o policy_test && ./policy_test`.
- The first animation tick may advance once because the OpenRAC moby gate fires *after* the game's animation advancement.
- The mod accesses game memory using documented addresses from OpenRAC's RAC1 PAL and NTSC world hosts. Additional regions or future layout changes need checking.
- OpenRAC's moby-update gate is exclusive. If the gate is unavailable, only Ratchet freezes.

## Attribution

Game decompilation observations and source references: [OpenRAC](https://github.com/OpenRAC/OpenRAC).
