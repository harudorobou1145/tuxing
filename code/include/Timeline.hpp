#pragma once

#include <algorithm>

namespace animation {
constexpr float duration = 25.0f;

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
    const Point cameras[] = {{-9, 7, 27}, {0, 3.8f, 16}, {6, 2.8f, 12},
                             {6, 3.2f, 13}, {4.8f, 6.8f, 10.5f}, {2, 5, 16}};
    const Point targets[] = {{0, 2, 0}, {0, 1.8f, 0}, {0, 1, 6},
                             {0, 1, 8.5f}, {3.6f, 6.6f, 8}, {0, 13, 0}};
    const float time = std::clamp(seconds, 0.0f, duration);
    const int index = std::min(static_cast<int>(time / 5.0f), 4);
    float parameter = segment(time, index * 5.0f, (index + 1) * 5.0f);
    parameter = parameter * parameter * (3.0f - 2.0f * parameter);
    return {mix(cameras[index], cameras[index + 1], parameter),
            mix(targets[index], targets[index + 1], parameter), index};
}

inline Point aircraft(float seconds) {
    return curve({-18, 13, -6}, {-8, 15, 0}, {8, 14, 5}, {20, 12, -2},
                 segment(seconds, 20.0f, 25.0f));
}
}