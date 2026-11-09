# Paired Swingshot and Nanotech Pause Visuals (testing)

## Intent

When Quick Select pauses RAC1, retain the yellow energy connection between the two spherical Swingshot targets and the luminous Nanotech pickups above health crates, on every level where these appear. Keep the underlying world/hero movement paused. The earlier Swingshot glove-to-hook cable experiment addressed the wrong effect and should be removed from `testing`.

## Constraints

- Only change `testing`; do not release or change the `1.0.1` version.
- NTSC-U and PAL support, using original guest function addresses and SDK hooks.
- No general exemptions for gameplay object updates: collision, healing, target interaction and physics must remain paused.
- Gracefully skip a missing guest draw function or an unavailable hook.
- Mark the visuals experimental until the user can test them in-game.

## Approaches

1. **Release regular moby updates:** easy, but dangerous. Nanotech orbs would heal/animate and Swingshot targets could move and select cameras. Rejected.
2. **Use original drawing with minimal visual updates:** directly queue Nanotech's draw callback, and let the paired Swingshot's particle-emitting update run with its motion clocks temporarily zeroed, freezing the targets. Selected for the experimental branch.
3. **Reimplement geometry and particles:** potentially stricter animation control, but substantially more reverse engineering and rendering risk. Defer unless tests show option 2 insufficient.

## Technical design

- Class `806` (`nanotech_ball`) is spawned from class `501` (`nanotech_crate`). `nanotech_ball_update` normally enqueues `draw_nanotech_ball` in states 1/2, but also moves the orb, collects it and modifies health. Intercept the class 806 hook and queue only its original guest draw function for initialized, live crate-linked orbs.
- Class `803` (`swingshot_swinging_versatarget`) draws its yellow connection through moving fade particles in `swingshot_versatarget_update`, not an `AddDrawCallback`. Only allow its state-1 update under a hook that temporarily zeros `g_frame_time` and `g_frame_time_sq`; this retains particle replenishment while avoiding time-based steering of the target. Existing particles may still animate; evaluate in-game before claiming perfect stillness.
- Record pure gating and guest-address policies in `pause_policy.hpp` with tests, plus assembly-independent C++ compilation checks.
