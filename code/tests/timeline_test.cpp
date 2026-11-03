#include "Timeline.hpp"
#include <cassert>
#include <cmath>

int main() {
    assert(animation::progress(-1.0f) == 0.0f);
    assert(animation::duration == 35.0f);
    assert(animation::progress(17.5f) == 0.5f);
    assert(animation::progress(35.0f) == 1.0f);
    const auto start = animation::bezier(0.0f);
    const auto end = animation::bezier(1.0f);
    assert(start.x == -3.0f && start.y == 1.5f);
    assert(end.x == 3.0f && end.y == 4.5f);
    assert(animation::shot(-1).index == 0);
    assert(animation::shot(35).index == 5);
    assert(animation::aircraft(26).x == -18);
    assert(animation::aircraft(31).x == 20);
    assert(animation::aircraft(35).x == 15);
    assert(animation::flagHeight(0) == 1.35f);
    assert(std::abs(animation::flagHeight(26) - 5.55f) < 0.001f);
    const auto flagClose = animation::shot(26);
    assert(std::abs(flagClose.camera.x - 3.7f) < 0.001f);
    assert(flagClose.camera.z > 8.4f && flagClose.camera.z < 8.8f);
    assert(flagClose.camera.y > animation::flagHeight(26));
    assert(flagClose.camera.y < animation::flagHeight(26) + 1.6f);
    assert(animation::shot(28).camera.y > animation::flagHeight(28) + 1.6f);
    const auto march = animation::shot(animation::shotTimes[2]);
    assert(std::abs(march.target.x - (animation::soldierPosition(animation::shotTimes[2]) - 2)) < 0.01f);
    assert(march.target.z == animation::paradeDepth);
    assert(animation::paradeDepth - 2.5f > 6.63f);
    for (float boundary : animation::shotTimes) {
        const auto before = animation::shot(boundary - 0.001f);
        const auto after = animation::shot(boundary + 0.001f);
        assert(std::abs(before.camera.x - after.camera.x) < 0.01f);
        assert(std::abs(before.target.y - after.target.y) < 0.01f);
        if (boundary > 0 && boundary < animation::duration) {
            const auto center = animation::shot(boundary);
            const float incoming = (center.camera.x - before.camera.x) / 0.001f;
            const float outgoing = (after.camera.x - center.camera.x) / 0.001f;
            assert(std::abs(incoming - outgoing) < 0.05f);
        }
    }
    for (int frame = 0; frame <= static_cast<int>(animation::duration * 60); ++frame) {
        const float seconds = frame / 60.0f;
        assert(animation::flagHeight(seconds) >= animation::flagHeight(seconds - 1.0f / 60));
        assert(animation::tankPosition(seconds, 3) - animation::soldierPosition(seconds) >= 7.99f);
        if (frame > 0) assert(std::abs((animation::soldierPosition(seconds)
            - animation::soldierPosition(seconds - 1.0f)) - animation::soldierSpeed) < 0.001f);
        if (seconds >= animation::shotTimes[2] && seconds <= animation::shotTimes[3]) {
            assert(std::abs(animation::shot(seconds).target.x
                - (animation::soldierPosition(seconds) - 3)) < 3.0f);
        }
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