#include "pause_policy.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>

int main() {
    using namespace pause_policy;
    assert(needs_update_for_water(1, 751));
    // Novalis's static-strip water needs a per-frame draw callback too.
    assert(needs_update_for_water(1, 676));
    assert(needs_update_for_water(1, 678));
    assert(needs_update_for_water(1, 761));
    assert(needs_update_for_water(1, 1225));
    // Aridia's ocean/mud class must register its draw callback while paused.
    assert(needs_update_for_water(2, 854));
    assert(!needs_update_for_water(1, 854));
    // The ship's separate glass draw callbacks are needed on every planet.
    const int levels_with_ship_glass[] = {0, 4, 12, 18};
    for (const int level : levels_with_ship_glass) {
        for (int ship_glass_class = 530; ship_glass_class <= 533; ++ship_glass_class) {
            assert(needs_update_for_water(level, ship_glass_class));
        }
        assert(!needs_update_for_water(level, 529));
        assert(!needs_update_for_water(level, 534));
    }
    assert(needs_update_for_water(5, 831));
    // Blackwater City has a separate ocean-grid callback controller.
    assert(needs_update_for_water(5, 879));
    assert(!needs_update_for_water(4, 879));
    assert(!needs_update_for_water(6, 879));
    // Nebula G34: the dual-texture quad controller must enqueue its glass draw.
    assert(needs_update_for_water(6, 1062));
    assert(!needs_update_for_water(5, 1062));
    assert(!needs_update_for_water(7, 1062));
    // Don't advance Nebula's breakable-window state machine just to show glass.
    assert(!needs_update_for_water(6, 1083));
    assert(needs_update_for_water(7, 902));
    // Umbris's ocean/mud-looking surface is a separate class 1018 draw callback.
    assert(needs_update_for_water(7, 1018));
    assert(!needs_update_for_water(5, 1018));
    assert(!needs_update_for_water(6, 1018));
    // Batalia's water-surface controller enqueues its own draw callback.
    assert(needs_update_for_water(8, 327));
    assert(!needs_update_for_water(7, 327));
    assert(!needs_update_for_water(9, 327));
    // Two Batalia-only scrolling mesh renderers register additional
    // textured-surface callbacks independently of its main water plane.
    assert(needs_update_for_water(8, 600));
    assert(needs_update_for_water(8, 1649));
    assert(!needs_update_for_water(7, 600));
    assert(!needs_update_for_water(9, 1649));
    // Do not broadly release unrelated flickering/light controllers.
    assert(!needs_update_for_water(8, 671));
    // Gaspar repurposes class 317 to register its lava-grid render callback.
    assert(needs_update_for_water(9, 317));
    assert(!needs_update_for_water(7, 317));
    assert(!needs_update_for_water(8, 317));
    assert(needs_update_for_water(7, 948));
    assert(needs_update_for_water(7, 955));
    assert(needs_update_for_water(7, 971));
    // Pokitaru's water tiles do not draw the ocean or scrolling textured flow.
    assert(needs_update_for_water(11, 1158));
    assert(needs_update_for_water(11, 1111));
    assert(needs_update_for_water(11, 1903));
    assert(!needs_update_for_water(10, 1111));
    assert(!needs_update_for_water(12, 1903));
    // Keep the separate effect controller frozen until its purpose is verified.
    assert(!needs_update_for_water(11, 1919));
    assert(needs_update_for_water(12, 19));
    // Hoven's large textured-effect surface and reservoir are not class 19.
    assert(needs_update_for_water(12, 293));
    assert(needs_update_for_water(12, 384));
    // The broad scrolling surface on Hoven is drawn by this class.
    assert(needs_update_for_water(12, 1901));
    assert(needs_update_for_water(13, 1263));
    assert(needs_update_for_water(13, 1393));
    // Oltanis scenery draws are enqueued by a separate effect controller.
    assert(needs_update_for_water(14, 1418));
    // Oltanis burning buildings use class 760 to enqueue smoke/fire drawing.
    assert(needs_update_for_water(14, 760));
    // Oltanis rain and rain splashes are spawned by weather class 1400.
    assert(needs_update_for_water(14, 1400));
    // Oltanis's low-lying liquid/fog-like plane is queued by class 1260.
    assert(needs_update_for_water(14, 1260));
    assert(!needs_update_for_water(13, 1260));
    // Class 809 only advances their scroll phase; leave it frozen.
    assert(!needs_update_for_water(14, 809));
    // Quartu's ripple strips use separate classes in the first water area
    // and electric-water room; the shock gameplay controller stays gated.
    assert(needs_update_for_water(15, 1408));
    assert(needs_update_for_water(15, 1409));
    assert(needs_update_for_water(15, 1565));
    assert(needs_update_for_water(15, 1567));
    assert(!needs_update_for_water(15, 655));
    assert(!needs_update_for_water(14, 1408));
    assert(!needs_update_for_water(16, 1408));
    // The Quartu electric controller is routed only when its draw-only hook
    // is present; never turn the hazard's full update loose during a pause.
    assert(should_route_quartu_electric_draw(true, true, true, true, 15, 655));
    assert(!should_route_quartu_electric_draw(true, true, true, false, 15, 655));
    assert(!should_route_quartu_electric_draw(true, true, false, true, 15, 655));
    assert(!should_route_quartu_electric_draw(true, false, true, true, 15, 655));
    assert(!should_route_quartu_electric_draw(false, true, true, true, 15, 655));
    assert(!should_route_quartu_electric_draw(true, true, true, true, 17, 655));
    assert(!should_route_quartu_electric_draw(true, true, true, true, 15, 1565));

    // Kalebo III's purple doorway wall is drawn by linked class-552 pairs.
    // Only the initialized group leader in active state 1 may refresh its
    // ribbon draw callback; avoid initialization and follower updates.
    assert(should_route_kalebo_wall_ribbons(true, true, true, true, 16, 552, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(false, true, true, true, 16, 552, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(true, false, true, true, 16, 552, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, false, true, 16, 552, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, true, false, 16, 552, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, true, true, 15, 552, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, true, true, 16, 546, 1, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, true, true, 16, 552, 0, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, true, true, 16, 552, 2, 0));
    assert(!should_route_kalebo_wall_ribbons(true, true, true, true, 16, 552, 1, 2));

    // Drek's Fleet / Veldin Orbit: class-99 laser beams share their normal
    // update with collision hit tests. Only route them through a draw-only
    // hook on this planet, after their beam geometry is initialized.
    assert(should_route_fleet_laser_draw(true, true, true, true, 17, 99));
    assert(!should_route_fleet_laser_draw(false, true, true, true, 17, 99));
    assert(!should_route_fleet_laser_draw(true, false, true, true, 17, 99));
    assert(!should_route_fleet_laser_draw(true, true, false, true, 17, 99));
    assert(!should_route_fleet_laser_draw(true, true, true, false, 17, 99));
    assert(!should_route_fleet_laser_draw(true, true, true, true, 16, 99));
    assert(!should_route_fleet_laser_draw(true, true, true, true, 18, 99));
    assert(!should_route_fleet_laser_draw(true, true, true, true, 17, 655));
    assert(can_draw_fleet_laser(1, 0x1000u, 0.09f));
    assert(can_draw_fleet_laser(2, 0x1000u, 0.09f));
    assert(!can_draw_fleet_laser(0, 0x1000u, 0.09f));
    assert(!can_draw_fleet_laser(3, 0x1000u, 0.09f));
    assert(!can_draw_fleet_laser(2, 0, 0.09f));
    assert(!can_draw_fleet_laser(1, 0x1000u, 0.0f));
    assert(!can_draw_fleet_laser(1, 0x1000u, -1.0f));
    assert(fleet_laser_draw_code_address(true) == 0x002AA880u);
    assert(fleet_laser_draw_code_address(false) == 0x002A95B8u);
    std::uint32_t fleet_draw_callback = 0;
    std::uint32_t fleet_draw_target = 0;
    enqueue_fleet_laser(false, 0x123456u,
                        [&](std::uint32_t fn, std::uint32_t moby) {
                            fleet_draw_callback = fn;
                            fleet_draw_target = moby;
                        });
    assert(fleet_draw_callback == 0x002A95B8u && fleet_draw_target == 0x123456u);

    // Veldin Orbit's distant blue-white fleet beam and purple impact glow
    // are rendered by class 1772, separately from class-99 corridor lasers.
    // The scenery controller initializes its endpoints in state 0; only
    // its initialized state may enqueue the two beam and five glow quads.
    assert(should_route_fleet_backdrop_draw(true, true, true, true, 17, 1772));
    assert(!should_route_fleet_backdrop_draw(false, true, true, true, 17, 1772));
    assert(!should_route_fleet_backdrop_draw(true, false, true, true, 17, 1772));
    assert(!should_route_fleet_backdrop_draw(true, true, false, true, 17, 1772));
    assert(!should_route_fleet_backdrop_draw(true, true, true, false, 17, 1772));
    assert(!should_route_fleet_backdrop_draw(true, true, true, true, 16, 1772));
    assert(!should_route_fleet_backdrop_draw(true, true, true, true, 17, 99));
    assert(can_draw_fleet_backdrop(1, 0x1000u));
    assert(!can_draw_fleet_backdrop(0, 0x1000u));
    assert(!can_draw_fleet_backdrop(1, 0u));
    assert(fleet_backdrop_draw_code_address(true) == 0x002F4E30u);
    assert(fleet_backdrop_draw_code_address(false) == 0x002F32F0u);
    std::uint32_t backdrop_draw_callback = 0;
    std::uint32_t backdrop_draw_target = 0;
    enqueue_fleet_backdrop(false, 0x123456u,
                           [&](std::uint32_t fn, std::uint32_t target) {
                               backdrop_draw_callback = fn;
                               backdrop_draw_target = target;
                           });
    assert(backdrop_draw_callback == 0x002F32F0u);
    assert(backdrop_draw_target == 0x123456u);

    // Final Veldin (level 18): class 1392's green Trespasser barrier must
    // retain its glow quads without executing the pad-linked update that
    // can switch the barrier off and alter collision or sound state.
    assert(should_route_veldin_final_barrier_draw(true, true, true, true, 18, 1392));
    assert(!should_route_veldin_final_barrier_draw(false, true, true, true, 18, 1392));
    assert(!should_route_veldin_final_barrier_draw(true, false, true, true, 18, 1392));
    assert(!should_route_veldin_final_barrier_draw(true, true, false, true, 18, 1392));
    assert(!should_route_veldin_final_barrier_draw(true, true, true, false, 18, 1392));
    assert(!should_route_veldin_final_barrier_draw(true, true, true, true, 17, 1392));
    assert(!should_route_veldin_final_barrier_draw(true, true, true, true, 18, 1402));
    assert(can_draw_veldin_final_barrier(1, 0x1000u));
    assert(!can_draw_veldin_final_barrier(0, 0x1000u));
    assert(!can_draw_veldin_final_barrier(2, 0x1000u));
    assert(!can_draw_veldin_final_barrier(1, 0u));
    assert(veldin_final_barrier_draw_code_address(true) == 0x002F33D8u);
    assert(veldin_final_barrier_draw_code_address(false) == 0x002F1FD8u);
    std::uint32_t final_barrier_callback = 0;
    std::uint32_t final_barrier_target = 0;
    enqueue_veldin_final_barrier(false, 0x123456u,
                                 [&](std::uint32_t fn, std::uint32_t target) {
                                     final_barrier_callback = fn;
                                     final_barrier_target = target;
                                 });
    assert(final_barrier_callback == 0x002F1FD8u);
    assert(final_barrier_target == 0x123456u);

    // Final Veldin's class-1402 shaft/lift uses three textured meshes.
    // Preserve the selected draw callback while its height/command logic
    // remains frozen; never run the full update just to draw the surface.
    assert(should_route_veldin_final_lift_draw(true, true, true, true, 18, 1402));
    assert(!should_route_veldin_final_lift_draw(false, true, true, true, 18, 1402));
    assert(!should_route_veldin_final_lift_draw(true, false, true, true, 18, 1402));
    assert(!should_route_veldin_final_lift_draw(true, true, false, true, 18, 1402));
    assert(!should_route_veldin_final_lift_draw(true, true, true, false, 18, 1402));
    assert(!should_route_veldin_final_lift_draw(true, true, true, true, 17, 1402));
    assert(!should_route_veldin_final_lift_draw(true, true, true, true, 18, 1392));
    assert(can_draw_veldin_final_lift(2, 0x1000u, 0));
    assert(can_draw_veldin_final_lift(3, 0x1000u, 1));
    assert(can_draw_veldin_final_lift(2, 0x1000u, 2));
    assert(!can_draw_veldin_final_lift(0, 0x1000u, 0));
    assert(!can_draw_veldin_final_lift(1, 0x1000u, 0));
    assert(!can_draw_veldin_final_lift(2, 0u, 0));
    assert(!can_draw_veldin_final_lift(2, 0x1000u, -1));
    assert(!can_draw_veldin_final_lift(2, 0x1000u, 3));
    assert(veldin_final_lift_draw_code_address(false, 0) == 0x002F2620u);
    assert(veldin_final_lift_draw_code_address(false, 1) == 0x002F27C8u);
    assert(veldin_final_lift_draw_code_address(false, 2) == 0x002F2970u);
    assert(veldin_final_lift_draw_code_address(true, 0) == 0x002F3A80u);
    assert(veldin_final_lift_draw_code_address(true, 1) == 0x002F3C28u);
    assert(veldin_final_lift_draw_code_address(true, 2) == 0x002F3DD0u);
    assert(veldin_final_lift_draw_code_address(true, 3) == 0u);
    std::uint32_t final_lift_callback = 0;
    std::uint32_t final_lift_target = 0;
    enqueue_veldin_final_lift(false, 1, 0x654321u,
                              [&](std::uint32_t fn, std::uint32_t target) {
                                  final_lift_callback = fn;
                                  final_lift_target = target;
                              });
    assert(final_lift_callback == 0x002F27C8u);
    assert(final_lift_target == 0x654321u);

    // Use only an initialized, active bolt. Ignore dormant hazards and
    // missing/deallocated effect storage rather than drawing garbage.
    assert(can_draw_quartu_electric_arc(1, 0x1000, 0));
    assert(!can_draw_quartu_electric_arc(0, 0x1000, 0));
    assert(!can_draw_quartu_electric_arc(2, 0x1000, 0));
    assert(!can_draw_quartu_electric_arc(3, 0x1000, 0));
    assert(!can_draw_quartu_electric_arc(1, 0, 0));
    assert(!can_draw_quartu_electric_arc(1, 0x1000, -1));

    int hazard_updates = 0;
    int arc_draws = 0;
    dispatch_quartu_electric_update(true, true,
                                    [&] { ++hazard_updates; },
                                    [&] { ++arc_draws; });
    assert(hazard_updates == 0 && arc_draws == 1);
    dispatch_quartu_electric_update(true, false,
                                    [&] { ++hazard_updates; },
                                    [&] { ++arc_draws; });
    assert(hazard_updates == 0 && arc_draws == 1);
    dispatch_quartu_electric_update(false, true,
                                    [&] { ++hazard_updates; },
                                    [&] { ++arc_draws; });
    assert(hazard_updates == 1 && arc_draws == 1);

    // OpenRAC's callback list stores 32-bit *guest code addresses*, not
    // 64-bit native function pointers returned by Mod::function_at().
    std::uint32_t queued_code = 0;
    std::uint32_t queued_moby = 0;
    const auto capture_guest_draw = [&](std::uint32_t code, std::uint32_t moby) {
        queued_code = code;
        queued_moby = moby;
    };
    enqueue_quartu_electric_bolt(false, 0x123456u, capture_guest_draw);
    assert(queued_code == 0x002D8710u); // NTSC-U guest address
    assert(queued_moby == 0x123456u);
    enqueue_quartu_electric_bolt(true, 0x654321u, capture_guest_draw);
    assert(queued_code == 0x002D9B00u); // PAL guest address
    assert(queued_moby == 0x654321u);

    // The Gadgetron vendor update draws its beam but also advances motion,
    // changes proximity state and can open the shop. Keep the whole update
    // gated and submit its existing draw callback only while paused.
    assert(should_route_vendor_draw(true, true, true, true, 11));
    assert(!should_route_vendor_draw(false, true, true, true, 11));
    assert(!should_route_vendor_draw(true, false, true, true, 11));
    assert(!should_route_vendor_draw(true, true, false, true, 11));
    assert(!should_route_vendor_draw(true, true, true, false, 11));
    assert(!should_route_vendor_draw(true, true, true, true, 1143));
    assert(!should_route_vendor_draw(true, true, true, true, 655));

    // States 1 and 2 have an initialized ring and live visual data.
    assert(can_draw_vendor_effects(1, 0x1000u, 0x2000u));
    assert(can_draw_vendor_effects(2, 0x1000u, 0x2000u));
    assert(!can_draw_vendor_effects(0, 0x1000u, 0x2000u));
    assert(!can_draw_vendor_effects(3, 0x1000u, 0x2000u));
    assert(!can_draw_vendor_effects(1, 0, 0x2000u));
    assert(!can_draw_vendor_effects(2, 0x1000u, 0));
    // A phase of zero has full alpha but zero-area geometry. Hold a bright,
    // visible mid-sized glow instead of freezing at an arbitrary dim phase.
    assert(vendor_full_glow_alpha_phase() == 0.34f);
    VendorBrightnessHold vendor_brightness;
    vendor_brightness.capture(9, 0x300000u, 0x301000u, 123u, 0x400000u, 0.25f);
    vendor_brightness.capture(9, 0x300000u, 0x301000u, 123u, 0x400000u, 0.75f);
    assert(vendor_brightness.active && vendor_brightness.saved_glow == 0.25f);
    float vendor_current_glow = 1.0f;
    assert(!vendor_brightness.restore(8, 0x300000u, 0x301000u, 123u, 0x400000u,
                                     [&](float) { assert(false); }));
    assert(!vendor_brightness.restore(9, 0x300000u, 0x301000u, 124u, 0x400000u,
                                     [&](float) { assert(false); }));
    assert(vendor_brightness.restore(9, 0x300000u, 0x301000u, 123u, 0x400000u,
                                     [&](float value) { vendor_current_glow = value; }));
    assert(vendor_current_glow == 0.25f);
    vendor_brightness.reset();
    assert(!vendor_brightness.active);

    // The original vendor update is needed to keep its beam's render state
    // alive, but state 2 can open the shop. During a paused visual refresh,
    // it must see state 1 and a frame counter that skips proximity checks.
    std::uint8_t live_vendor_state = 2;
    std::uint32_t vendor_frame_counter = 16;
    float live_vendor_glow = 0.7f;
    int original_vendor_visual_updates = 0;
    with_vendor_visual_state(
        [&] { return live_vendor_state; },
        [&](std::uint8_t v) { live_vendor_state = v; },
        [&] { return vendor_frame_counter; },
        [&](std::uint32_t v) { vendor_frame_counter = v; },
        [&](float v) { live_vendor_glow = v; },
        [&] {
            assert(live_vendor_state == 1);
            assert((vendor_frame_counter & 7u) != 0u);
            assert(live_vendor_glow == 1.1f);
            live_vendor_glow -= 0.1f; // Retail state-1 update.
            ++original_vendor_visual_updates;
        });
    assert(original_vendor_visual_updates == 1);
    assert(live_vendor_state == 2 && vendor_frame_counter == 16);
    assert(live_vendor_glow == 1.0f);

    // The vendor's original update also rotates the two ring manipulators
    // by a fixed amount even with zero frame time. Restore all eight
    // quaternion components and the sphere's size. The original state-1
    // update writes a new ring scale, which caused a visible size change
    // on opening Quick Select; preserve it before rebuilding the matrix.
    float vendor_ring_quats[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float vendor_ring_scale = 2.35f;
    int vendor_matrix_rebuilds = 0;
    with_vendor_ring_pose_held(
        [&](int i) { return vendor_ring_quats[i]; },
        [&](int i, float v) { vendor_ring_quats[i] = v; },
        [&] { return vendor_ring_scale; },
        [&](float v) { vendor_ring_scale = v; },
        [&] {
            for (float& v : vendor_ring_quats) v = 99.0f;
            vendor_ring_scale = 1.5f; // Simulates original vendor_update.
        },
        [&] {
            assert(vendor_ring_scale == 2.35f);
            ++vendor_matrix_rebuilds;
        });
    for (int i = 0; i < 8; ++i) {
        assert(vendor_ring_quats[i] == static_cast<float>(i + 1));
    }
    assert(vendor_ring_scale == 2.35f);
    assert(vendor_matrix_rebuilds == 1);

    int vendor_updates = 0;
    int vendor_draws = 0;
    dispatch_vendor_update(true, true,
                           [&] { ++vendor_updates; },
                           [&] { ++vendor_draws; });
    assert(vendor_updates == 0 && vendor_draws == 1);
    dispatch_vendor_update(true, false,
                           [&] { ++vendor_updates; },
                           [&] { ++vendor_draws; });
    assert(vendor_updates == 0 && vendor_draws == 1);
    dispatch_vendor_update(false, true,
                           [&] { ++vendor_updates; },
                           [&] { ++vendor_draws; });
    assert(vendor_updates == 1 && vendor_draws == 1);

    enqueue_vendor_effects(false, 0x123456u, capture_guest_draw);
    assert(queued_code == 0x002A5040u); // NTSC-U vendor draw guest code
    assert(queued_moby == 0x123456u);
    enqueue_vendor_effects(true, 0x654321u, capture_guest_draw);
    assert(queued_code == 0x002A62D0u); // PAL vendor draw guest code
    assert(queued_moby == 0x654321u);

    // Health crates create a separate class-806 Nanotech orb. Attached
    // state-1 refreshes glow with motion held; detached state-2 refreshes
    // only after guards suppress collection and restore its physics.
    assert(should_route_nanotech_draw(true, true, true, true, 806));
    assert(!should_route_nanotech_draw(false, true, true, true, 806));
    assert(!should_route_nanotech_draw(true, false, true, true, 806));
    assert(!should_route_nanotech_draw(true, true, false, true, 806));
    assert(!should_route_nanotech_draw(true, true, true, false, 806));
    assert(!should_route_nanotech_draw(true, true, true, true, 501));
    assert(!needs_update_for_water(9, 806));
    assert(can_draw_nanotech_orb(1, 0x1000u, 0x2000u, 501));
    assert(can_draw_nanotech_orb(2, 0x1000u, 0x2000u, 501));
    assert(!can_draw_nanotech_orb(0, 0x1000u, 0x2000u, 501));
    assert(!can_draw_nanotech_orb(3, 0x1000u, 0x2000u, 501));
    assert(!can_draw_nanotech_orb(4, 0x1000u, 0x2000u, 501));
    assert(!can_draw_nanotech_orb(1, 0, 0x2000u, 501));
    assert(!can_draw_nanotech_orb(1, 0x1000u, 0, 501));
    // Detached Nanotech orbs retain their own draw/particle state after
    // the destroyed crate's moby slot is recycled.
    assert(can_draw_nanotech_orb(2, 0x1000u, 0, 0));
    assert(!can_draw_nanotech_orb(2, 0, 0, 0));
    assert(!can_draw_nanotech_orb(1, 0x1000u, 0x2000u, 502));
    assert(can_emit_attached_nanotech_glow(1, 0x1000u, 0x2000u, 501, 1));
    assert(!can_emit_attached_nanotech_glow(2, 0x1000u, 0x2000u, 501, 1));
    assert(!can_emit_attached_nanotech_glow(1, 0x1000u, 0x2000u, 501, 0xFE));
    assert(!can_emit_attached_nanotech_glow(1, 0x1000u, 0x2000u, 501, 0x80));
    assert(!can_emit_attached_nanotech_glow(1, 0, 0x2000u, 501, 1));
    assert(!can_emit_attached_nanotech_glow(1, 0x1000u, 0x2000u, 502, 1));

    // A detached orb can emit the missing bright halo only with the
    // game's initialized primary-orb pointer available for safe masking.
    assert(can_emit_detached_nanotech_glow(2, 0x1000u, 0x2000u, 806));
    assert(!can_emit_detached_nanotech_glow(1, 0x1000u, 0x2000u, 806));
    assert(!can_emit_detached_nanotech_glow(3, 0x1000u, 0x2000u, 806));
    assert(!can_emit_detached_nanotech_glow(2, 0, 0x2000u, 806));
    assert(!can_emit_detached_nanotech_glow(2, 0x1000u, 0, 806));
    assert(!can_emit_detached_nanotech_glow(2, 0x1000u, 0x2000u, 501));

    // Distinguish a loose pickup that is never routed from one whose
    // primary pointer fails the safety gate. These probe results must not
    // change the actual paused update or heal Ratchet.
    using NP = NanotechPauseProbe;
    assert(nanotech_pause_probe(false, 2, false, 0, 0, false, -1) == NP::NotDrawable);
    assert(nanotech_pause_probe(true, 1, true, 0, 0, false, -1) == NP::AttachedRefresh);
    assert(nanotech_pause_probe(true, 1, false, 0, 0, false, -1) == NP::AttachedFallback);
    assert(nanotech_pause_probe(true, 2, false, 0, 0, false, -1) == NP::DetachedNoSlot);
    assert(nanotech_pause_probe(true, 2, false, 0x1000u, 0, false, -1)
           == NP::DetachedNoPrimary);
    assert(nanotech_pause_probe(true, 2, false, 0x1000u, 0x2000u, false, -1)
           == NP::DetachedOutsideArray);
    assert(nanotech_pause_probe(true, 2, false, 0x1000u, 0x2000u, true, 501)
           == NP::DetachedWrongClass);
    assert(nanotech_pause_probe(true, 2, false, 0x1000u, 0x2000u, true, 806)
           == NP::DetachedRefresh);

    // The detached pickup's visual refresh masks the primary-orb pointer.
    // NTSC-U's Nanotech update accesses D_L00_0015F63C directly. Do not
    // infer this overlay-global from core.lit's unrelated relocation offset.
    assert(nanotech_primary_moby_global(true) == 0x0015F71Cu);
    assert(nanotech_primary_moby_global(false) == 0x0015F63Cu);

    // Only temporarily mask collection and velocity: restore *every*
    // captured guest word after the visual refresh, including exceptions.
    std::array<std::uint32_t, 3> detached_words = {0x10u, 0x20u, 0x30u};
    std::uint32_t detached_values[3] = {0xA000u, 0xBBBBu, 0xCCCCu};
    const auto detached_read = [&](std::uint32_t address) {
        return detached_values[(address >> 4) - 1u];
    };
    const auto detached_write = [&](std::uint32_t address, std::uint32_t value) {
        detached_values[(address >> 4) - 1u] = value;
    };
    with_restored_guest_words(detached_words, detached_read, detached_write, [&] {
        detached_write(0x10u, 0); // Suppress health collection through the primary pointer.
        detached_write(0x20u, 0); // Hold the detached orb's falling speed.
        detached_write(0x30u, 0xDDDDu); // Allow the original visual path to mutate state.
        assert(detached_read(0x10u) == 0 && detached_read(0x20u) == 0);
    });
    assert(detached_values[0] == 0xA000u && detached_values[1] == 0xBBBBu
           && detached_values[2] == 0xCCCCu);
    bool detached_threw = false;
    try {
        with_restored_guest_words(detached_words, detached_read, detached_write, [&] {
            detached_write(0x10u, 0);
            detached_write(0x30u, 0);
            throw std::runtime_error("simulated effect error");
        });
    } catch (const std::runtime_error&) {
        detached_threw = true;
    }
    assert(detached_threw && detached_values[0] == 0xA000u
           && detached_values[2] == 0xCCCCu);

    int nanotech_updates = 0;
    int nanotech_draws = 0;
    dispatch_nanotech_update(true, true,
                             [&] { ++nanotech_updates; },
                             [&] { ++nanotech_draws; });
    assert(nanotech_updates == 0 && nanotech_draws == 1);
    dispatch_nanotech_update(true, false,
                             [&] { ++nanotech_updates; },
                             [&] { ++nanotech_draws; });
    assert(nanotech_updates == 0 && nanotech_draws == 1);
    dispatch_nanotech_update(false, true,
                             [&] { ++nanotech_updates; },
                             [&] { ++nanotech_draws; });
    assert(nanotech_updates == 1 && nanotech_draws == 1);
    int nanotech_glow_emissions = 0;
    dispatch_nanotech_visual_update(true, true, true,
                                    [&] { assert(false); },
                                    [&] { ++nanotech_draws; },
                                    [&] { ++nanotech_glow_emissions; });
    assert(nanotech_glow_emissions == 1 && nanotech_draws == 1);
    dispatch_nanotech_visual_update(true, true, false,
                                    [&] { assert(false); },
                                    [&] { ++nanotech_draws; },
                                    [&] { assert(false); });
    assert(nanotech_glow_emissions == 1 && nanotech_draws == 2);
    dispatch_nanotech_visual_update(true, false, false,
                                    [&] { assert(false); },
                                    [&] { assert(false); },
                                    [&] { assert(false); });
    // The shared Nanotech draw callback is relocated independently in
    // each level's overlay, despite the function's level-00 symbol name.
    assert(nanotech_orb_code_address(false, 0) == 0x002D9810u);
    assert(nanotech_orb_code_address(true, 0) == 0x002DACC0u);
    assert(nanotech_orb_code_address(false, 16) == 0x002D83C0u);
    assert(nanotech_orb_code_address(true, 16) == 0x002D9788u);
    assert(nanotech_orb_code_address(false, 18) == 0x002E39A0u);
    assert(nanotech_orb_code_address(true, 18) == 0x002E4D90u);
    assert(nanotech_orb_code_address(false, -1) == 0);
    assert(nanotech_orb_code_address(false, 19) == 0);
    enqueue_nanotech_orb(false, 16, 0x123456u, capture_guest_draw);
    assert(queued_code == 0x002D83C0u && queued_moby == 0x123456u);
    enqueue_nanotech_orb(true, 18, 0x654321u, capture_guest_draw);
    assert(queued_code == 0x002E4D90u && queued_moby == 0x654321u);
    queued_code = 0;
    enqueue_nanotech_orb(false, -1, 0x123456u, capture_guest_draw);
    enqueue_nanotech_orb(false, 19, 0x123456u, capture_guest_draw);
    assert(queued_code == 0); // Never enqueue an invalid guest code address.

    // Class 803's yellow twin-target connection is replenished by its
    // particle emitter; class 758 is not the paired yellow target.
    assert(should_route_paired_swingshot_glow(true, true, true, true, 803, 1));
    assert(!should_route_paired_swingshot_glow(false, true, true, true, 803, 1));
    assert(!should_route_paired_swingshot_glow(true, false, true, true, 803, 1));
    assert(!should_route_paired_swingshot_glow(true, true, false, true, 803, 1));
    assert(!should_route_paired_swingshot_glow(true, true, true, false, 803, 1));
    assert(!should_route_paired_swingshot_glow(true, true, true, true, 758, 1));
    assert(!should_route_paired_swingshot_glow(true, true, true, true, 803, 0));
    assert(!should_route_paired_swingshot_glow(true, true, true, true, 803, 2));
    const auto pal_target_clock = paired_swingshot_motion_clock_addresses(true);
    assert(pal_target_clock.frame_time == 0x0015EE6Cu);
    assert(pal_target_clock.frame_time_squared == 0x0015EE70u);
    const auto ntsc_target_clock = paired_swingshot_motion_clock_addresses(false);
    assert(ntsc_target_clock.frame_time == 0x0015ED6Cu);
    assert(ntsc_target_clock.frame_time_squared == 0x0015ED70u);
    float target_time = 0.016f;
    float target_time_sq = 0.000256f;
    int target_emissions = 0;
    with_stopped_motion_clocks(
        [&] { return target_time; }, [&](float v) { target_time = v; },
        [&] { return target_time_sq; }, [&](float v) { target_time_sq = v; },
        [&] {
            assert(target_time == 0.0f && target_time_sq == 0.0f);
            ++target_emissions;
        });
    assert(target_emissions == 1);
    assert(target_time == 0.016f && target_time_sq == 0.000256f);
    float target_yaw = 0.75f;
    float target_steering = 0.125f;
    with_preserved_motion_pair(
        [&] { return target_yaw; }, [&](float v) { target_yaw = v; },
        [&] { return target_steering; }, [&](float v) { target_steering = v; },
        [&] { target_yaw = 3.0f; target_steering = 1.0f; });
    assert(target_yaw == 0.75f && target_steering == 0.125f);
    try {
        with_preserved_motion_pair(
            [&] { return target_yaw; }, [&](float v) { target_yaw = v; },
            [&] { return target_steering; }, [&](float v) { target_steering = v; },
            [&] {
                target_yaw = 4.0f;
                target_steering = 2.0f;
                throw std::runtime_error("test exception");
            });
        assert(false);
    } catch (const std::runtime_error&) {
        assert(target_yaw == 0.75f && target_steering == 0.125f);
    }


    const auto ntsc_vendor_phases = vendor_phase_addresses(false);
    assert(ntsc_vendor_phases.sparkle_uv == 0x00161394u);
    assert(ntsc_vendor_phases.glow == 0x00161398u);
    const auto pal_vendor_phases = vendor_phase_addresses(true);
    assert(pal_vendor_phases.sparkle_uv == 0x0016145Cu);
    assert(pal_vendor_phases.glow == 0x00161460u);

    // The draw callback advances its UV/glow phases by fixed 0.01 steps,
    // even with zero frame delta. Reapply the FIRST held values each tick;
    // do not accidentally recapture each frame or write into a new level.
    VendorVisualPhaseLock vendor_phases;
    assert(!vendor_phases.active);
    vendor_phases.capture(9, 0.25f, 0.75f);
    vendor_phases.capture(9, 0.50f, 0.50f);
    float visual_uv = 0.90f;
    float visual_glow = 0.90f;
    assert(vendor_phases.restore(9, [&](float uv, float glow) {
        visual_uv = uv;
        visual_glow = glow;
    }));
    assert(visual_uv == 0.25f && visual_glow == 0.75f);
    assert(!vendor_phases.restore(8, [&](float, float) { assert(false); }));
    vendor_phases.reset();
    assert(!vendor_phases.active);
    assert(!vendor_phases.restore(9, [&](float, float) { assert(false); }));

    assert(!needs_update_for_water(14, 1409));
    assert(!needs_update_for_water(16, 1567));
    // Kalebo III's ocean/horizon effect has its own renderer, and each
    // reflection-mapped glass mesh needs a separate per-frame draw callback.
    assert(needs_update_for_water(16, 1111));
    assert(needs_update_for_water(16, 1943));
    assert(needs_update_for_water(16, 1947));
    assert(needs_update_for_water(16, 1948));
    assert(needs_update_for_water(16, 1953));
    assert(!needs_update_for_water(15, 1943));
    assert(!needs_update_for_water(17, 1953));
    // Kalebo's conveyor draws two textured strips via class 471's callback.
    assert(needs_update_for_water(16, 471));
    assert(!needs_update_for_water(15, 471));
    assert(!needs_update_for_water(17, 471));
    // Do not accidentally unpause Kalebo's doors or barriers.
    assert(!needs_update_for_water(16, 647));
    assert(!needs_update_for_water(16, 1443));
    assert(!needs_update_for_water(16, 1952));
    assert(!needs_update_for_water(7, 940));
    assert(!needs_update_for_water(1, 831));
    assert(!needs_update_for_water(2, 676));
    assert(!needs_update_for_water(2, 678));
    assert(!needs_update_for_water(2, 761));
    assert(!needs_update_for_water(2, 1225));
    assert(!needs_update_for_water(2, 751));
    assert(!needs_update_for_water(12, 999));
    assert(!needs_update_for_water(14, 1901));

    // Allow drawing Hoven's water, but do not advance its scroll clock while paused.
    assert(should_hold_hoven_scroll(true, 12, true, true));
    assert(!should_hold_hoven_scroll(false, 12, true, true));
    assert(!should_hold_hoven_scroll(true, 14, true, true));
    assert(!should_hold_hoven_scroll(true, 12, false, true));
    assert(!should_hold_hoven_scroll(true, 12, true, false));

    float game_delta = 0.02f;
    bool draw_was_enqueued = false;
    with_stopped_frame_time(
        [&] { return game_delta; },
        [&](float value) { game_delta = value; },
        [&] {
            assert(game_delta == 0.0f);
            draw_was_enqueued = true;
        });
    assert(draw_was_enqueued);
    assert(game_delta == 0.02f);

    constexpr std::uint16_t original = 0x1284;
    const auto stopped = with_animation_stopped(original);
    assert((stopped & kSkipAnimation) != 0);
    assert(with_animation_resumed(stopped) == original);
    assert(with_animation_stopped(stopped) == stopped);
    assert(with_animation_resumed(0xFFFF) == 0xFFBF);
}
