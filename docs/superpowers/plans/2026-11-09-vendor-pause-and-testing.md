# Vendor Pause Visuals and Testing Automation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep the Gadgetron vendor's glow visible and stationary during Quick Select, and automatically build unreleased Windows artifacts on testing-branch pushes.

**Architecture:** The moby gate admits class 11 only when a vendor-specific hook is installed, and that hook submits the existing guest draw callback without calling the stateful update. Store/restore vendor-only visual phase globals during pause. The existing Windows workflow handles `testing` as a build-only push branch.

**Tech Stack:** C++17, OpenRAC SDK 1.14.2, GitHub Actions YAML.

**Spec:** `docs/superpowers/specs/2026-11-09-vendor-pause-and-testing-design.md`

## Global Constraints

- Work on `testing` only; leave `main` and version `1.0.1` unchanged.
- No GitHub Release from an ordinary testing branch push or manual testing run.
- Keep the `keep_water` setting as the existing visual option.
- Preserve PAL and NTSC-U address separation and skip native pointers in guest draw queues.
- Full vendor logic must not run while the world gate owns the pause.

## Review Focus

- Missing vendor hook or draw function: skip safely rather than executing vendor gameplay.
- Vendor state 0/3 and missing pvars/ring: don't render with uninitialized effect data.
- Level transition or wheel closing: don't write stale phase globals.
- Vendor animation: keep the `0x40` animation-skip bit owned/restored safely.
- Build-only testing push: no release tag creation or asset overwrite.

---

### Task 1: Automatic build-only testing workflow

**Files:**
- Modify: `.github/workflows/Windows.yml`

**Interfaces:**
- Consumes: existing `workflow_dispatch` and tag publishing logic.
- Produces: an automatically uploaded installable artifact for pushes to `testing`.

- [ ] Add `testing` to `push.branches`, retaining version-tag triggers.
- [ ] Restrict manual release requests to `main`/`master` to prevent accidental testing-branch publication.
- [ ] Validate YAML and inspect event/publish conditions.

### Task 2: Vendor draw-only pause behaviour

**Files:**
- Modify: `src/pause_policy.hpp`
- Modify: `tests/policy_test.cpp`
- Modify: `src/mod.cpp`
- Modify: `openrac-mod.json`
- Modify: `README.md`

**Interfaces:**
- Consumes: SDK `openrac::rac1::Moby_Vendor_Update`, `Mod::function_at`, `AddDrawCallback` guest address convention, and existing animation gate.
- Produces: `should_route_vendor_draw`, `can_draw_vendor_effects`, `vendor_effect_code_address`, `vendor_phase_addresses`, and `VendorVisualPhaseLock` policy; native mod hook and per-frame restore.

- [ ] Write tests for vendor pause routing, eligible states, both version address pairs and locked phase behaviour.
- [ ] Run the new tests and verify RED before changing production code.
- [ ] Implement the policy and draw-only hook, animation freeze and phase locking using exact address mapping.
- [ ] Run new tests and existing suite; ensure GREEN and all regression tests pass.
- [ ] Build against OpenRAC SDK 1.14.2 and check mod manifest and packaging.
- [ ] Update README with vendor limitation/testing workflow directions.

### Task 3: Publish to testing and verify CI

**Files:**
- None beyond Task 1/2 changes and their plans.

**Interfaces:**
- Consumes: tested local changes on branch `testing`.
- Produces: remote testing-branch commit(s), an automatic Actions build for download.

- [ ] Verify branch diff contains no version bump and no unrelated changes.
- [ ] Push/commit exclusively to remote `testing`, using the linked GitHub connector if git CLI has no write credentials.
- [ ] Confirm remote files and Actions build status; report game-side validation still pending.
