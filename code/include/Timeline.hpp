#pragma once

#include <algorithm>
#include <array>

namespace animation {
constexpr float duration = 35.0f;
constexpr float aircraftStart = 18.0f;
constexpr float aircraftPass = 26.0f;
constexpr float aircraftTurn = 31.0f;
inline constexpr std::array<float, 7> shotTimes{0, 5.3f, 11.8f, 16, aircraftStart, aircraftTurn, duration};
constexpr float tankSpeed = 7.5f;
constexpr float soldierSpeed = 3.9f;
constexpr float paradeDepth = 72.0f;
constexpr float paradeRoadFirst = -522.0f;
constexpr float paradeRoadLast = 300.0f;
constexpr float roadPostOffset = 26.0f;
constexpr float roadPostCapTop = 0.76f;
constexpr float roadBallRadius = 0.205f;
// Source Cylinder's center/base, transformed with the full scene's gateModel.
constexpr float flagPoleX = -2.74f;
constexpr float flagPoleZ = 154.0f;
constexpr float flagBaseOffset = 0.44f;
inline float tankPosition(float seconds, int index) {
    return -72.0f + (seconds - 3.0f) * tankSpeed - index * 10.0f;
}
inline float soldierPosition(float seconds, int row = 0) {
    return tankPosition(0, 3) - 48.0f + seconds * soldierSpeed - row * 1.6f;
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
    return 8.1f + flagBaseOffset + 25.2f * eased;
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
        // Fly in from far off-screen, matching the pass segment's velocity.
        return curve({-1500, 180, -270}, {-900, 168, -210}, {-259.2f, 174, -127.2f}, {-144, 174, -108},
                     segment(seconds, aircraftStart, aircraftPass));
    }
    Point position;
    if (seconds <= aircraftTurn) {
        position = curve({-144, 174, -108}, {-72, 174, -96}, {42, 180, -108}, {120, 198, -144},
                     segment(seconds, aircraftPass, aircraftTurn));
    } else {
        position = curve({120, 198, -144}, {182.4f, 212.4f, -172.8f}, {221.76f, 234.48f, -216.96f}, {258, 258, -288},
                         segment(seconds, aircraftTurn, duration));
    }
    position.x = -144.0f + (position.x + 144.0f) * 1.6f;
    return position;
}

inline Shot shot(float seconds) {
    constexpr float armorFront = 7.6f;
    constexpr float armorPass = 9.3f;
    constexpr std::array<float, 7> cameraTimes{0, shotTimes[1], armorFront, armorPass,
        shotTimes[2], shotTimes[3], aircraftStart};
    // The camera stays south of the parade lane so the gate stays in frame and
    // the road is read as a horizontal band rather than a vanishing line.
    constexpr float roadSideDepth = 124.0f;
    const std::array<Point, 7> cameras{{{66, 38.0f, 474.0f},
        {flagPoleX + 33.0f, 34.2f, flagPoleZ + 29.4f}, {20.4f, 24.6f, 124.0f},
        {7.5f, 18.6f, 124.0f}, {-54, 12.0f, roadSideDepth},
        {-150, 18.0f, roadSideDepth}, {-180, 30.6f, roadSideDepth}}};
    const std::array<Point, 7> cameraVelocities{{{-3.0f, -0.3f, -36}, {-3.9f, -2.1f, -25.0f},
        {-6.6f, -3.3f, 0}, {-15.6f, -2.7f, 0}, {-24.0f, 1.2f, 0},
        {-22.2f, 0.5f, 0}, {-3.9f, 0, 0}}};
    const Point flagLook{flagPoleX + 7.2f, 31.5f, flagPoleZ};
    const std::array<Point, 7> targets{{flagLook, flagLook,
        {tankPosition(armorFront, 0), 8.4f, paradeDepth},
        {tankPosition(armorPass, 0), 8.4f, paradeDepth},
        {soldierPosition(shotTimes[2]) - 2, 6.6f, paradeDepth},
        {soldierPosition(shotTimes[3]) - 2, 6.6f, paradeDepth}, aircraft(aircraftStart)}};
    const std::array<Point, 7> targetVelocities{{{0, 0, 0}, {0, 0, 0},
        {tankSpeed, 0, 0}, {-18.0f, 0, 0}, {soldierSpeed, 0, 0}, {soldierSpeed, 0, 0}, {40.5f, 0, 9.0f}}};
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(
        std::upper_bound(shotTimes.begin(), shotTimes.end(), time) - shotTimes.begin()) - 1, 5);
    if (time >= aircraftStart) {
        Point camera = cameras.back();
        camera.x += cameraVelocities.back().x * (time - aircraftStart);
        // Follow the flypast, then ease toward the established finale framing.
        const float departure = segment(time, aircraftTurn, duration);
        const float settle = departure * departure * (3.0f - 2.0f * departure);
        const Point target = mix(aircraft(time), {720, 330, -3600}, settle);
        return {camera, target, index};
    }
    const auto key = static_cast<std::size_t>(
        std::upper_bound(cameraTimes.begin(), cameraTimes.end(), time) - cameraTimes.begin() - 1);
    const float parameter = segment(time, cameraTimes[key], cameraTimes[key + 1]);
    const float handleTime = (cameraTimes[key + 1] - cameraTimes[key]) / 3.0f;
    const auto interpolate = [&](const auto& points, const auto& velocities) {
        const auto a = points[key];
        const auto b = points[key + 1];
        const auto va = velocities[key];
        const auto vb = velocities[key + 1];
        return curve(a, {a.x + va.x * handleTime, a.y + va.y * handleTime, a.z + va.z * handleTime},
                     {b.x - vb.x * handleTime, b.y - vb.y * handleTime, b.z - vb.z * handleTime}, b, parameter);
    };
    Point target = interpolate(targets, targetVelocities);
    if (index == 0) {
        // The opening looks left enough to include the nearby monument and the flag.
        const float openingBias = 1.0f - parameter * parameter * (3.0f - 2.0f * parameter);
        target = {flagLook.x - 81.0f * openingBias,
                  flagLook.y + (flagHeight(time) + 4.8f - flagLook.y) * openingBias,
                  flagPoleZ + 25.2f * openingBias};
    }
    return {interpolate(cameras, cameraVelocities), target, index};
}
}
