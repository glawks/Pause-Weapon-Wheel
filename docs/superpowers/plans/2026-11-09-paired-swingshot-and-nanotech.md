# Paired Swingshot and Nanotech Pause Visuals Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve yellow paired-Swingshot energy and Nanotech crate orb glow without advancing gameplay while paused.

**Architecture:** Draw class 806 independently of its update; use a zero-motion, scoped update for class 803's particle emission. Remove the unrelated class 208 cable route. Restrict everything to an installed and active world gate.

**Tech Stack:** C++17, OpenRAC SDK 1.14.2, GitHub Actions Windows x64.

**Spec:** `docs/superpowers/specs/2026-11-09-paired-swingshot-and-nanotech-design.md`

## Global Constraints

- Only commit to `testing`, never `main`, and do not tag/release.
- Preserve mod ID, version 1.0.1, and both PAL/NTSC support.
- Never invoke a Nanotech original update while paused.
- Hold both Swingshot target motion clocks to zero only during its callback.

## Review Focus

- Absent draw/hook: stay frozen, don't crash or run gameplay.
- State-zero Nanotech or null/reused crate pointer: never draw uninitialized objects.
- Nanotech collected/inactive: never unpause healing.
- Other Swingshot class `758`: leave normal update gated.
- Restoration of time globals after update, including exception paths.

## Tasks

### Task 1: Remove misplaced cable experiment

**Files:** `src/mod.cpp`, `src/pause_policy.hpp`, `tests/policy_test.cpp`, `README.md`.

- [ ] Identify and remove only class 208/209 cable-specific routing, callbacks, tests and wording.
- [ ] Keep vendor and scenery work intact and compile the mod.

### Task 2: Nanotech orb visual-only draw

**Files:** `src/mod.cpp`, `src/pause_policy.hpp`, `tests/policy_test.cpp`, `openrac-mod.json`, `README.md`.

- [ ] Write failing tests for the class 806 route, states 1/2, class 501 ownership, guest draw addresses and no-update dispatch.
- [ ] Implement a guarded `Moby_NanotechBall_Update` hook that only queues its draw callback while paused.
- [ ] Run tests to green and compile both API-1.12 and API-1.14 paths.

### Task 3: Paired Swingshot yellow connection

**Files:** `src/mod.cpp`, `src/pause_policy.hpp`, `tests/policy_test.cpp`, `openrac-mod.json`, `README.md`.

- [ ] Write failing tests for class 803 state-1 route and scoped zeroing of two clocks.
- [ ] Implement a guarded `Moby_SwingshotVersatarget_Update` hook with zeroed motion clocks for particle emission.
- [ ] Run tests to green, compile and record in-game caveat.

### Task 4: Verify and push

- [ ] Run all unit tests, warnings-as-errors compilation, and `git diff --check`.
- [ ] Create one remote GitHub commit on `testing` and verify main is unchanged.
- [ ] Wait for `testing` Windows Actions packaging and check release step was skipped.
