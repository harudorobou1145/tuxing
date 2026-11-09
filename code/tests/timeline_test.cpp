#include "Timeline.hpp"
#include <cassert>
#include <cmath>

int main() {
    assert(animation::progress(-1.0f) == 0.0f);
    assert(animation::duration == 37.0f);
    assert(animation::progress(18.5f) == 0.5f);
    assert(animation::progress(37.0f) == 1.0f);
    assert(animation::finaleFade(34) == 0);
    assert(animation::finaleFade(35.5f) == 0.5f);
    assert(animation::finaleFade(37) == 1);
    assert(animation::balloonRelease == 29.0f);
    assert(animation::shot(37).camera.y > animation::shot(35).camera.y);
    assert(std::abs(animation::shot(37).camera.x - animation::shot(35).camera.x + 2 * 3.9f) < 0.001f);
    const auto start = animation::bezier(0.0f);
    const auto end = animation::bezier(1.0f);
    assert(start.x == -3.0f && start.y == 1.5f);
    assert(end.x == 3.0f && end.y == 4.5f);
    assert(animation::shot(-1).index == 0);
    assert(animation::shot(35).index == 5);
    assert(animation::aircraftStart == 16.0f);
    assert(animation::aircraft(16).x == -260.0f);
    assert(animation::aircraft(17).x > animation::aircraft(16).x);
    assert(animation::shot(17.99f).index == 3 && animation::shot(18.0f).index == 4);
    const auto earlyPlane = animation::aircraft(animation::marchPanMid);
    assert(earlyPlane.x < animation::soldierPosition(animation::marchPanMid));
    assert(animation::soldierPosition(animation::marchPanMid) - earlyPlane.x < 200);
    assert(std::abs(animation::aircraft(31).x - 278.4f) < 0.001f);
    assert(animation::aircraft(35).z < -200.0f);
    assert(animation::aircraft(40).x > animation::aircraft(35).x);
    assert(animation::aircraft(40).z < animation::aircraft(35).z);
    const auto flightOrigin = animation::aircraft(animation::aircraftStart);
    const auto flightReference = animation::aircraft(animation::aircraftTurn);
    for (float seconds = 22; seconds <= 31; seconds += 0.25f) {
        const auto plane = animation::aircraft(seconds);
        const float along = (plane.x - flightOrigin.x) / (flightReference.x - flightOrigin.x);
        assert(std::abs(plane.y - (flightOrigin.y + along * (flightReference.y - flightOrigin.y))) < 0.001f);
        assert(std::abs(plane.z - (flightOrigin.z + along * (flightReference.z - flightOrigin.z))) < 0.001f);
    }
    const auto finale = animation::shot(animation::flightEnd);
    const auto viewingPitch = [](const animation::Shot& s) {
        return std::atan2(s.target.y - s.camera.y,
            std::hypot(s.target.x - s.camera.x, s.target.z - s.camera.z));
    };
    assert(viewingPitch(animation::shot(37)) > viewingPitch(finale));
    for (float boundary : {24.0f, animation::aircraftTurn, animation::flightEnd}) {
        const auto before = animation::aircraft(boundary - 0.01f);
        const auto center = animation::aircraft(boundary);
        const auto after = animation::aircraft(boundary + 0.01f);
        assert(std::abs((center.x - before.x) - (after.x - center.x)) < 0.006f);
        assert(std::abs((center.y - before.y) - (after.y - center.y)) < 0.006f);
        assert(std::abs((center.z - before.z) - (after.z - center.z)) < 0.006f);
    }
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
    const auto armorEntry = animation::shot(7.6f);
    const auto armorCenter = animation::shot(animation::armorCentered);
    assert(armorEntry.camera.z > animation::paradeDepth + 11.9f);
    assert(armorCenter.camera.z == animation::paradeDepth);
    assert(armorCenter.camera.x < armorEntry.camera.x && armorCenter.camera.y < armorEntry.camera.y);
    assert(animation::soldierPosition(0, 13) - 1.0f >= animation::paradeRoadFirst);
    const auto reflection = animation::shot(animation::reflectionStart);
    assert(animation::reflectionStart >= 14.0f && animation::reflectionStart <= 15.0f);
    assert(reflection.index == 2);
    assert(reflection.target.x == animation::reflectionBall.x && reflection.target.y == animation::reflectionBall.y
        && reflection.target.z == animation::reflectionBall.z);
    const auto march = animation::shot(animation::marchStart);
    assert(march.index == 3);
    assert(march.camera.y > 9.0f && march.camera.y < 12.0f);
    assert(march.target.y < march.camera.y && march.target.x < march.camera.x);
    assert(march.camera.x > animation::soldierPosition(animation::marchStart));
    assert(march.camera.z > animation::paradeDepth + 9.3f);
    const auto panEnd = animation::shot(animation::aircraftTrack);
    const auto yaw = [](const animation::Shot& s) {
        return std::atan2(s.target.x - s.camera.x, s.camera.z - s.target.z);
    };
    assert(std::abs(yaw(panEnd) - yaw(march)) < 0.6f);
    assert(animation::shot(animation::aircraftTrack).camera.y > march.camera.y + 5);
    assert(animation::paradeDepth - 2.5f > 6.63f);
    for (float boundary : {0.0f, animation::shotTimes[1], 6.9f, 7.6f,
                           animation::marchLiftStart, animation::flypastStart, animation::marchPanMid,
                           animation::aircraftTrack, 24.0f, animation::cameraJoin,
                           animation::aircraftTurn, animation::flightEnd, animation::duration}) {
        const auto before = animation::shot(boundary - 0.001f);
        const auto after = animation::shot(boundary + 0.001f);
        assert(std::abs(before.camera.x - after.camera.x) < 0.06f);
        assert(std::abs(viewingPitch(before) - viewingPitch(after)) < 0.002f);
        if (boundary > 0 && boundary < animation::duration) {
            const auto center = animation::shot(boundary);
            const float incoming = (center.camera.x - before.camera.x) / 0.001f;
            const float outgoing = (after.camera.x - center.camera.x) / 0.001f;
            assert(std::abs(incoming - outgoing) < 0.3f);
        }
    }
    const auto direction = [](const animation::Shot& s) {
        const animation::Point d{s.target.x - s.camera.x, s.target.y - s.camera.y, s.target.z - s.camera.z};
        const float length = std::hypot(d.x, d.y, d.z);
        return animation::Point{d.x / length, d.y / length, d.z / length};
    };
    constexpr float frameTime = 1.0f / 60;
    constexpr int lastFrame = static_cast<int>(animation::duration * 60);
    const auto take = [](int index) { return index < 2 ? 0 : std::min(index, 3); };
    float previousArmorStep = 1.0f;
    for (int frame = 0; frame <= lastFrame; ++frame) {
        const float seconds = frame * frameTime;
        assert(animation::flagHeight(seconds) >= animation::flagHeight(seconds - 1.0f / 60));
        assert(animation::tankPosition(seconds, 3) - animation::soldierPosition(seconds) >= 7.99f);
        if (frame > 0) assert(std::abs((animation::soldierPosition(seconds)
            - animation::soldierPosition(seconds - 1.0f)) - animation::soldierSpeed) < 0.001f);
        const auto shot = animation::shot(seconds);
        const auto next = animation::shot((frame + 1) * frameTime);
        if (seconds >= animation::armorCentered && (frame + 1) * frameTime < animation::reflectionStart) {
            assert(shot.camera.z == animation::paradeDepth && shot.target.z == animation::paradeDepth);
            assert(next.camera.x < shot.camera.x && next.camera.y < shot.camera.y);
            assert(shot.target.x < shot.camera.x - 20);
            const float step = std::hypot(next.camera.x - shot.camera.x, next.camera.y - shot.camera.y);
            assert(step <= previousArmorStep + 0.00002f);
            previousArmorStep = step;
        }
        if (seconds >= animation::marchStart && seconds < animation::cameraJoin) {
            assert(next.camera.y >= shot.camera.y - 0.001f);
        }
        if (seconds >= animation::reflectionStart && seconds < animation::marchStart) {
            assert(shot.target.x == animation::reflectionBall.x && shot.target.y == animation::reflectionBall.y
                && shot.target.z == animation::reflectionBall.z);
            assert(shot.camera.z > animation::paradeDepth + 11.9f);
        }
        // Bound motion on every frame within each continuous take, including shot-label changes.
        if (frame < lastFrame && take(shot.index) == take(next.index)) {
            const float speed = std::hypot(next.camera.x - shot.camera.x, next.camera.y - shot.camera.y,
                                           next.camera.z - shot.camera.z) / frameTime;
            assert(speed > 0.25f);
            const auto a = direction(shot), b = direction(next);
            constexpr float degreesPerRadian = 57.29578f;
            assert(std::hypot(b.x - a.x, b.y - a.y, b.z - a.z) * degreesPerRadian / frameTime < 32);
            if (frame > 0) {
                const auto previous = animation::shot((frame - 1) * frameTime);
                if (take(previous.index) == take(shot.index)) {
                    const auto p = previous.camera, q = shot.camera, r = next.camera;
                    assert(std::hypot(r.x - 2 * q.x + p.x, r.y - 2 * q.y + p.y,
                                      r.z - 2 * q.z + p.z) / (frameTime * frameTime) < 25);
                    const auto c = direction(previous);
                    assert(std::hypot(b.x - 2 * a.x + c.x, b.y - 2 * a.y + c.y,
                                      b.z - 2 * a.z + c.z) * degreesPerRadian / (frameTime * frameTime) < 35);
                }
            }
        }
        const auto aircraft = animation::aircraft(seconds);
        assert(std::isfinite(aircraft.x) && std::isfinite(aircraft.y) && std::isfinite(aircraft.z));
        assert(aircraft.y >= 115.0f && aircraft.y <= 300.0f);
        if (seconds >= 28.0f) {
            const float flightDistance = std::sqrt(
                std::pow(aircraft.x - shot.camera.x, 2)
                + std::pow(aircraft.y - shot.camera.y, 2)
                + std::pow(aircraft.z - shot.camera.z, 2));
            assert(flightDistance > 48.0f);
        }
        if (seconds >= animation::aircraftTrack && seconds <= animation::aircraftTurn) {
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
