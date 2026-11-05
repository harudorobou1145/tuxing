#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace animation {
constexpr float duration = 35.0f;
constexpr float aircraftStart = 18.0f;
constexpr float aircraftPass = 26.0f;
constexpr float aircraftTurn = 31.0f;
constexpr float approachEnd = 5.3f;
constexpr float armorDollyStart = 7.0f;
constexpr float reflectionStart = 11.0f;
constexpr float marchStart = 15.0f;
constexpr float aircraftLookUp = 15.8f;
constexpr float aircraftTrack = 19.55f;
inline constexpr std::array<float, 7> shotTimes{0, approachEnd, reflectionStart, marchStart, aircraftStart, aircraftTurn, duration};
constexpr float tankSpeed = 1.25f;
constexpr float soldierSpeed = 0.65f;
constexpr float paradeDepth = 12.0f;
constexpr float roadPostOffset = 4.05f;
constexpr float roadPostCapTop = 0.76f;
constexpr float roadBallRadius = 0.205f;
constexpr float paradeRoadFirst = -87.0f;
constexpr float paradeRoadLast = 50.0f;
// Source Cylinder's center/base, transformed with the full scene's gateModel.
constexpr float flagPoleX = -0.4566794f;
constexpr float flagPoleZ = 25.6760101f;
constexpr float flagBaseOffset = 0.0731168f;
inline float tankPosition(float seconds, int index) {
    return -12.0f + (seconds - 3.0f) * tankSpeed - index * 7.0f;
}
inline float soldierPosition(float seconds, int row = 0) {
    return tankPosition(0, 3) - 8.0f + seconds * soldierSpeed - row * 1.6f;
}

struct Point {
    float x, y, z;
};

inline constexpr Point reflectionBall{-8.0f, roadPostCapTop + roadBallRadius - 0.02f, paradeDepth + roadPostOffset};

inline Point bezier(float progress) {
    const float parameter = std::clamp(progress, 0.0f, 1.0f);
    const float inverse = 1.0f - parameter;
    const Point start{-3.0f, 1.5f, 0.0f};
    const Point controlA{-1.5f, 5.0f, -2.0f};
    const Point controlB{2.0f, 1.5f, 2.0f};
    const Point end{3.0f, 4.5f, 0.0f};
    return {
        inverse * inverse * inverse * start.x + 3 * inverse * inverse * parameter * controlA.x + 3 * inverse * parameter * parameter * controlB.x + parameter * parameter * parameter * end.x,
        inverse * inverse * inverse * start.y + 3 * inverse * inverse * parameter * controlA.y + 3 * inverse * parameter * parameter * controlB.y + parameter * parameter * parameter * end.y,
        inverse * inverse * inverse * start.z + 3 * inverse * inverse * parameter * controlA.z + 3 * inverse * parameter * parameter * controlB.z + parameter * parameter * parameter * end.z
    };
}

inline float progress(float seconds) {
    return std::clamp(seconds / duration, 0.0f, 1.0f);
}

inline float segment(float seconds, float start, float end) {
    return std::clamp((seconds - start) / (end - start), 0.0f, 1.0f);
}

inline float flagHeight(float seconds) {
    const float parameter = segment(seconds, 0.0f, 3.0f);
    const float eased = parameter * parameter * (3.0f - 2.0f * parameter);
    return 1.35f + flagBaseOffset + 4.20f * eased;
}

inline Point mix(Point start, Point end, float parameter) {
    return {start.x + (end.x - start.x) * parameter,
            start.y + (end.y - start.y) * parameter,
            start.z + (end.z - start.z) * parameter};
}

inline Point curve(Point start, Point controlA, Point controlB, Point end, float parameter) {
    return mix(mix(mix(start, controlA, parameter), mix(controlA, controlB, parameter), parameter),
               mix(mix(controlA, controlB, parameter), mix(controlB, end, parameter), parameter), parameter);
}

struct Shot {
    Point camera;
    Point target;
    int index;
};

inline Point aircraft(float seconds) {
    if (seconds < aircraftPass) {
        // Lead into the original pass at the same position and velocity.
        return curve({-92, 26, -30}, {-74, 26, -26}, {-54.72f, 29, -21.2f}, {-24, 29, -18},
                     segment(seconds, aircraftStart, aircraftPass));
    }
    Point position;
    if (seconds <= aircraftTurn) {
        position = curve({-24, 29, -18}, {-12, 29, -16}, {7, 30, -18}, {20, 33, -24},
                     segment(seconds, aircraftPass, aircraftTurn));
    } else {
        position = curve({20, 33, -24}, {30.4f, 35.4f, -28.8f}, {36.96f, 39.08f, -36.16f}, {43, 43, -48},
                         segment(seconds, aircraftTurn, duration));
    }
    position.x = -24.0f + (position.x + 24.0f) * 1.6f;
    const float departure = segment(seconds, aircraftTurn, duration);
    const float acceleration = departure * departure * departure;
    position.x += 55.0f * acceleration;
    position.y += 15.0f * acceleration;
    position.z -= 1200.0f * acceleration;
    return position;
}

inline Shot shot(float seconds) {
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(
        std::upper_bound(shotTimes.begin(), shotTimes.end(), time) - shotTimes.begin()) - 1,
        static_cast<int>(shotTimes.size()) - 2);
    constexpr Point flypastCamera{-30, 5.1f, 18.2f};
    if (time >= aircraftTrack) {
        Point camera = flypastCamera;
        camera.x -= 0.65f * (time - aircraftStart);
        // Follow the flypast, then ease toward the established finale framing.
        const float departure = segment(time, aircraftTurn, duration);
        const float settle = departure * departure * (3.0f - 2.0f * departure);
        const Point target = mix(aircraft(time), {120, 55, -600}, settle);
        return {camera, target, index};
    }
    if (time >= marchStart) {
        const auto marchCamera = [&](float t) {
            return curve({-23, 4.25f, flypastCamera.z}, {-26, 4.55f, flypastCamera.z},
                {-29.35f, 5.1f, flypastCamera.z}, flypastCamera, segment(t, marchStart, aircraftStart));
        };
        const Point camera = time < aircraftStart ? marchCamera(time)
            : Point{flypastCamera.x - 0.65f * (time - aircraftStart), flypastCamera.y, flypastCamera.z};
        if (time <= aircraftLookUp) return {camera, {soldierPosition(time) - 2, 1.1f, paradeDepth}, index};

        // Ease out of the downward pan before lifting away from the formation.
        constexpr float settleDuration = 0.35f;
        const float settle = segment(time, aircraftLookUp, aircraftLookUp + settleDuration);
        const float formationTime = aircraftLookUp + settleDuration * settle * (1.0f - 0.5f * settle);
        const Point formationCamera = marchCamera(formationTime);
        const Point soldierLook{camera.x + soldierPosition(formationTime) - 2 - formationCamera.x,
            camera.y + 1.1f - formationCamera.y, camera.z + paradeDepth - formationCamera.z};

        const Point entry = aircraft(aircraftStart);
        const float untilEntry = aircraftStart - time;
        const Point aircraftLook = time < aircraftStart
            ? Point{entry.x - 6.75f * untilEntry, entry.y, entry.z - 1.5f * untilEntry}
            : aircraft(time);
        const float progress = segment(time, aircraftLookUp, aircraftTrack);
        // Keep the aircraft above the frame when it starts moving, then ease into tracking it.
        const float rise = std::pow(progress, 2.2f);
        const float ease = rise * rise * (3.0f - 2.0f * rise);
        const auto distance = [&](Point point) {
            return std::hypot(point.x - camera.x, point.y - camera.y, point.z - camera.z);
        };
        // Blend viewing directions so the distant aircraft cannot pull the lens up too quickly.
        const float soldierWeight = (1.0f - ease) / distance(soldierLook);
        const float aircraftWeight = ease / distance(aircraftLook);
        return {camera, mix(soldierLook, aircraftLook, aircraftWeight / (soldierWeight + aircraftWeight)), index};
    }
    if (time >= reflectionStart) {
        const Point ball = reflectionBall;
        const Point offset = mix({-0.75f, 0.26f, -1.33f}, {0.15f, 0.18f, -1.45f},
                                 segment(time, reflectionStart, marchStart));
        return {{ball.x + offset.x, ball.y + offset.y, ball.z + offset.z}, ball, index};
    }

    constexpr Point flagCamera{flagPoleX + 5.5f, 5.7f, flagPoleZ + 4.9f};
    constexpr Point flagLook{flagPoleX + 1.2f, 5.25f, flagPoleZ};
    constexpr Point armorCamera{5.2f, 4.15f, paradeDepth + 0.65f};
    constexpr Point armorVelocity{-0.5f, -0.25f, 0.24f};
    const auto offsetBy = [](Point point, Point velocity, float seconds) {
        return Point{point.x + velocity.x * seconds, point.y + velocity.y * seconds, point.z + velocity.z * seconds};
    };
    const auto armorLook = [](float t) { return Point{tankPosition(t, 0), 1.4f, paradeDepth}; };
    if (time >= armorDollyStart) {
        return {offsetBy(armorCamera, armorVelocity, time - armorDollyStart), armorLook(time), index};
    }
    constexpr Point flagVelocity{-0.65f, -0.35f, -7.5f};
    if (time < approachEnd) {
        constexpr Point openingCamera{11, 6.4f, 79};
        constexpr Point openingVelocity{-0.5f, -0.05f, -6};
        const float parameter = segment(time, 0, approachEnd);
        const float openingBias = 1.0f - parameter * parameter * (3.0f - 2.0f * parameter);
        return {curve(openingCamera, offsetBy(openingCamera, openingVelocity, approachEnd / 3),
                      offsetBy(flagCamera, flagVelocity, -approachEnd / 3), flagCamera, parameter),
                {flagLook.x - 13.5f * openingBias, flagLook.y + (flagHeight(time) + 0.8f - flagLook.y) * openingBias,
                 flagPoleZ + 4.2f * openingBias}, index};
    }
    const float parameter = segment(time, approachEnd, armorDollyStart);
    const float handleTime = (armorDollyStart - approachEnd) / 3;
    return {curve(flagCamera, offsetBy(flagCamera, flagVelocity, handleTime),
                  offsetBy(armorCamera, armorVelocity, -handleTime), armorCamera, parameter),
            curve(flagLook, flagLook, armorLook(armorDollyStart - handleTime), armorLook(armorDollyStart), parameter), index};
}
}
