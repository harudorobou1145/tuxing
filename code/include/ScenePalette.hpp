#pragma once
#include <glm/glm.hpp>
#include <array>
#include <string>

namespace palette {
// RGB components range from 0 to 1. These are base colors before lighting.
inline const glm::vec3 wallRed{0.65f, 0.12f, 0.09f};
inline const glm::vec3 roofGold{0.85f, 0.59f, 0.16f};
inline const glm::vec3 stoneWhite{0.88f, 0.85f, 0.77f};
inline const glm::vec3 standRed{0.46f, 0.055f, 0.045f};

inline glm::vec3 tiananmenColor(const std::string& objectName) {
    // The source exporter gave the paired reviewing stands generic Cube names.
    // Match only these known parts; other Cube objects include ground/buildings.
    static const std::array<const char*, 12> standParts = {
        "Cube.11", "Cube.10", "Cube.9", "Cube.8", "Cube.7", "Cube.6",
        "Cube.5", "Cube.4", "Cube.3", "Cube.1_1", "Cube.2_1", "Cube_2"};
    for (const auto* part : standParts) {
        if (objectName == part) return standRed;
    }
    if (objectName.find("tiananmen_square_rouge,_brique") == 0) return wallRed;
    if (objectName.find("tiananmen_square_Jaune,_toit") == 0) return roofGold;
    if (objectName.find("tiananmen_square_blanc") == 0) return stoneWhite;
    return glm::vec3(1.0f);
}
}
