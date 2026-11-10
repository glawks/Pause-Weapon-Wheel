# Blackwater water draw-only testing build plan

## Task 1: Regression tests (red)

Extend `tests/policy_test.cpp` to test the Blackwater-only route, primary grid
validation, one queued draw despite many controller calls, and dispatch that
does not invoke the gameplay update while paused. Confirm compile fails until
the policy is implemented.

## Task 2: Rendering hook (green)

Introduce the tested policy helpers, register `Moby_Water_Update` in
`src/mod.cpp`, forward class 831 through the moby gate only when the hook is
available, and queue `render_level_water_grids` exactly once per frame from
the primary controller. Reset the guard each `on_frame` tick. Include bounded
diagnostic counters and one-time unavailable-function warnings.

## Task 3: Verify and publish isolated test

Run native tests, API 1.12/1.14 syntax checks, shared-library build and git
diff checks. Commit changes only to `testing`, confirm main is untouched, wait
for Windows CI, and provide an artifact link and targeted in-game checks.
