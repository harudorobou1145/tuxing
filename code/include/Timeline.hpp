#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace animation {
constexpr float duration = 37.0f;
constexpr float fadeStart = duration - 3.0f;
constexpr float flightEnd = 35.0f;
inline float finaleFade(float seconds) {
    const float parameter = std::clamp((seconds - fadeStart) / (duration - fadeStart), 0.0f, 1.0f);
    return parameter * parameter * (3.0f - 2.0f * parameter);
}
constexpr float aircraftStart = 18.0f;
constexpr float aircraftPass = 26.0f;
constexpr float aircraftTurn = 31.0f;
constexpr float balloonRelease = 29.0f;
constexpr float balloonDepthMin = 280.0f;
constexpr float balloonDepthRange = 84.0f;
constexpr float balloonRiseScale = 0.12f;
constexpr float reflectionStart = 14.5f;
constexpr float marchStart = 17.5f;
constexpr float marchLiftStart = 18.7f;
constexpr float marchPanMid = 20.0f;
constexpr float aircraftTrack = 22.0f;
constexpr float cameraJoin = 26.0f;
inline constexpr std::array<float, 7> shotTimes{0, 5.3f, reflectionStart, marchStart, aircraftStart, aircraftTurn, duration};
constexpr float tankSpeed = 7.5f;
constexpr float soldierSpeed = 3.9f;
constexpr float paradeDepth = 72.0f;
constexpr float paradeRoadFirst = -522.0f;
constexpr float paradeRoadLast = 300.0f;
constexpr float paradeRoadWidth = 25.0f;
constexpr float roadCurbOffset = paradeRoadWidth * 0.5f + 0.15f;
constexpr float roadPostOffset = roadCurbOffset + 1.2f;
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
    if (seconds > flightEnd) {
        // Continue along the final curve's tangent, easing the climb into level flight.
        const float age = seconds - flightEnd;
        return {499.2f + 83.484f * age,
                258.0f + 26.46f * (1.0f - std::exp(-age / 1.5f)),
                -288.0f - 49.32f * age};
    }
    if (seconds < aircraftPass) {
        // Begin above and behind the infantry instead of far off-screen to the left.
        return curve({-260, 115, 45}, {-235, 135, 40}, {-195, 174, -114}, {-144, 174, -108},
                     segment(seconds, aircraftStart, aircraftPass));
    }
    if (seconds <= aircraftTurn) {
        // World-space handles match the incoming velocity and the departure's acceleration.
        return curve({-144, 174, -108}, {-112.125f, 174, -104.25f}, {153.6f, 180, -108}, {278.4f, 198, -144},
                     segment(seconds, aircraftPass, aircraftTurn));
    }
    return curve({278.4f, 198, -144}, {378.24f, 212.4f, -172.8f}, {387.888f, 234.48f, -222.24f}, {499.2f, 258, -288},
                 segment(seconds, aircraftTurn, flightEnd));
}

inline float balloonLaunchHeight(float depth, float radius) {
    constexpr Point eye{-180.0f - 3.9f * (balloonRelease - aircraftStart), 30.6f, 124};
    const Point look = aircraft(balloonRelease);
    const float pitch = std::atan2(look.y - eye.y, std::hypot(look.x - eye.x, look.z - eye.z));
    const float bottom = eye.y + std::tan(pitch - 22.0f * 3.14159265f / 180.0f) * depth;
    return std::min(3.9f, bottom - 2.2f * radius);
}

inline Shot shot(float seconds) {
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(
        std::upper_bound(shotTimes.begin(), shotTimes.end(), time) - shotTimes.begin()) - 1, 5);
    const auto interpolate = [&](const auto& times, const auto& points, const auto& velocities) {
        const auto key = static_cast<std::size_t>(
            std::upper_bound(times.begin(), times.end(), time) - times.begin() - 1);
        const float parameter = segment(time, times[key], times[key + 1]);
        const float handleTime = (times[key + 1] - times[key]) / 3.0f;
        const auto a = points[key], b = points[key + 1];
        const auto va = velocities[key], vb = velocities[key + 1];
        return curve(a, {a.x + va.x * handleTime, a.y + va.y * handleTime, a.z + va.z * handleTime},
                     {b.x - vb.x * handleTime, b.y - vb.y * handleTime, b.z - vb.z * handleTime}, b, parameter);
    };
    constexpr Point flypastCamera{-180, 30.6f, 124};
    if (time >= cameraJoin) {
        Point camera = flypastCamera;
        camera.x -= 3.9f * (time - aircraftStart);
        Point target = aircraft(time);
        if (time >= aircraftTurn) {
            // Ease into a rising dolly and pass the gaze from the aircraft to the balloon spread.
            const float age = time - aircraftTurn;
            camera.y += 0.85f * (age - 1.5f * (1.0f - std::exp(-age / 1.5f)));
            const Point releaseEye{flypastCamera.x - 3.9f * (balloonRelease - aircraftStart),
                                   flypastCamera.y, flypastCamera.z};
            const Point releaseLook = aircraft(balloonRelease);
            const float horizontalLength = std::hypot(releaseLook.x - releaseEye.x, releaseLook.z - releaseEye.z);
            constexpr float meanDepth = balloonDepthMin + balloonDepthRange * 0.5f;
            const Point balloons{releaseEye.x + (releaseLook.x - releaseEye.x) / horizontalLength * meanDepth,
                balloonLaunchHeight(meanDepth, meanDepth * 0.0115f)
                    + meanDepth * 0.725f * (time - balloonRelease) * balloonRiseScale,
                releaseEye.z + (releaseLook.z - releaseEye.z) / horizontalLength * meanDepth};
            const float parameter = segment(time, aircraftTurn, aircraftTurn + 5.0f);
            const float ease = parameter * parameter * (3.0f - 2.0f * parameter);
            const auto distance = [&](Point point) {
                return std::hypot(point.x - camera.x, point.y - camera.y, point.z - camera.z);
            };
            const float planeWeight = (1.0f - ease) / distance(target);
            const float balloonWeight = ease / distance(balloons);
            target = mix(target, balloons, balloonWeight / (planeWeight + balloonWeight));
        }
        return {camera, target, index};
    }
    if (time >= marchStart) {
        // Three-quarter view from the monument side, showing the formation's front and depth.
        constexpr float infantrySide = paradeDepth + 10.0f;
        const float frontX = soldierPosition(marchStart) + 24.0f;
        const std::array<float, 6> times{{marchStart, marchLiftStart, marchPanMid, aircraftTrack, 24, cameraJoin}};
        const std::array<Point, 6> cameras{{{frontX, 10.5f, infantrySide + 18.0f},
            {frontX + 4.0f, 10.6f, infantrySide + 18.5f}, {-103.8f, 13.6f, 96}, {-120, 19, 95},
            {-170, 24, 120}, {flypastCamera.x - 3.9f * (cameraJoin - aircraftStart), flypastCamera.y, flypastCamera.z}}};
        const std::array<Point, 6> velocities{{{3.9f, 0, 0}, {2, 0.2f, -0.8f}, {-6, 2, -2},
            {-10, 2, 0}, {-20, 3.6f, 2}, {-3.9f, 0, 0}}};
        const Point camera = interpolate(times, cameras, velocities);
        if (time >= aircraftTrack) return {camera, aircraft(time), index};
        const Point groundLook{camera.x - 42.0f, 1.1f, camera.z - 28.0f};
        const Point aircraftLook = aircraft(time);
        const float progress = segment(time, marchLiftStart, aircraftTrack);
        const float ease = progress * progress * (3.0f - 2.0f * progress);
        const auto distance = [&](Point point) {
            return std::hypot(point.x - camera.x, point.y - camera.y, point.z - camera.z);
        };
        // Blend directions, preserving a gentle pan despite the aircraft's much greater distance.
        const float groundWeight = (1.0f - ease) / distance(groundLook);
        const float aircraftWeight = ease / distance(aircraftLook);
        return {camera, mix(groundLook, aircraftLook, aircraftWeight / (groundWeight + aircraftWeight)), index};
    }
    if (time >= reflectionStart) {
        const Point offset = mix({-0.6f, 0.07f, -0.90f}, {0.45f, 0.03f, -0.94f},
                                 segment(time, reflectionStart, marchStart));
        const Point ball = reflectionBall;
        return {{ball.x + offset.x, ball.y + offset.y, ball.z + offset.z}, ball, index};
    }
    constexpr float roadTurn = 6.9f;
    constexpr float armorFront = 7.6f;
    const auto armorLook = [&](float t) { return Point{-37.5f - 1.5f * (t - armorFront), 1.4f, paradeDepth + 9}; };
    const std::array<float, 5> times{{0, shotTimes[1], roadTurn, armorFront, reflectionStart}};
    const std::array<Point, 5> cameras{{{66, 38, 474},
        {flagPoleX + 33, 34.2f, flagPoleZ + 29.4f}, {3, 16, 108},
        {2.5f, 7.2f, paradeDepth + 12.7f}, {-9.7f, 2.5f, paradeDepth + 12.8f}}};
    const std::array<Point, 5> velocities{{{-3, -0.3f, -36}, {-3.9f, -2.1f, -25},
        {-4, -10, -35}, {-3, -1, 0}, {-1.5f, -0.4f, 0}}};
    const Point flagLook{flagPoleX + 7.2f, 31.5f, flagPoleZ};
    const std::array<Point, 5> targets{{flagLook, flagLook, armorLook(roadTurn),
        armorLook(armorFront), armorLook(reflectionStart)}};
    const std::array<Point, 5> targetVelocities{{{0, 0, 0}, {0, 0, 0},
        {-1.5f, 0, 0}, {-1.5f, 0, 0}, {-1.5f, 0, 0}}};
    Point target = interpolate(times, targets, targetVelocities);
    if (index == 0) {
        const float parameter = segment(time, 0, shotTimes[1]);
        const float openingBias = 1.0f - parameter * parameter * (3.0f - 2.0f * parameter);
        target = {flagLook.x - 81.0f * openingBias,
                  flagLook.y + (flagHeight(time) + 4.8f - flagLook.y) * openingBias,
                  flagPoleZ + 25.2f * openingBias};
    }
    return {interpolate(times, cameras, velocities), target, index};
}
}
