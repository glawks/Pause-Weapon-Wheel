# Weapon Wheel Pause for OpenRAC (Ratchet & Clank 1)

**Version 1.0.0 — working release**

A configurable weapon-wheel pause mod for **OpenRAC's Ratchet & Clank 1 (RAC1)**. Holding Quick Select freezes Ratchet and ordinary world-object updates while keeping the weapon wheel responsive. The mod also preserves water, glass, lava, atmosphere and other scenery that would otherwise disappear when those updates are paused.

The visual fixes have been tested in-game on **NTSC-U** across the affected planets, including Quartu's electrified water. The manifest also supports PAL, but PAL and non-Windows gameplay have not been independently verified.

## Features

- **Pause gameplay during Quick Select.** Ratchet and normal moby (world-object) updates stop until the wheel closes.
- **Freeze enemy animations.** Optionally set the game's animation-skip flag on paused objects, then restore only flags this mod changed.
- **Keep scenery visible.** Allow the specific controllers needed to register water, mud, lava, glass and environmental draw callbacks each frame.
- **Keep Quartu's electrical arcs visible.** A draw-only hook renders previously initialized electric-bolt geometry without running the controller's shock-damage and timer update.
- **Keep Hoven water still.** Stop its scrolling texture's frame-time advancement while retaining the draw callback.
- **Shade the background behind the wheel.** Adjustable 10–60% tint; this is a visual shade, **not** Gaussian blur.
- **Optional diagnostics.** Log observed moby classes and scenery exemptions when Quick Select closes.

### Verified scenery fixes

| Planet / area | Visual effects preserved during Quick Select |
| --- | --- |
| Novalis | Water and static water strips |
| Aridia | Mud/ocean surface |
| Nebula G34 | Transparent glass and textured quads |
| Blackwater City (Rilgar) | Rising water and ocean surface |
| Umbris | Water grids and ocean surface |
| Batalia | Main water and flowing textured water |
| Gaspar | Lava |
| Pokitaru | Ocean, water tiles and flowing surfaces |
| Hoven | Water and scrolling surfaces |
| Gemlik Base | Level-specific environmental effects |
| Oltanis | Scenery, fire/smoke, rain and lower liquid-like surface |
| Quartu | Regular water, electrified water and electrical arcs |
| Kalebo III | Glass, distant atmosphere and luminous conveyors |
| Multiple planets | Ship canopy/visor transparency |

## Settings

| Setting | Default | Purpose |
| --- | --- | --- |
| Freeze enemy animations | On | Hold paused moby animations in place |
| Keep water and scenery drawing | On | Preserve scenery and selected visual-effect callbacks |
| Freeze Hoven water scrolling | On | Keep Hoven's visible water texture from scrolling |
| Log pause diagnostics | On | Record moby class IDs and exempted classes when the wheel closes |
| Soften background during weapon selection | On | Shade the world behind Quick Select |
| Background softening strength | 30% | Adjust the shade from 10% to 60% |

Restart the game after changing settings.

## Install and build

This repository contains **source code**, not a precompiled mod.

To build the Windows x64 package:

1. Push the project files to a GitHub repository.
2. Open **Actions → windows-x64 → Run workflow**, or push to `main`/`master`.
3. Download the **pause-weapon-wheel-windows-x86_64** artifact after the workflow succeeds.
4. Extract the Actions artifact ZIP, then install the **mod ZIP inside it** through OpenRAC's **Mods** tab. Do not install the outer Actions artifact ZIP.

A newly published GitHub Release will contain the installable mod ZIP directly, named `pause-weapon-wheel-1.0.0-windows-x86_64.zip` for version 1.0.0. Existing release assets using the previous ID are not renamed automatically.

For a local Windows build with MSVC, CMake, Ninja and the [OpenRAC mod SDK 1.14.2](https://github.com/OpenRAC/OpenRAC/tree/sdk-v1.14.2/sdk) (or a newer SDK providing API 1.14+):

```powershell
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DOpenRACMod_DIR="C:/path/to/OpenRAC/sdk/cmake"
cmake --build build-win --parallel
```

The Windows workflow builds against the official **OpenRAC mod SDK 1.14.2** tag (SDK API **1.14**). The manifest still targets minimum runtime API **1.12**, so the mod remains loadable on older compatible OpenRAC versions. At runtime, API 1.14+ uses the optional feathered overlay for background shading; API 1.12–1.13 falls back to RAC1's existing rectangle renderer. Both are tints rather than sampled blurs.

**Mod ID change:** The internal mod ID is now `pause-weapon-wheel`. Earlier releases used `weapon-wheel-pause`, and test builds used `weapon-wheel-pause-test`. OpenRAC treats these as separate mods, so uninstall older builds after installing a release with the new ID, and reapply any settings linked to the old ID.

## Known limitations

- Some exempted scenery controllers also advance visual motion. For example, Kalebo III's conveyor texture keeps moving while Ratchet is frozen; animated rain, fire and flowing surfaces can continue animating.
- The first animation tick can advance once because the moby-update gate runs after the game's animation step.
- The mod needs OpenRAC's exclusive moby-update gate. If another mod owns that gate, this mod can freeze Ratchet but cannot fully pause the world.
- Additional game builds, PAL behaviour and macOS/Linux gameplay have not been verified. Scenery callbacks that were not encountered during testing may need additional exemptions.
- The background shading is not a true Gaussian blur. Disable it if it causes display issues.

## Technical notes

`src/mod.cpp` contains the OpenRAC hooks and main pause loop. `src/pause_policy.hpp` holds the level-specific scenery rules; `src/pause_legacy_shade.hpp` and `src/pause_veil.hpp` contain the two background-shading paths.

Most water/scenery surfaces register a **draw callback each frame** from a moby update. Skipping every moby update freezes gameplay but also removes those surfaces. This mod keeps the required rendering controllers active, with a separate draw-only exception for Quartu's damaging electrical-water controller (class `655`).

Quartu's electric-bolt callback uses the game's **32-bit code address**, rather than a native function pointer: `0x002D8710` on NTSC-U and `0x002D9B00` on PAL. The normal electric-water update—which can damage Ratchet—remains blocked during Quick Select.

Source references: [OpenRAC](https://github.com/OpenRAC/OpenRAC), [RAC1 electric-water decompilation](https://github.com/OpenRAC/rac1-decomp/blob/main/src/overlays/shared/electric_water.c) and [ripple-strip renderer](https://github.com/lombyte-project/Lombyte/blob/main/src/overlays/shared/props/ripple_strips.c).

Tests: `tests/policy_test.cpp`, `tests/veil_test.cpp`, and `tests/legacy_shade_test.cpp`.

## License

MIT — see [LICENSE](LICENSE). Authored by **glawks**.
