#include "Timeline.hpp"
#include <cassert>
#include <cmath>

int main() {
    assert(animation::progress(-1.0f) == 0.0f);
    assert(animation::duration == 44.0f);
    assert(animation::progress(22.0f) == 0.5f);
    assert(animation::progress(44.0f) == 1.0f);
    assert(animation::finaleFade(41) == 0);
    assert(animation::finaleFade(42.5f) == 0.5f);
    assert(animation::finaleFade(44) == 1);
    assert(animation::shot(40).camera.y > animation::shot(35).camera.y);
    const auto start = animation::bezier(0.0f);
    const auto end = animation::bezier(1.0f);
    assert(start.x == -3.0f && start.y == 1.5f);
    assert(end.x == 3.0f && end.y == 4.5f);
    assert(animation::shot(-1).index == 0);
    assert(animation::shot(35).index == 5);
    assert(animation::aircraftStart == 18.0f);
    assert(animation::aircraft(18).x == -1500.0f);
    assert(animation::aircraft(19).x > animation::aircraft(18).x);
    assert(animation::aircraft(26).x == -144);
    assert(std::abs(animation::aircraft(31).x - 278.4f) < 0.001f);
    assert(animation::aircraft(35).z < -200.0f);
    assert(animation::aircraft(30).x - animation::aircraft(29).x > 72.0f);
    const auto finale = animation::shot(animation::flightEnd);
    assert(finale.target.x == 720 && finale.target.y == 330 && finale.target.z == -3600);
    const auto turnBefore = animation::aircraft(animation::aircraftTurn - 0.01f);
    const auto turnCenter = animation::aircraft(animation::aircraftTurn);
    const auto turnAfter = animation::aircraft(animation::aircraftTurn + 0.01f);
    assert(std::abs((turnCenter.x - turnBefore.x) - (turnAfter.x - turnCenter.x)) < 0.006f);
    assert(std::abs((turnCenter.y - turnBefore.y) - (turnAfter.y - turnCenter.y)) < 0.006f);
    assert(std::abs((turnCenter.z - turnBefore.z) - (turnAfter.z - turnCenter.z)) < 0.006f);
    const auto turnEarlier = animation::aircraft(animation::aircraftTurn - 0.02f);
    const auto turnLater = animation::aircraft(animation::aircraftTurn + 0.02f);
    const auto checkAcceleration = [](float earlier, float before, float center, float after, float later) {
        const float incoming = (center - 2 * before + earlier) / 0.0001f;
        const float outgoing = (later - 2 * after + center) / 0.0001f;
        assert(std::abs(incoming - outgoing) < 9.0f);
    };
    checkAcceleration(turnEarlier.x, turnBefore.x, turnCenter.x, turnAfter.x, turnLater.x);
    checkAcceleration(turnEarlier.y, turnBefore.y, turnCenter.y, turnAfter.y, turnLater.y);
    checkAcceleration(turnEarlier.z, turnBefore.z, turnCenter.z, turnAfter.z, turnLater.z);
    assert(animation::flagHeight(0) == 8.1f + animation::flagBaseOffset);
    assert(animation::flagHeight(0.5f) > animation::flagHeight(0));
    assert(animation::flagHeight(2.9f) < animation::flagHeight(3.0f));
    assert(std::abs(animation::flagHeight(3.0f) - (33.3f + animation::flagBaseOffset)) < 0.001f);
    assert(std::abs(animation::flagHeight(26) - (33.3f + animation::flagBaseOffset)) < 0.001f);
    const auto approachStart = animation::shot(0);
    const auto approachEnd = animation::shot(animation::shotTimes[1]);
    assert(approachStart.camera.x > animation::flagPoleX + 10);
    assert(std::hypot(approachEnd.camera.x - animation::flagPoleX,
                      approachEnd.camera.z - animation::flagPoleZ) < 48);
    assert(approachEnd.camera.z < approachStart.camera.z);
    assert(approachEnd.target.y < approachEnd.camera.y);
    const auto beforeTurn = animation::shot(0.1f).camera;
    assert(std::hypot(beforeTurn.x - approachStart.camera.x, beforeTurn.z - approachStart.camera.z) < 4.8f);
    const auto armorFront = animation::shot(7.6f);
    const auto armorPass = animation::shot(9.3f);
    assert(armorFront.target.x == animation::tankPosition(7.6f, 0));
    assert(armorFront.target.x < armorFront.camera.x - 6);
    assert(armorFront.camera.z > animation::paradeDepth + 30.0f);
    assert(armorPass.camera.x < armorFront.camera.x && armorPass.camera.y < armorFront.camera.y);
    assert(animation::soldierPosition(0, 13) - 1.0f >= animation::paradeRoadFirst);
    const auto march = animation::shot(animation::shotTimes[2]);
    assert(std::abs(march.target.x - (animation::soldierPosition(animation::shotTimes[2]) - 2)) < 0.01f);
    assert(march.target.z == animation::paradeDepth);
    assert(animation::paradeDepth - 2.5f > 6.63f);
    for (float boundary : animation::shotTimes) {
        const auto before = animation::shot(boundary - 0.001f);
        const auto after = animation::shot(boundary + 0.001f);
        assert(std::abs(before.camera.x - after.camera.x) < 0.06f);
        assert(std::abs(before.target.y - after.target.y) < 0.06f);
        if (boundary > 0 && boundary < animation::duration) {
            const auto center = animation::shot(boundary);
            const float incoming = (center.camera.x - before.camera.x) / 0.001f;
            const float outgoing = (after.camera.x - center.camera.x) / 0.001f;
            assert(std::abs(incoming - outgoing) < 0.3f);
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
        if (frame < static_cast<int>(animation::duration * 60)) {
            const auto next = animation::shot(seconds + 1.0f / 60);
            const float speed = std::sqrt(
                std::pow(next.camera.x - shot.camera.x, 2)
                + std::pow(next.camera.y - shot.camera.y, 2)
                + std::pow(next.camera.z - shot.camera.z, 2)) * 60;
            assert(speed > 0.25f);
        }
        const auto aircraft = animation::aircraft(frame / 60.0f);
        assert(std::isfinite(aircraft.x) && std::isfinite(aircraft.y) && std::isfinite(aircraft.z));
        assert(aircraft.y >= 138.0f && aircraft.y <= 300.0f);
        if (seconds >= 28.0f) {
            const float flightDistance = std::sqrt(
                std::pow(aircraft.x - shot.camera.x, 2)
                + std::pow(aircraft.y - shot.camera.y, 2)
                + std::pow(aircraft.z - shot.camera.z, 2));
            assert(flightDistance > 48.0f);
        }
        if (seconds >= animation::aircraftStart && seconds <= animation::aircraftTurn) {
            assert(shot.target.x == aircraft.x && shot.target.y == aircraft.y && shot.target.z == aircraft.z);
        }
        assert(std::isfinite(shot.camera.x) && std::isfinite(shot.target.y));
        const float distance = std::abs(shot.camera.x - shot.target.x)
            + std::abs(shot.camera.y - shot.target.y) + std::abs(shot.camera.z - shot.target.z);
        assert(distance > 0.1f);
        const auto point = animation::bezier(animation::progress(frame / 60.0f));
        assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
        assert(point.y >= 1.5f && point.y <= 5.0f);
    }
}
