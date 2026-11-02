#pragma once
#include <glm/glm.hpp>
#include <string>

namespace palette {
// RGB components range from 0 to 1. These are base colors before lighting.
inline const glm::vec3 wallRed{0.65f, 0.12f, 0.09f};
inline const glm::vec3 roofGold{0.85f, 0.59f, 0.16f};
inline const glm::vec3 stoneWhite{0.88f, 0.85f, 0.77f};

inline glm::vec3 tiananmenColor(const std::string& objectName) {
    if (objectName.find("tiananmen_square_rouge,_brique") == 0) return wallRed;
    if (objectName.find("tiananmen_square_Jaune,_toit") == 0) return roofGold;
    if (objectName.find("tiananmen_square_blanc") == 0) return stoneWhite;
    return glm::vec3(1.0f);
}
}
