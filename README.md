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

The mod supports **Ratchet & Clank 1 (PAL and NTSC-U)** on **Windows x86_64, Linux x86_64/aarch64, macOS arm64/x86_64, and Android ARM64**. It requires OpenRAC mod API **1.12 or newer**.

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
- Most level-specific scenery fixes were tested on NTSC-U for Windows; PAL and macOS gameplay have not been tested to the same extent.
- The Linux build has been tested in-game on ChimeraOS by community contributor [Promises](https://github.com/Promises), including all 16 hooks. Android ARM64 has also been tested successfully in-game.
- The launcher needs the archive matching your platform, not an archive for a different operating system.

## Building from source

[GitHub Actions](https://github.com/glawks/Pause-Weapon-Wheel/actions) builds and packages all six supported platforms using the checksum-pinned [OpenRAC mod SDK v1.14.6](https://github.com/OpenRAC/OpenRAC/releases/tag/sdk-v1.14.6). Each versioned GitHub Release includes individual platform ZIPs, a combined ZIP, and `SHA256SUMS`.

To build locally, install Python 3, CMake, Ninja, and a compatible C++ compiler, then run:

```sh
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The build uses the OpenRAC SDK from `sdk.lock`. Cross-compiling for Android requires the Android NDK with `ANDROID_ABI=arm64-v8a` and `ANDROID_PLATFORM=android-28`; CI performs that build automatically.

For a workflow artifact, extract the outer GitHub Actions artifact first, then install the **platform-specific mod ZIP inside it** in OpenRAC's Mods tab.

Thanks to [Promises](https://github.com/Promises) for testing the Linux build in ChimeraOS and contributing Linux packaging feedback via [PR #1](https://github.com/glawks/Pause-Weapon-Wheel/pull/1).

## License

MIT — see [LICENSE](LICENSE). Created by **glawks**.
