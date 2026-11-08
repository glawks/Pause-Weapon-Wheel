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
    assert(needs_update_for_water(5, 831));
    assert(needs_update_for_water(7, 902));
    assert(needs_update_for_water(7, 948));
    assert(needs_update_for_water(7, 955));
    assert(needs_update_for_water(7, 971));
    assert(needs_update_for_water(11, 1158));
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
