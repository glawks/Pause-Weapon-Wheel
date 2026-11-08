#include "pause_policy.hpp"

#include <cassert>
#include <cstdint>

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
