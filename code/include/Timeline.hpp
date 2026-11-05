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

inline Shot shot(float seconds) {
    constexpr std::array<float, 9> cameraTimes{0, 5, 12, 21, 24.8f, 26, 28, 31, duration};
    std::array<Point, 9> cameras{{{-55, 6, 105}, {-28, 4, 40},
        {soldierPosition(shotTimes[2]) + 6, 2.8f, paradeDepth + 9}, {soldierPosition(shotTimes[3]) + 6, 3.0f, paradeDepth + 10},
        {3.25f, 5.3f, 9.6f}, {3.7f, 6.7f, 8.8f}, {4.4f, 10, 18.5f}, {2, 12, 55}, {8, 14, 75}}};
    std::array<Point, 9> targets{{{0, 9, -3}, {-4, 6, -3},
        {soldierPosition(shotTimes[2]) - 2, 1.1f, paradeDepth}, {soldierPosition(shotTimes[3]) - 2, 1.1f, paradeDepth},
        {3.6f, 6.0f, 8}, {3.9f, 7.0f, 7.8f}, {-8, 30, -17}, {20, 33, -24}, {43, 43, -48}}};
    const auto moveWithFlag = [](Point& point) {
        point.x += flagPoleX - 2.5f;
        point.y += flagBaseOffset;
        point.z += flagPoleZ - 8.0f;
    };
    for (int key : {4, 5, 6}) moveWithFlag(cameras[key]);
    for (int key : {4, 5}) moveWithFlag(targets[key]);
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(
        std::upper_bound(shotTimes.begin(), shotTimes.end(), time) - shotTimes.begin()) - 1, 5);
    const int cameraIndex = std::min(static_cast<int>(
        std::upper_bound(cameraTimes.begin(), cameraTimes.end(), time) - cameraTimes.begin()) - 1, 7);
    const float parameter = segment(time, cameraTimes[cameraIndex], cameraTimes[cameraIndex + 1]);
    const auto interpolate = [&](const auto& points, bool cameraPath) {
        const auto velocity = [&](int key) {
            if (key == 0 || key == 8) {
                const int first = key == 0 ? 0 : 7;
                const float interval = cameraTimes[first + 1] - cameraTimes[first];
                return Point{(points[first + 1].x - points[first].x) / interval,
                             (points[first + 1].y - points[first].y) / interval,
                             (points[first + 1].z - points[first].z) / interval};
            }
            if (key == 2 || key == 3) return Point{soldierSpeed, 0, 0};
            if (!cameraPath && (key == 4 || key == 5)) return Point{0.15f, 1.2f, -0.2f};
            const float interval = cameraTimes[key + 1] - cameraTimes[key - 1];
            return Point{(points[key + 1].x - points[key - 1].x) / interval,
                         (points[key + 1].y - points[key - 1].y) / interval,
                         (points[key + 1].z - points[key - 1].z) / interval};
        };
        const float interval = (cameraTimes[cameraIndex + 1] - cameraTimes[cameraIndex]) / 3.0f;
        const auto startVelocity = velocity(cameraIndex);
        const auto endVelocity = velocity(cameraIndex + 1);
        const auto start = points[cameraIndex];
        const auto end = points[cameraIndex + 1];
        return curve(start, {start.x + startVelocity.x * interval, start.y + startVelocity.y * interval,
                            start.z + startVelocity.z * interval},
                     {end.x - endVelocity.x * interval, end.y - endVelocity.y * interval,
                      end.z - endVelocity.z * interval}, end, parameter);
    };
    return {interpolate(cameras, true), interpolate(targets, false), index};
}

inline Point aircraft(float seconds) {
    if (seconds <= aircraftTurn) {
        return curve({-24, 29, -18}, {-12, 29, -16}, {7, 30, -18}, {20, 33, -24},
                     segment(seconds, aircraftStart, aircraftTurn));
    }
    return curve({20, 33, -24}, {30.4f, 35.4f, -28.8f}, {36.96f, 39.08f, -36.16f}, {43, 43, -48},
                 segment(seconds, aircraftTurn, duration));
}
}
