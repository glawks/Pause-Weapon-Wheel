# Blackwater water visibility without flood progression

## Goal

During RAC1 level 5 (Blackwater City) Quick Select, leave the existing water
height, ripple simulation and level logic frozen, but keep the previously
prepared water grids visible. Normal gameplay must remain unchanged.

## Design

- Retain the existing pause gate for class 831 (`Moby_Water_Update`). The
  separate class 879 ocean draw controller remains unchanged.
- When `keep_water`, `world_gate`, Quick Select and the installed class 831 hook
  are all active in level 5, allow calls through the gate to a draw-only hook.
- Never call the original class 831 update on this path. It controls the timed
  flood, grid preparation, visibility, and ripple simulation.
- Enqueue the level's known `render_level_water_grids` guest callback only for
  an initialized class 831 moby whose pvars `record_index` is zero (the single
  primary water grid), at most once per frame. Leave other records untouched.
- Reuse the already-prepared water render state; do not call `pool_flush`,
  mutate grids, or alter timers/animation.
- If the expected guest callback cannot be resolved, log a one-time warning
  and keep the gameplay safely paused.

## Verification

- Unit-test route eligibility, validity of primary controller, duplicate
  guard, original-vs-draw dispatch, and exact NTSC/PAL guest draw addresses.
- Compile against the installed SDK and its API 1.12 compatibility mode.
- Build on the `testing` branch only, then ask Tom to check outdoor puddles and
  the Blackwater flooding room. In-game success requires his confirmation.
