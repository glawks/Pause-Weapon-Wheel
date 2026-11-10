# Weapon Wheel Pause for OpenRAC (Ratchet & Clank 1)

**Version 1.0.1 — working release**

> The `testing` branch contains unreleased experiments. The paired Swingshot
> connection, intact-crate Nanotech halo, Gadgetron vendor beam, vendor
> sphere-size hold and Kalebo III doorway wall are confirmed in NTSC-U testing.
> The detached Nanotech glow is still awaiting confirmation after a new
> NTSC-U primary-pointer and per-level draw-address correction. The branch
> logs extra read-only diagnostics for the detached safety gate.
> Veldin Orbit's lasers, water, reflective glass and distant planet beam,
> final Veldin's water-like shaft mesh and green security barriers, and
> detached Nanotech halo are confirmed in NTSC-U testing. The Veldin Orbit
> yellow spark-field barrier and exterior air-flow veil now have targeted
> draw-only test fixes awaiting an in-game check.

A configurable weapon-wheel pause mod for **OpenRAC's Ratchet & Clank 1 (RAC1)**. Holding Quick Select freezes Ratchet and ordinary world-object updates while keeping the weapon wheel responsive. The mod also preserves water, glass, lava, atmosphere and other scenery that would otherwise disappear when those updates are paused.

The visual fixes have been tested in-game on **NTSC-U** across the affected planets, including Quartu's electrified water. The manifest also supports PAL, but PAL and non-Windows gameplay have not been independently verified.

## Features

- **Pause gameplay during Quick Select.** Ratchet and normal moby (world-object) updates stop until the wheel closes.
- **Freeze enemy animations.** Optionally set the game's animation-skip flag on paused objects, then restore only flags this mod changed.
- **Keep scenery visible.** Allow the specific controllers needed to register water, mud, lava, glass and environmental draw callbacks each frame.
- **Keep Quartu's electrical arcs visible.** A draw-only hook renders previously initialized electric-bolt geometry without running the controller's shock-damage and timer update.
- **Keep Hoven water still.** Stop its scrolling texture's frame-time advancement while retaining the draw callback.
- **Keep Gadgetron vendor effects visible (testing).** Refresh the beam through the vendor's original non-interactive update, then restore its state, held ring rotation and original sphere scale. The glow is kept at full strength and its previous value restored after Quick Select closes.
- **Keep yellow paired Swingshot targets glowing (testing).** Replenish the particle stream connecting the two yellow spheres while temporarily stopping the target's motion clocks.
- **Keep Nanotech crate glows visible (testing).** Refresh the purple particle halo both inside intact crates and around detached orbs after a crate breaks. Both visual updates hold the orb's movement; detached orbs temporarily mask the pickup path so Ratchet cannot collect health during Quick Select.
- **Keep Kalebo III's purple doorway wall visible (testing).** Refresh the active linked-pair ribbon renderer while its switch and surrounding world-object updates remain paused. Confirmed in-game on NTSC-U.
- **Keep Veldin Orbit's red laser beams visible (testing).** Requeue the previously initialized beam quads without running the class-99 motion or hit-detection update. Confirmed in-game on NTSC-U.
- **Keep Veldin Orbit's distant fleet beam and planet impact visible (testing).** Requeue class 1772's original backdrop renderer, which draws the blue-white beam and purple impact glow, without running the scenery controller's update. The distant beam was confirmed in-game on NTSC-U; the glow has not been separately checked.
- **Keep Veldin Orbit's water and reflective glass visible (testing).** Allow the four visual-only water ripple-strip controllers and six reflective-mesh callbacks to run while the weapon wheel is open. Confirmed in-game on NTSC-U.
- **Keep Veldin Orbit's spark barrier and air-flow veil visible (testing).** Requeue the active class-78 spark-field and class-1378 updraft-vent translucent quads without re-running linked emitter, particle or hero-updraft gameplay logic. Awaiting in-game confirmation.
- **Keep the final Veldin's green security barriers visible (testing).** Requeue class 1392's active-state glow quads without running the linked Trespasser-pad, sound or collision update. Confirmed in-game on NTSC-U.
- **Keep the final Veldin's shaft meshes visible (testing).** Requeue one of class 1402's three textured lift/shaft renderers without advancing the lift or linked scenery. The missing water-like surface is confirmed fixed in-game on NTSC-U.
- **Shade the background behind the wheel.** Adjustable 10–60% tint; this is a visual shade, **not** Gaussian blur.
- **Optional diagnostics.** Log observed moby classes and scenery exemptions, plus counts of vendor, Nanotech, Kalebo wall-ribbon, Veldin Orbit laser, fleet-backdrop, spark-field barrier, air-flow veil, final Veldin barrier and shaft-mesh visual refreshes when Quick Select closes. The `testing` branch also separates attached/detached Nanotech fallback reasons and records the sampled primary pointer.

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
2. Open **Actions → windows-x64 → Run workflow**, or push to `main`/`master`. On the `testing` branch, every push automatically starts a Windows build and uploads an artifact without publishing a release.
3. Download the **pause-weapon-wheel-windows-x86_64** artifact after the workflow succeeds.
4. Extract the Actions artifact ZIP, then install the **mod ZIP inside it** through OpenRAC's **Mods** tab. Do not install the outer Actions artifact ZIP.

A newly published GitHub Release will contain the installable mod ZIP directly, named `pause-weapon-wheel-1.0.0-windows-x86_64.zip` for version 1.0.0. Existing release assets using the previous ID are not renamed automatically.

**Testing without a release:** open the latest successful `testing` Actions run, download the `pause-weapon-wheel-windows-x86_64` artifact, extract it, and install the mod ZIP inside using OpenRAC's Mods tab. The mod keeps ID `pause-weapon-wheel` and version `1.0.1` while testing, so it is not a separate side-by-side installation; retain the stable release ZIP if you want to switch back. Do not enter `publish_tag` for test builds.

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
- The `testing` branch's vendor beam and sphere-size correction are confirmed visible in NTSC-U, using the original state-1 visual update with proximity checks disabled and the pre-pause scale restored. The internal ring angle counters still advance, although the ring's visible quaternion/matrix is restored while paused; it may snap to the advanced angle after unpausing. If the vendor was off when paused, its beam may appear brighter than its ordinary distance-based fade.
- The yellow paired Swingshot target's connection was confirmed visible in the initial user test. Its short-lived particles may continue moving/fading as they are replenished; the target's own steering is held. This does **not** change Ratchet's grappling cable.
- The Nanotech crate's attached purple halo and detached-orb halo have both been confirmed working in NTSC-U testing. The diagnostic log isolated two address errors: an incorrect primary-orb pointer (`0x0015F61C` instead of the game's `0x0015F63C`) and a level-00 draw callback reused in other overlays. Both mappings are corrected on `testing`. The `nanotech probe` log still separates state-1 fallback from state-2 pointer failures without running extra gameplay updates. PAL behaviour remains unverified.
- Kalebo III's class-552 linked-pair ribbons were **confirmed visible in-game**. Their active-state refresh also maintains ambient wall audio, which may continue while the wheel is held; the wall switch and unrelated mobys are still paused.
- Veldin Orbit's class-99 laser beam draw-only path is **confirmed in-game on NTSC-U**. The renderer uses the most recently calculated beam endpoints and width, which remain stationary during Quick Select; normal movement, particle spraying and hit detection stay blocked. This may omit some moving sparks while preserving the main red beam quads.
- Veldin Orbit's class-1772 fleet backdrop draw-only path has its **distant beam confirmed in-game on NTSC-U**, but the impact glow has not been separately checked. It preserves the precomputed beam endpoint and glow parameters, avoiding the controller's gameplay-affecting updates. Its draw callback may still scroll beam-texture UVs slightly while paused.
- Veldin Orbit's water/glass allowance is **confirmed in-game on NTSC-U**. The observed class `1405`/`1426` ripple-strip mobys and class `1907`/`1908`/`1910`/`1911`/`1913`/`1918` reflective-mesh mobys were previously frozen. The update routines initialise or enqueue scenery callbacks only, so they now remain active while Quick Select is open. Class `655` (electric-water hazard), underwater doors and rings remain paused.
- Veldin Orbit's doorway spark field and exterior air-flow veil **await in-game confirmation**. The class-`78` linked spark-emitter update normally creates particles, checks a linked object and enqueues `draw_spark_emitter_effect_quads` in states `1`/`2`. Class `1378` (updraft vent) normally changes Ratchet's updraft flag as well as enqueuing `draw_moby_animated_quad_layers` in state `1`. The two testing hooks submit their original level-17 renderers only: `0x002A9AD0` / `0x002EB288` (PAL), `0x002A8808` / `0x002E9798` (NTSC-U). Their gameplay logic remains frozen; independently updated particles could still fade during Quick Select.
- Final Veldin's class-1392 green-barrier draw-only path was **confirmed in-game on NTSC-U**. It reuses initialized glow parameters without processing the linked puzzle or collision state. The glow's own scrolling UV phase can still advance a little while paused.
- Final Veldin's class-1402 lift/shaft draw-only path is **confirmed to restore the missing water-like surface in-game on NTSC-U**. The decompiled update submits a 25-, 38- or 22-quad renderer chosen by the lift's mesh selector, but also changes lift height, linked mobys and commands. The test branch redraws initialized meshes with those updates frozen. Mesh UV scrolling also stops while paused.

## Technical notes

`src/mod.cpp` contains the OpenRAC hooks and main pause loop. `src/pause_policy.hpp` holds the level-specific scenery rules and visual-only policies; `src/pause_legacy_shade.hpp` and `src/pause_veil.hpp` contain the two background-shading paths.

Most water/scenery surfaces register a **draw callback each frame** from a moby update. Skipping every moby update freezes gameplay but also removes those surfaces. This mod keeps the required rendering controllers active, with a separate draw-only exception for Quartu's damaging electrical-water controller (class `655`).

Quartu's electric-bolt callback uses the game's **32-bit code address**, rather than a native function pointer: `0x002D8710` on NTSC-U and `0x002D9B00` on PAL. The normal electric-water update—which can damage Ratchet—remains blocked during Quick Select.

Nanotech crates (class `501`) spawn a class `806` orb; the orb's original glow draw callback uses `0x002D9810` (NTSC-U) or `0x002DACC0` (PAL) **only in the level-00 overlay**. The testing branch selects the correct callback for each level (e.g. NTSC-U level `16` uses `0x002D83C0`). The attached orb's state-1 update produces its short-lived halo particles and cannot heal Ratchet while the crate is alive; we run that path with motion clocks and bobbing held. A detached orb (state `2`) emits the same halo, but its normal update also manages falling and health collection. The guarded testing path temporarily masks the level's primary Nanotech pointer (so the pickup condition is false), zeroes the motion clocks and falling velocity, calls the particle-producing update, then restores the primary pointer, position, bobbing, velocity and grounding state. This is isolated to state `2`; if the primary pointer is unavailable, it falls back to draw-only. Detached orbs continue emitting small short-lived particles while the wheel is held.

Yellow paired Swingshot targets are class `803`. Their state-1 update emits the connection's particles rather than registering a dedicated draw callback. The experimental hook runs that update with its two movement clocks temporarily zeroed; class `758` targets remain frozen as before.

Kalebo III's purple doorway wall is made from class `552` (`0x228`) linked-pair ribbons. Only the initialized group leader (state `1`, not flagged as a follower) is allowed to register its `draw_linked_pair_group_ribbons` callback during Quick Select; state-0 group initialization and class `546` wall-button updates are not unpaused. Its ribbon update also maintains an ambient sound handle.

Veldin Orbit (Drek's Fleet, level `17`) uses class `99` for red scanning laser beams. Its full update both positions the beams and calls collision-hit routines, so the testing branch submits `draw_fleet_beam_quad` directly using the game callback addresses `0x002AA880` (PAL) or `0x002A95B8` (NTSC-U). This draws only initialized active beams and leaves the normal laser update frozen while the weapon wheel is open.

The *distant* blue-white fleet beam and the pink-purple explosion against the planet are a separate class `1772` (`FleetReflectionUpdate`) effect. Its original draw callback, `draw_fleet_backdrop_quads`, emits two textured beam quads and five impact-glow quads. The testing branch enqueues this renderer using the guest address `0x002F4E30` (PAL) or `0x002F32F0` (NTSC-U), only after the controller reaches initialized state `1`; its normal update (which can change the hero fall state) remains paused.

Veldin Orbit's missing water/ripple surface is drawn by the shared `ripple_strips_moby_update` controller (classes `1405`, `1406`, `1407`, `1426`), which queues the radial mesh renderer every frame after initialisation. The missing blue-tinted reflective glass is drawn by `draw_only_moby_update` (classes `1907`, `1908`, `1910`, `1911`, `1913`, `1918`), which likewise only initialises or queues reflection meshes. The `testing` branch permits these class updates on level `17` only. Unlike class `99`, none of these class update routines needs gameplay or collision logic, so no additional draw-only hook is needed.

The yellow spark field at Veldin Orbit's doorway is a class-`78` linked spark emitter. Its quads are rendered by `draw_spark_emitter_effect_quads`; spawning sparks and handling the linked game-state check remain paused. The exterior air-flow effect uses class `1378`, whose `draw_moby_animated_quad_layers` callback renders the six layered translucent quads. Its full update can change the hero's updraft flag, so the testing branch only requeues its draw callback. These are distinct from the green class-1392 final Veldin barrier. An in-game test is still required to identify whether both reported missing doorway effects are fully restored.

The green security lasers on the **final Veldin** (level `18`) are class `1392` (`trespasser_barrier_update`), distinct from both Veldin Orbit effects. In state `1` its original update queues `draw_trespasser_barrier_quads`, while also checking the linked Trespasser pad and potentially changing collision and sound. The testing branch queues the 12 glow quads without that state update via guest address `0x002F33D8` (PAL) or `0x002F1FD8` (NTSC-U). The state-0 setup and state-2 fade-out remain paused.

Final Veldin also has a class-`1402` lift that queues one of three 3D textured-quad meshes from `lift_update`. The test draw-only hook uses the initialized pvars mesh selector at offset `0x14`: NTSC-U callbacks `0x002F2620`, `0x002F27C8`, `0x002F2970` (PAL: `0x002F3A80`, `0x002F3C28`, `0x002F3DD0`). It holds the lift's position/commands and its shared UV scroll clock while queuing the original renderer. The `veldin_final_lift` diagnostic count shows whether this path actually runs during the weapon-wheel pause; an in-game result is still needed to establish whether this is the missing water-like surface.

Source references: [OpenRAC](https://github.com/OpenRAC/OpenRAC), [RAC1 electric-water decompilation](https://github.com/OpenRAC/rac1-decomp/blob/main/src/overlays/shared/electric_water.c) and [ripple-strip renderer](https://github.com/lombyte-project/Lombyte/blob/main/src/overlays/shared/props/ripple_strips.c).

Tests: `tests/policy_test.cpp`, `tests/veil_test.cpp`, and `tests/legacy_shade_test.cpp`.

## License

MIT — see [LICENSE](LICENSE). Authored by **glawks**.
