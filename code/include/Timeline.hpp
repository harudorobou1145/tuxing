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
constexpr float aircraftStart = 16.0f;
constexpr float aircraftTurn = 31.0f;
constexpr float balloonRelease = 29.0f;
constexpr float balloonDepthMin = 280.0f;
constexpr float balloonDepthRange = 84.0f;
constexpr float balloonRiseScale = 0.12f;
constexpr float reflectionStart = 14.5f;
constexpr float marchStart = 17.5f;
constexpr float flypastStart = 18.0f;
constexpr float marchLiftStart = 18.7f;
constexpr float marchPanMid = 20.0f;
constexpr float aircraftTrack = 22.0f;
constexpr float cameraJoin = 26.0f;
constexpr float armorCentered = 9.8f;
inline constexpr std::array<float, 7> shotTimes{0, 5.3f, reflectionStart, marchStart, flypastStart, aircraftTurn, duration};
constexpr float tankSpeed = 7.5f;
constexpr float soldierSpeed = 3.9f;
constexpr float paradeDepth = 72.0f;
constexpr float paradeRoadFirst = -800.0f;
constexpr float paradeRoadLast = 600.0f;
constexpr float paradeRoadWidth = 25.0f;
constexpr float roadCurbOffset = paradeRoadWidth * 0.5f + 0.15f;
constexpr float roadPostOffset = roadCurbOffset + 1.2f;
constexpr float roadPostCapTop = 0.76f;
constexpr float roadBallRadius = 0.205f;
// Center of the opening between the four raised stone blocks in the plaza mesh.
constexpr float flagPoleX = -2.56f;
constexpr float flagPoleZ = 168.47f;
constexpr float flagBaseOffset = 0.68f;
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
inline constexpr Point flypastCamera{-89.0f, 30.6f, 124.0f};

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

inline float smootherStep(float t) {
    return t * t * t * (10.0f + t * (-15.0f + 6.0f * t));
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
    const float time = std::max(seconds - aircraftStart, 0.0f) / (aircraftTurn - aircraftStart);
    // Accelerate along one heading through the pass and departure.
    const float distance = time * (0.20f + 0.80f * time);
    return mix({-260, 115, 45}, {278.4f, 198, -144}, distance);
}

inline float balloonLaunchHeight(float depth, float radius) {
    constexpr Point eye{flypastCamera.x - 3.9f * (balloonRelease - flypastStart),
                        flypastCamera.y, flypastCamera.z};
    const Point look = aircraft(balloonRelease);
    const float pitch = std::atan2(look.y - eye.y, std::hypot(look.x - eye.x, look.z - eye.z));
    const float bottom = eye.y + std::tan(pitch - 22.0f * 3.14159265f / 180.0f) * depth;
    return std::min(3.9f, bottom - 2.2f * radius);
}

inline Shot shot(float seconds) {
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(
        std::upper_bound(shotTimes.begin(), shotTimes.end(), time) - shotTimes.begin()) - 1, 5);
    const auto interpolate = [](float at, const auto& times, const auto& points, const auto& velocities) {
        const auto key = static_cast<std::size_t>(
            std::upper_bound(times.begin(), times.end(), at) - times.begin() - 1);
        const float t = segment(at, times[key], times[key + 1]);
        const float span = times[key + 1] - times[key];
        const auto a = points[key], b = points[key + 1];
        const auto va = velocities[key], vb = velocities[key + 1];
        // Quintic Hermite interpolation shares velocity and zero acceleration at each key.
        const float blend = smootherStep(t);
        const float startWeight = span * t * (1.0f - t) * (1.0f - t) * (1.0f - t) * (1.0f + 3.0f * t);
        const float endWeight = span * t * t * t * (1.0f - t) * (3.0f * t - 4.0f);
        Point point = mix(a, b, blend);
        return Point{point.x + va.x * startWeight + vb.x * endWeight,
                     point.y + va.y * startWeight + vb.y * endWeight,
                     point.z + va.z * startWeight + vb.z * endWeight};
    };
    if (time >= cameraJoin) {
        Point camera = flypastCamera;
        camera.x -= 3.9f * (time - flypastStart);
        Point target = aircraft(time);
        if (time >= aircraftTurn) {
            // Ease into a rising dolly and pass the gaze from the aircraft to the balloon spread.
            const float age = time - aircraftTurn;
            const float rise = 1.0f - std::exp(-age / 1.5f);
            camera.y += 0.85f * (age - 1.5f * rise * (1.0f + 0.5f * rise));
            const Point releaseEye{flypastCamera.x - 3.9f * (balloonRelease - flypastStart),
                                   flypastCamera.y, flypastCamera.z};
            const Point releaseLook = aircraft(balloonRelease);
            const float horizontalLength = std::hypot(releaseLook.x - releaseEye.x, releaseLook.z - releaseEye.z);
            constexpr float meanDepth = balloonDepthMin + balloonDepthRange * 0.5f;
            const Point balloons{releaseEye.x + (releaseLook.x - releaseEye.x) / horizontalLength * meanDepth,
                balloonLaunchHeight(meanDepth, meanDepth * 0.0115f)
                    + meanDepth * 0.725f * (time - balloonRelease) * balloonRiseScale,
                releaseEye.z + (releaseLook.z - releaseEye.z) / horizontalLength * meanDepth};
            const float ease = smootherStep(segment(time, aircraftTurn, aircraftTurn + 5.0f));
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
        const std::array<float, 3> times{{marchStart, 20.4f, cameraJoin}};
        const std::array<Point, 3> cameras{{{frontX, 10.5f, infantrySide + 18.0f},
            {-82, 14.2f, 103.5f},
            {flypastCamera.x - 3.9f * (cameraJoin - flypastStart), flypastCamera.y, flypastCamera.z}}};
        const std::array<Point, 3> velocities{{{3.9f, 0, 0.35f}, {-5, 2.3f, 2.1f}, {-3.9f, 0, 0}}};
        const Point camera = interpolate(time, times, cameras, velocities);
        if (time >= aircraftTrack) return {camera, aircraft(time), index};
        const Point groundLook{camera.x - 42.0f, 1.1f, camera.z - 28.0f};
        const Point aircraftLook = aircraft(time);
        const float pan = segment(time, marchLiftStart, aircraftTrack);
        const float shoulder = pan * (1.0f - pan);
        // Spread the faster pan across the same window while easing in and out at both ends.
        const float ease = smootherStep(pan) + 16.0f * shoulder * shoulder * shoulder * (0.5f - pan);
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
    constexpr Point roadEntry{0, 5.8f, paradeDepth};
    constexpr Point roadEntryVelocity{-4.0f, -0.55f, 0};
    constexpr Point roadExitVelocity{-0.75f, -0.10f, 0};
    const std::array<float, 3> times{{0, shotTimes[1], armorCentered}};
    const std::array<Point, 3> cameras{{{66, 38, 474},
        {flagPoleX + 33, 34.2f, flagPoleZ + 29.4f}, roadEntry}};
    const std::array<Point, 3> velocities{{{-4, -0.3f, -45}, {-6, -2, -47}, roadEntryVelocity}};
    Point camera;
    if (time < armorCentered) {
        camera = interpolate(time, times, cameras, velocities);
    } else {
        // Integrate a smooth reduction in dolly speed once aligned with the road.
        const float span = reflectionStart - armorCentered;
        const float t = segment(time, armorCentered, reflectionStart);
        const float travel = span * t * t * t * (1.0f - 0.5f * t);
        const float age = time - armorCentered;
        camera = {roadEntry.x + roadEntryVelocity.x * age + (roadExitVelocity.x - roadEntryVelocity.x) * travel,
                  roadEntry.y + roadEntryVelocity.y * age + (roadExitVelocity.y - roadEntryVelocity.y) * travel,
                  paradeDepth};
    }
    const Point roadLook{camera.x - 40.0f, 1.4f, camera.z};
    constexpr float roadAimEnd = armorCentered - 0.5f;
    if (time >= roadAimEnd) return {camera, roadLook, index};

    // Let go of the flag gradually before passing it, then pan toward the oncoming column.
    constexpr float flagAimBrake = 4.5f;
    constexpr float flagAimEase = 1.5f;
    const float u = segment(time, flagAimBrake, flagAimBrake + flagAimEase);
    const float aimTime = std::min(time, flagAimBrake) + flagAimEase * (u - u * u * u * (1.0f - 0.5f * u));
    const Point aimEye = interpolate(aimTime, times, cameras, velocities);
    const float openingBias = 1.0f - smootherStep(segment(aimTime, 0, shotTimes[1]));
    const Point flagDirection{flagPoleX + 7.2f - 81.0f * openingBias - aimEye.x,
        31.5f + (flagHeight(aimTime) + 4.8f - 31.5f) * openingBias - aimEye.y,
        flagPoleZ + 25.2f * openingBias - aimEye.z};
    const float flagDistance = std::hypot(flagDirection.x, flagDirection.y, flagDirection.z);
    const float roadDistance = std::hypot(40.0f, roadLook.y - camera.y);
    const float blend = smootherStep(segment(time, 4.8f, roadAimEnd));
    const float scale = roadDistance / flagDistance;
    const Point target = mix({camera.x + flagDirection.x * scale,
                              camera.y + flagDirection.y * scale,
                              camera.z + flagDirection.z * scale}, roadLook, blend);
    return {camera, target, index};
}
}
