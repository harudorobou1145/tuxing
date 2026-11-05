#pragma once

#include <algorithm>
#include <array>

namespace animation {
constexpr float duration = 35.0f;
constexpr float aircraftStart = 26.0f;
constexpr float aircraftTurn = 31.0f;
inline constexpr std::array<float, 7> shotTimes{0, 5, 12, 21, aircraftStart, aircraftTurn, duration};
constexpr float tankSpeed = 1.25f;
constexpr float soldierSpeed = 0.65f;
constexpr float paradeDepth = 12.0f;
// Source Cylinder's center/base, transformed with the full scene's gateModel.
constexpr float flagPoleX = -0.4566794f;
constexpr float flagPoleZ = 25.6760101f;
constexpr float flagBaseOffset = 0.0731168f;
inline float tankPosition(float seconds, int index) {
    return 2.0f + (seconds - 3.0f) * tankSpeed - index * 7.0f;
}
inline float soldierPosition(float seconds, int row = 0) {
    return tankPosition(0, 3) - 8.0f + seconds * soldierSpeed - row * 1.6f;
}

struct Point {
    float x, y, z;
};

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
    Point position;
    if (seconds <= aircraftTurn) {
        position = curve({-24, 29, -18}, {-12, 29, -16}, {7, 30, -18}, {20, 33, -24},
                     segment(seconds, aircraftStart, aircraftTurn));
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
    constexpr std::array<float, 6> cameraTimes{0, 5, 8, 12, 21, aircraftStart};
    // Approach from the east of the monument, round the flagpole on the east,
    // then travel west along the plaza side of the parade road.
    constexpr float roadSideDepth = 18.2f;
    const std::array<Point, 6> cameras{{{11, 6.4f, 79},
        {flagPoleX + 5.1f, 5.8f, flagPoleZ + 2.8f}, {0, 5.4f, roadSideDepth},
        {-11, 4.9f, roadSideDepth}, {-27, 5.0f, roadSideDepth}, {-33, 5.4f, roadSideDepth}}};
    const std::array<Point, 6> cameraVelocities{{{-0.5f, -0.05f, -12}, {-1.0f, -0.1f, -1.2f},
        {-2.5f, -0.15f, 0}, {-2.2f, 0, 0}, {-1.4f, 0.08f, 0}, {-0.65f, 0, 0}}};
    const Point raisedFlagCenter{flagPoleX + 1.2f, flagHeight(5) + 0.8f, flagPoleZ};
    const std::array<Point, 6> targets{{raisedFlagCenter, raisedFlagCenter,
        {tankPosition(8, 1), 1.4f, paradeDepth},
        {soldierPosition(12) - 2, 1.1f, paradeDepth},
        {soldierPosition(21) - 2, 1.1f, paradeDepth}, aircraft(aircraftStart)}};
    const std::array<Point, 6> targetVelocities{{{0, 0, 0}, {0, 0, 0},
        {-2.5f, 0, 0}, {soldierSpeed, 0, 0}, {soldierSpeed, 0, 0}, {11.52f, 0, 1.2f}}};
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(
        std::upper_bound(shotTimes.begin(), shotTimes.end(), time) - shotTimes.begin()) - 1, 5);
    if (time >= aircraftStart) {
        Point camera = cameras.back();
        camera.x += cameraVelocities.back().x * (time - aircraftStart);
        // Follow the flypast, then ease toward the established finale framing.
        const float departure = segment(time, aircraftTurn, duration);
        const float settle = departure * departure * (3.0f - 2.0f * departure);
        const Point target = mix(aircraft(time), {120, 55, -600}, settle);
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
        target = {raisedFlagCenter.x - 13.5f * openingBias,
                  flagHeight(time) + 0.8f,
                  flagPoleZ + 4.2f * openingBias};
    }
    return {interpolate(cameras, cameraVelocities), target, index};
}
}
