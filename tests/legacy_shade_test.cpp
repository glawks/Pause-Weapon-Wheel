#include "pause_legacy_shade.hpp"

#include <cassert>
#include <cstdint>

int main() {
    assert(pause_legacy_shade::game_rect_address(false) == 0x001F52A0u);
    assert(pause_legacy_shade::game_rect_address(true) == 0x001F5650u);

    assert(pause_legacy_shade::colour_for_strength(0) == 0x00000000u);
    assert(pause_legacy_shade::colour_for_strength(30) == 0x26000000u);
    assert(pause_legacy_shade::colour_for_strength(60) == 0x4D000000u);
    assert(pause_legacy_shade::colour_for_strength(100) == 0x4D000000u);

    assert(!pause_legacy_shade::should_draw(false, true, true));
    assert(!pause_legacy_shade::should_draw(true, false, true));
    assert(!pause_legacy_shade::should_draw(true, true, false));
    assert(pause_legacy_shade::should_draw(true, true, true));
}
