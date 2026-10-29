#include "Timeline.hpp"
#include <cassert>
#include <cmath>

int main() {
    assert(animation::progress(-1.0f) == 0.0f);
    assert(animation::progress(12.5f) == 0.5f);
    assert(animation::progress(25.0f) == 1.0f);
    assert(animation::progress(30.0f) == 1.0f);
    const auto start = animation::bezier(0.0f);
    const auto end = animation::bezier(1.0f);
    assert(start.x == -3.0f && start.y == 1.5f);
    assert(end.x == 3.0f && end.y == 4.5f);
    for (int frame = 0; frame <= 1500; ++frame) {
        const auto point = animation::bezier(animation::progress(frame / 60.0f));
        assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
        assert(point.y >= 1.5f && point.y <= 5.0f);
    }
}