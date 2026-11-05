#pragma once
#include <glm/glm.hpp>
#include <array>
#include <string>

namespace palette {
// RGB components range from 0 to 1. These are base colors before lighting.
inline const glm::vec3 wallRed{0.67f, 0.16f, 0.13f};
inline const glm::vec3 roofGold{0.88f, 0.65f, 0.25f};
inline const glm::vec3 stoneWhite{0.86f, 0.87f, 0.83f};
inline const glm::vec3 standRed{0.52f, 0.105f, 0.085f};

inline glm::vec3 tiananmenColor(const std::string& objectName) {
    if (objectName == "heroes_monument") return stoneWhite;
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
    static const std::array<glm::vec3, 4> buildingColors = {
        glm::vec3(0.56f, 0.49f, 0.46f), glm::vec3(0.69f, 0.68f, 0.61f),
        glm::vec3(0.43f, 0.53f, 0.49f), glm::vec3(0.49f, 0.57f, 0.64f)};
    unsigned int colorIndex = 0;
    for (unsigned char character : objectName) {
        colorIndex = colorIndex * 31u + character;
    }
    return buildingColors[colorIndex % buildingColors.size()];
}
}
