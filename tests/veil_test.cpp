#include "pause_veil.hpp"

#include <cassert>
#include <cstdint>

int main() {
    using namespace pause_veil;

    // The wheel and its icons need a clear area; the surrounding scene is shaded.
    assert(alpha_at(92.0f, 220.0f, 30) == 0);
    assert(alpha_at(95.0f, 136.0f, 30) == 0);
    assert(alpha_at(450.0f, 220.0f, 30) > 0);
    assert(alpha_at(450.0f, 220.0f, 60) > alpha_at(450.0f, 220.0f, 30));
    assert(alpha_at(450.0f, 220.0f, 0) == 0);
    // The edge fades in gradually instead of forming a hard cutout.
    const auto feather = alpha_at(192.0f, 220.0f, 30);
    assert(feather > 0);
    assert(feather < alpha_at(450.0f, 220.0f, 30));

    int rects = 0;
    int clear_coverage = 0;
    for_each_rect(30, [&](float x, float y, float w, float h, std::uint32_t rgba) {
        assert(x >= 0.0f && y >= 0.0f);
        assert(w > 0.0f && h > 0.0f);
        assert(x + w <= 512.0f && y + h <= 448.0f);
        assert((rgba & 0xFF) != 0);
        if (x < 92.0f && x + w > 92.0f && y < 220.0f && y + h > 220.0f) {
            ++clear_coverage;
        }
        ++rects;
    });
    assert(rects > 0 && rects < 3000);
    assert(clear_coverage == 0);

    int disabled_rects = 0;
    for_each_rect(0, [&](float, float, float, float, std::uint32_t) { ++disabled_rects; });
    assert(disabled_rects == 0);
}
