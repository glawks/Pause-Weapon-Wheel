# Vendor visuals during Quick Select and automatic testing builds

## Intent

On every RAC1 planet, the Gadgetron vendor's light beam and holographic
effects remain visible while Quick Select is held, but the vendor, its
lettering, its rings and all effect motion stay frozen. Normal gameplay and
vendor interaction remain blocked until the wheel closes.

Develop and test this as an unreleased change on `testing`, leaving `main`
and the published 1.0.1 release untouched. Pushes to `testing` should build
downloadable Windows installable mod ZIP artifacts automatically, never
publish releases.

## Approach and constraints

- Use the existing `keep_water` scenery option; no new user-facing setting.
- The vendor is class 11, which the mod's gate currently skips. Its update
  also handles shop activation and increments pulse/rotation by fixed amounts,
  so a zero-frame-time Hoven-style full update is unsafe.
- Use OpenRAC SDK 1.14.2's `Moby_Vendor_Update` hook and a visual-only
  draw-callback registration, as with Quartu's electric water; only the
  initialized vendor in states 1/2 is eligible. Keep the original update
  blocked during the pause. The vendor orb class 1143 remains gated.
- The vendor draw callback itself advances two texture/beam phases; snapshot
  and restore them each paused frame so effects do not scroll. Do not retain
  a snapshot across levels or after unpausing.
- Use verified guest addresses for PAL and NTSC-U, and never enqueue a native
  function pointer as a guest callback address. Fall back safely when an
  address/hook is unavailable.
- Preserve version 1.0.1 during testing. No new release or tag.
- Trigger Actions builds automatically on pushes to `testing`. Only a tag
  push or an explicit manual release request on `main`/`master` may publish.

## Alternatives considered

- **Run vendor update with frame time zero:** rejected because its fixed
  animation increments and vendor interaction handling ignore frame delta.
- **Allow vendor's full update:** rejected; it would advance animations and
  potentially open the shop during Quick Select.
- **Draw-only registration plus phase freezing:** selected; it isolates the
  required visuals from gameplay and follows the existing Quartu pattern.

## Verification

Unit-test routing, non-eligible vendor states, version-specific guest
addresses, and preservation/restoration of the two visual phases. Compile
and run existing tests, build the mod against SDK 1.14.2, and inspect the
automated testing Actions run. In-game appearance and behaviour require
Tom's manual check after downloading the artifact.
