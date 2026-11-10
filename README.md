# Weapon Wheel Pause

Pause the action in **Ratchet & Clank (2002)** while choosing a weapon in **OpenRAC**.

Holding **Quick Select** freezes Ratchet, enemies and most world interactions while keeping the weapon wheel responsive. Water, glass and other scenery remain visible instead of disappearing when the game is paused.

## Features

- **Pause during weapon selection.** Take your time picking a weapon without enemies continuing to attack.
- **Freeze animations.** Keep enemies and other paused objects from animating while the wheel is open.
- **Preserve water and scenery.** Keep water, mud, lava, ship glass and environmental effects visible across the game.
- **Pause rising water.** Timed water hazards, including Blackwater City's flooding room, stop progressing while you select a weapon, without making the water invisible.
- **Preserve special effects.** Keep effects such as Gadgetron vendor beams, Nanotech glows, Swingshot target connections, electric arcs, laser barriers and other level-specific visuals on screen.
- **Optional background shading.** Dim the world behind Quick Select to make the weapon wheel easier to see.

The scenery fixes have been tested primarily in the **NTSC-U** version. PAL is supported, but has not been independently tested to the same extent.

## Installation

Install **Weapon Wheel Pause** using OpenRAC's **Mods** tab.

Alternatively, download the appropriate mod ZIP from the [GitHub Releases page](https://github.com/glawks/Pause-Weapon-Wheel/releases) and install it through OpenRAC's Mods tab.

The mod supports **Ratchet & Clank 1 (PAL and NTSC-U)** and requires OpenRAC mod API **1.12 or newer**.

## Settings

| Setting | Default | Description |
| --- | --- | --- |
| Freeze enemy animations | On | Freeze the animations of paused world objects. |
| Keep water and scenery drawing | On | Keep scenery and supported visual effects visible during Quick Select. |
| Freeze Hoven water scrolling | On | Keep Hoven's scrolling water texture still while paused. |
| Log pause diagnostics | On | Write extra information to the log for troubleshooting. |
| Soften background during weapon selection | On | Dim the scene behind the weapon wheel. |
| Background softening strength | 30% | Set the background shade strength (10–60%). |

Restart the game after changing settings.

## Things to know

- The mod pauses gameplay **while Quick Select is open**; it is not a general pause-menu replacement.
- Some decorative particles, rain, flowing textures and other ambient effects may continue animating even while gameplay is frozen.
- Other mods that control OpenRAC's world-object update gate may conflict. If another mod owns that gate, Ratchet may freeze without the rest of the world freezing.
- The optional background effect is a **tint**, not a blur.
- PAL and macOS/Linux gameplay have not yet been verified as extensively as NTSC-U on Windows.
- The [`testing` branch](https://github.com/glawks/Pause-Weapon-Wheel/tree/testing) may contain fixes that have not yet been included in the latest public release.

## Building from source

This repository contains the mod source code. Windows x64 builds are produced by [GitHub Actions](https://github.com/glawks/Pause-Weapon-Wheel/actions), or you can build locally using CMake, Ninja and the [OpenRAC mod SDK](https://github.com/OpenRAC/OpenRAC/tree/sdk-v1.14.2/sdk):

```powershell
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DOpenRACMod_DIR="C:/path/to/OpenRAC/sdk/cmake"
cmake --build build-win --parallel
```

If you use a GitHub Actions build, extract the workflow artifact first and install the **mod ZIP inside it**, not the outer artifact ZIP.

## License

MIT — see [LICENSE](LICENSE). Created by **glawks**.
