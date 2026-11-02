#include "Timeline.hpp"
#include <cassert>
#include <cmath>

int main() {
    assert(animation::progress(-1.0f) == 0.0f);
    assert(animation::progress(15.0f) == 0.5f);
    assert(animation::progress(25.0f) == 25.0f / 30.0f);
    assert(animation::progress(30.0f) == 1.0f);
    const auto start = animation::bezier(0.0f);
    const auto end = animation::bezier(1.0f);
    assert(start.x == -3.0f && start.y == 1.5f);
    assert(end.x == 3.0f && end.y == 4.5f);
    assert(animation::shot(-1).index == 0);
    assert(animation::shot(25).index == 4);
    assert(animation::aircraft(20).x == -18);
    assert(animation::aircraft(25).x == 20);
    assert(animation::aircraft(30).x == 15);
    const auto march = animation::shot(12.5f);
    assert(std::abs(march.target.x - (-12.85f)) < 0.01f);
    assert(march.target.z == 5.0f);
    for (int boundary = 5; boundary <= 20; boundary += 5) {
        const auto before = animation::shot(boundary - 0.001f);
        const auto after = animation::shot(boundary + 0.001f);
        assert(std::abs(before.camera.x - after.camera.x) < 0.01f);
        assert(std::abs(before.target.y - after.target.y) < 0.01f);
    }
    for (int frame = 0; frame <= 1800; ++frame) {
        const auto shot = animation::shot(frame / 60.0f);
        const auto aircraft = animation::aircraft(frame / 60.0f);
        assert(std::isfinite(aircraft.x) && std::isfinite(aircraft.y) && std::isfinite(aircraft.z));
        assert(aircraft.y >= 23.0f && aircraft.y <= 50.0f);
        assert(std::isfinite(shot.camera.x) && std::isfinite(shot.target.y));
        const float distance = std::abs(shot.camera.x - shot.target.x)
            + std::abs(shot.camera.y - shot.target.y) + std::abs(shot.camera.z - shot.target.z);
        assert(distance > 0.1f);
        const auto point = animation::bezier(animation::progress(frame / 60.0f));
        assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
        assert(point.y >= 1.5f && point.y <= 5.0f);
    }
}