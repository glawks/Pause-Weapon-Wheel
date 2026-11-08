# Experimental Weapon Wheel Pause for OpenRAC RAC1

An **experimental weapon-wheel pause mod** for OpenRAC's Ratchet & Clank 1 (RAC1). It is still undergoing in-game testing.

The mod stops Ratchet and skips world moby updates while Quick Select is open. It also provides configurable options:

1. **Freeze enemy animations (default ON):** Set the game's `0x40` animation-skip flag on mobys whose updates are blocked. Clear only the bit this mod set when the wheel closes, and only if the same object is still in the same level/slot.
2. **Keep water and scenery drawing (default ON):** Let known visual-effect controllers execute the updates that register their render callbacks every frame. Novalis includes ripple-water class 751 and static-strip-water classes 676, 678, 761, and 1225 (new in `test.10`). Hoven includes scrolling-texture class 1901; Oltanis includes scenery class 1418, smoke/fire class 760, rain emitter class 1400, and lower liquid-grid class 1260. The water on Hoven and the low-lying surface on Oltanis were confirmed visible again; Oltanis rain and fire remain **animated**. Novalis's additional classes have not yet been verified in-game.
3. **Freeze Hoven water scrolling (default ON):** Temporarily set `frame_delta_time` to zero *only while class 1901's update is executing*, then restore its exact previous value. That update advances the UV phase using frame time and then queues its draw callback; testing confirmed Hoven's water remains visible and stops scrolling. Source: [OpenRAC Hoven scrolling layers](https://github.com/OpenRAC/OpenRAC/blob/develop/games/rac1/ntsc/src/overlays/l12/mobys/scrolling_texture_layers.c).
4. **Log pause diagnostics (default ON):** Record the game's reported level, all moby class IDs observed during the pause and which were allowed to update. The log is written when the wheel closes to help diagnose scenery that still disappears.
5. **Soften background during weapon selection (default ON, experimental):** Reduce the background's prominence while leaving the weapon-wheel icons readable. On **API 1.14+** (when compiled with SDK 1.14+), try a translucent blue-black overlay with a feathered opening around the wheel. On **API 1.12**, `test.9` and later use RAC1's own screen-rectangle renderer immediately *before* drawing Quick Select, tinting the world underneath it. The API 1.12 fallback was confirmed working in-game. **Neither method is a true Gaussian blur.** The 10–60% Background softening strength setting (default 30%) controls the tint. Disable this setting if the experiment produces rendering problems.

**Compatibility:** The manifest requests mod interface **1.12**, so both builds below target OpenRAC 1.12 and newer. The SDK version used *to compile* the mod determines which drawing methods it can use:

| Compile with | Run on OpenRAC API 1.12 | Run on OpenRAC API 1.14+ |
|---|---|---|
| SDK 1.12 | Pause works; experimental game-native tint | Pause works; experimental game-native tint |
| SDK 1.14+ | Pause works; experimental game-native tint | Pause works; 1.14 overlay tint |

Installing a new OpenRAC version does not add the 1.14 overlay to a DLL already compiled with SDK 1.12; rebuild the mod with SDK 1.14+ to include that path. The game-native fallback is available to both builds.

The Hoven class 1901 visibility and scroll freeze were **confirmed in-game**. On Oltanis, `test.5` restored fire, `test.6` restored rain, and `test.7` restored the stationary fog-like lower surface. The `test.9` game-native background tint was also **confirmed in-game**. Novalis's additional static-strip water exemptions in `test.10` still need in-game verification.

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

For **test.10**, open Quick Select on **Novalis** above the river near the bridge. Check whether both the near river and the distant water remain visible, and whether enemy AI remains frozen. With `Keep water and scenery drawing` enabled, the diagnostics should list `676`, `678`, `751`, `761`, and `1225` as exempted when their mobys run. The other visual test is to verify the background darkens while the wheel icons remain readable and returns to normal after closing. The native fallback logs `experimental RAC1 HUD-background tint hook registered` on load and `experimental RAC1 background tint: first draw call made` once its rectangle routine is invoked. These logs show invocation, not proof of a correct display. With SDK/runtime 1.14+, the alternate path logs `background softening overlay registered`. Disable the option if visual or gameplay issues occur.

## Windows x64 build

This repository includes `.github/workflows/windows.yml`. Push the folder's contents to a GitHub repository and select **Actions → windows-x64-test → Run workflow** (or push to `main`/`master`). Once it passes, download the `weapon-wheel-pause-test-windows-x86_64` artifact. Inside it is the OpenRAC-compatible mod ZIP.

The workflow currently uses a Windows-capable OpenRAC branch with **SDK 1.12**, producing the experimental game-native tint fallback. Once a Windows-compatible SDK 1.14+ is available, building with it will also include the optional port overlay, while retaining runtime compatibility with API 1.12. Alternatively, with CMake, Ninja, MSVC and an OpenRAC SDK 1.12 or newer installed:

```powershell
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DOpenRACMod_DIR="C:/path/to/OpenRAC/sdk/cmake"
cmake --build build-win --parallel
```

This source archive is **not a precompiled Windows mod**. The binary still needs to be built before OpenRAC can load it.

## Developer notes

- Source: `src/mod.cpp`; isolated allowlist/flag rules: `src/pause_policy.hpp`; overlay geometry: `src/pause_veil.hpp`; game-native tint constants: `src/pause_legacy_shade.hpp`.
- Checks: `tests/policy_test.cpp`, `tests/veil_test.cpp`, and `tests/legacy_shade_test.cpp`.
- The first animation tick may advance once because the OpenRAC moby gate fires *after* the game's animation advancement.
- The mod accesses game memory using documented addresses from OpenRAC's RAC1 PAL and NTSC world hosts. Additional regions or future layout changes need checking.
- OpenRAC's moby-update gate is exclusive. If the gate is unavailable, only Ratchet freezes.
- The API 1.12 fallback calls RAC1's existing `draw_rect_overlay` game function (NTSC) or `DrawRectOverlay_FiiiiUl` (PAL) via the mod function lookup, using documented game code addresses. Source: [NTSC screen rectangle](https://github.com/OpenRAC/OpenRAC/blob/master/games/rac1/ntsc/src/rendering/effects/draw_rect_overlay.c) and [PAL renderer](https://github.com/OpenRAC/OpenRAC/blob/master/games/rac1/src/game/render/draw.c). Drawing into the live HUD render pass remains an in-game experiment.

## Attribution

Game decompilation observations and source references: [OpenRAC](https://github.com/OpenRAC/OpenRAC).
