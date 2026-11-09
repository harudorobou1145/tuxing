#include "Mesh.hpp"
#include "Shader.hpp"
#include "ScenePalette.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Architectural decals use their own UVs and clamp at the plaque edges.
GLuint loadDecal(const std::filesystem::path& path, bool repeat = false) {
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        std::cerr << "Texture unavailable: " << path << '\n';
        return 0;
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);
    return texture;
}

void upload(const std::vector<Vertex>& vertices, GLuint& vertexArray, GLuint& vertexBuffer, GLsizei& count) {
    count = static_cast<GLsizei>(vertices.size());
    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

bool isFence(const std::string& name) {
    return name.find("Fence") != std::string::npos || name.find("fence") != std::string::npos;
}

// The balanced export sampled the tower family one object at a time, separating
// some rooftop pieces from their original towers. Reuse compatible pieces on
// the neighboring towers that survived the export, within the same city tile.
auto balancedRoofOffsets(const tinyobj::ObjReader& reader) {
    std::unordered_map<std::string, std::optional<glm::vec3>> offsets;
    constexpr std::string_view family = "orto-edificio22-completo.";
    std::unordered_map<std::string_view, const tinyobj::shape_t*> towers;
    for (const auto& shape : reader.GetShapes()) {
        if (shape.name.compare(0, family.size(), family) == 0) towers.emplace(shape.name, &shape);
    }
    const auto bounds = [&](const tinyobj::shape_t& shape) {
        const auto& vertices = reader.GetAttrib().vertices;
        glm::vec3 low(INFINITY), high(-INFINITY);
        for (const auto& index : shape.mesh.indices) {
            const auto i = static_cast<std::size_t>(index.vertex_index) * 3;
            const glm::vec3 point{vertices.at(i), vertices.at(i + 1), vertices.at(i + 2)};
            low = glm::min(low, point);
            high = glm::max(high, point);
        }
        return std::pair{low, high};
    };
    for (const auto& [name, roof] : towers) {
        const auto part = name.substr(family.size(), 3);
        if (part == "003" || part == "021" || part == "045") {
            offsets.emplace(name, std::nullopt);
            continue;
        }
        const std::string_view bodyPart = part == "027" ? "024" : part == "036" ? "033" : part == "051" ? "048" : "";
        if (bodyPart.empty()) continue;
        std::string bodyName(name);
        bodyName.replace(family.size(), 3, bodyPart);
        const auto body = towers.find(bodyName);
        if (body == towers.end()) {
            offsets.emplace(name, std::nullopt);
            continue;
        }
        const auto [roofMin, roofMax] = bounds(*roof);
        const auto [bodyMin, bodyMax] = bounds(*body->second);
        auto offset = ((bodyMin + bodyMax) - (roofMin + roofMax)) * 0.5f;
        // Embed the slab as in the matching tower template.
        offset.y = bodyMax.y - roofMin.y - (part == "027" ? 0.7f : 16.0f);
        offsets.emplace(name, offset);
    }
    return offsets;
}
}

Mesh::Mesh(const std::string& objPath, bool excludeSourceMonument, float roadsideWallOffset) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    const bool soldier = objPath.find("/solider/") != std::string::npos;
    const bool tiananmen = std::filesystem::u8path(objPath).parent_path().filename() == "tiananmen";
    const auto sourcePath = std::filesystem::u8path(objPath);
    std::ifstream objFile(sourcePath);
    if (!objFile) throw std::runtime_error("OBJ not found: " + objPath);
    auto materialPath = sourcePath;
    materialPath.replace_extension(".mtl");
    std::ifstream materialFile(materialPath);
    std::ostringstream objText, materialText;
    objText << objFile.rdbuf();
    materialText << materialFile.rdbuf();
    const bool loaded = reader.ParseFromString(objText.str(), materialText.str(), config);
    if (!loaded) {
        throw std::runtime_error("OBJ load failed: " + reader.Error());
    }
    const auto& attributes = reader.GetAttrib();
    if (!reader.Warning().empty()) std::cerr << reader.Warning() << '\n';
    const auto& materials = reader.GetMaterials();
    std::vector<GLuint> materialTextures(materials.size(), 0);
    for (std::size_t material = 0; material < materials.size(); ++material) {
        if (materials[material].diffuse_texname.empty()) continue;
        const auto directory = std::filesystem::u8path(objPath).parent_path();
        auto path = directory / std::filesystem::u8path(materials[material].diffuse_texname);
        if (!std::filesystem::exists(path)) {
            const auto filename = std::filesystem::u8path(materials[material].diffuse_texname).filename();
            for (const auto& candidate : {directory / filename, directory.parent_path() / "textures" / filename}) {
                if (std::filesystem::exists(candidate)) { path = candidate; break; }
            }
        }
        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        std::ifstream textureFile(path, std::ios::binary);
        std::vector<unsigned char> textureBytes;
        if (!textureFile) {
            std::cerr << "Texture unavailable: " << path << '\n';
            continue;
        }
        textureBytes.assign(std::istreambuf_iterator<char>(textureFile), std::istreambuf_iterator<char>());
        unsigned char* pixels = stbi_load_from_memory(textureBytes.data(), static_cast<int>(textureBytes.size()), &width, &height, &channels, 4);
        if (!pixels) {
            std::cerr << "Texture unavailable: " << path << '\n';
            continue;
        }
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(pixels);
        materialTextures[material] = texture;
        textures.push_back(texture);
    }
    const auto roofOffsets = sourcePath.filename() == "tiananmen_balanced.obj" && tiananmen
        ? balancedRoofOffsets(reader) : decltype(balancedRoofOffsets(reader)){};
    const auto skipShape = [&](const std::string& name) {
        if ((soldier && name == "Cube") || isFence(name)) return true;
        // The animated pole replaces these two exact source objects; keep its pedestal.
        if (tiananmen && (name == "Cylinder" || name == "Plane")) return true;
        // The exported roof slabs ("Toit_top") are wider than the building body
        // and sit high above it, so their overhang floats in the air.
        if (tiananmen && (name == "Toit_top" || name == "Toit_top_1")) return true;
        // The distant skyline's cornice slabs ("Cube.0XX") are exported without
        // their building bodies, so they float as detached roof ledges.
        if (tiananmen && name.rfind("Cube.0", 0) == 0) return true;
        if (excludeSourceMonument && (name == "Tour_centre_Square" || name == "Cube_3")) return true;
        const auto roof = roofOffsets.find(name);
        return roof != roofOffsets.end() && !roof->second;
    };
    const auto positionOffset = [&](const std::string& name) {
        // Move only the long roadside wall, leaving the gate and grandstands in place.
        if (tiananmen && name == "Cube.2") return glm::vec3(0, 0, roadsideWallOffset);
        const auto roof = roofOffsets.find(name);
        return roof == roofOffsets.end() ? glm::vec3(0) : *roof->second;
    };
    bool firstPosition = true;
    for (const auto& shape : reader.GetShapes()) {
        if (skipShape(shape.name)) continue;
        const glm::vec3 offset = positionOffset(shape.name);
        for (const auto& index : shape.mesh.indices) {
            if (index.vertex_index < 0 || static_cast<std::size_t>(3 * index.vertex_index + 2) >= attributes.vertices.size()) {
                throw std::runtime_error("OBJ has invalid position index");
            }
            const auto position = static_cast<std::size_t>(index.vertex_index) * 3;
            glm::vec3 point{attributes.vertices[position], attributes.vertices[position + 1], attributes.vertices[position + 2]};
            point += offset;
            if (firstPosition) {
                boundsMin = point;
                boundsMax = point;
                firstPosition = false;
            }
            boundsMin = glm::min(boundsMin, point);
            boundsMax = glm::max(boundsMax, point);
        }
    }
    if (firstPosition) throw std::runtime_error("OBJ contains no triangles: " + objPath);
    std::cout << "Bounds " << objPath << ": [" << boundsMin.x << ", " << boundsMin.y << ", " << boundsMin.z
              << "] - [" << boundsMax.x << ", " << boundsMax.y << ", " << boundsMax.z << "]" << std::endl;
    const glm::vec3 extent = boundsMax - boundsMin;
    const glm::vec3 safeExtent{
        extent.x > 0.0f ? extent.x : 1.0f,
        extent.y > 0.0f ? extent.y : 1.0f,
        extent.z > 0.0f ? extent.z : 1.0f};

    GLuint roofTexture = 0;
    if (tiananmen && std::any_of(reader.GetShapes().begin(), reader.GetShapes().end(),
        [](const auto& shape) { return shape.name.find("tiananmen_square_Jaune,_toit") == 0; })) {
        roofTexture = loadDecal(std::filesystem::u8path(objPath).parent_path()
            / "textures" / "roof_glazed_tiles.png", true);
        if (roofTexture) textures.push_back(roofTexture);
    }
    GLuint raisedPlazaTexture = 0;
    if (tiananmen && std::any_of(reader.GetShapes().begin(), reader.GetShapes().end(),
        [](const auto& shape) { return shape.name == "Floor/Plancher.1"; })) {
        raisedPlazaTexture = loadDecal(std::filesystem::u8path(objPath).parent_path() / "tile_2.jpeg", true);
        if (raisedPlazaTexture) textures.push_back(raisedPlazaTexture);
    }
    const bool hasPlazaPlanters = tiananmen && std::any_of(reader.GetShapes().begin(), reader.GetShapes().end(),
        [](const auto& shape) { return shape.name == "Cube"; });
    std::vector<Vertex> vertices;
    for (const auto& shape : reader.GetShapes()) {
        if (skipShape(shape.name)) continue;
        const glm::vec3 translation = positionOffset(shape.name);
        const bool track = shape.name.find("TrackMesh") != std::string::npos;
        const bool roof = roofTexture && shape.name.find("tiananmen_square_Jaune,_toit") == 0;
        const bool plazaGround = tiananmen && shape.name == "Floor/Plancher.1";
        const bool plazaTerrain = plazaGround && hasPlazaPlanters;
        const bool plazaPlanters = tiananmen && shape.name == "Cube";
        const bool groundObject = tiananmen && (shape.name == "Floor/Plancher.1"
            || shape.name == "Bottom" || shape.name == "Bottom_1"
            || shape.name == "Tour_centre_Square" || shape.name == "Cube");
        for (std::size_t offset = 0; offset + 2 < shape.mesh.indices.size(); offset += 3) {
            if (plazaGround) {
                glm::vec3 points[3];
                for (int corner = 0; corner < 3; ++corner) {
                    const auto index = 3 * shape.mesh.indices[offset + corner].vertex_index;
                    points[corner] = {attributes.vertices[index], attributes.vertices[index + 1], attributes.vertices[index + 2]};
                }
                if (std::max({points[0].x, points[1].x, points[2].x}) >= -13000
                    && std::min({points[0].x, points[1].x, points[2].x}) <= 9000
                    && std::max({points[0].z, points[1].z, points[2].z}) >= 7500
                    && std::min({points[0].z, points[1].z, points[2].z}) <= 33000)
                    plazaTriangles.insert(plazaTriangles.end(), points, points + 3);
            }
            bool ground = plazaGround;
            bool raisedPlaza = false;
            if (plazaGround && raisedPlazaTexture) {
                glm::vec3 center(0);
                float lowest = 1.0e30f;
                for (int corner = 0; corner < 3; ++corner) {
                    const auto index = 3 * shape.mesh.indices[offset + corner].vertex_index;
                    center += glm::vec3(attributes.vertices[index], attributes.vertices[index + 1], attributes.vertices[index + 2]);
                    lowest = std::min(lowest, attributes.vertices[index + 1]);
                }
                center /= 3.0f;
                // The raised plaza has noisy heights (~35..133), whereas the
                // surrounding pavement is near zero. Keep the cutoff below its
                // full top surface so it cannot split into alternating materials.
                raisedPlaza = lowest > 20.0f && center.x > -11600.0f && center.x < 7700.0f
                    && center.z > 11300.0f && center.z < 60000.0f;
            }
            if (groundObject && !plazaGround) {
                glm::vec3 points[3];
                for (int k = 0; k < 3; ++k) {
                    const int index = shape.mesh.indices[offset + k].vertex_index;
                    points[k] = {attributes.vertices[3 * index], attributes.vertices[3 * index + 1], attributes.vertices[3 * index + 2]};
                }
                const auto normal = glm::cross(points[1] - points[0], points[2] - points[0]);
                ground = std::abs(normal.y) > 0.99f * glm::length(normal)
                    && std::min({points[0].y, points[1].y, points[2].y}) > -80.0f
                    && std::max({points[0].y, points[1].y, points[2].y}) < 50.0f;
                // Both planter beds sit at source y=41.47, below the raised plaza.
                raisedPlaza = raisedPlazaTexture && plazaPlanters && ground
                    && std::all_of(std::begin(points), std::end(points), [](glm::vec3 p) {
                        return std::abs(p.y - 41.47f) < 0.1f;
                    });
            }
            const int material = offset / 3 < shape.mesh.material_ids.size() ? shape.mesh.material_ids[offset / 3] : -1;
            const bool validMaterial = material >= 0 && static_cast<std::size_t>(material) < materials.size();
            const GLuint texture = raisedPlaza ? raisedPlazaTexture : roof ? roofTexture : validMaterial ? materialTextures[material] : 0;
            const glm::vec3 color = roof || (soldier && texture) ? glm::vec3(1.0f) : validMaterial
                ? glm::vec3(materials[material].diffuse[0], materials[material].diffuse[1], materials[material].diffuse[2])
                : tiananmen ? palette::tiananmenColor(shape.name) : glm::vec3(1.0f);
            const int architecture = shape.name == "heroes_monument" ? 2
                : tiananmen && (shape.name == "Bottom" || shape.name == "Bottom_1"
                    || shape.name == "Toit_top" || shape.name == "Toit_top_1") ? 1 : 0;
            if (batches.empty() || batches.back().color != color || batches.back().texture != texture
                || batches.back().track != track || batches.back().ground != ground || batches.back().roof != roof
                || batches.back().architecture != architecture || batches.back().plazaTerrain != plazaTerrain) {
                batches.push_back({static_cast<GLint>(vertices.size()), 0, color, texture, track, ground, roof, architecture, plazaTerrain});
            }
            batches.back().count += 3;
            Vertex triangle[3]{};
            for (int corner = 0; corner < 3; ++corner) {
                const auto index = shape.mesh.indices[offset + corner];
                if (index.vertex_index < 0 || static_cast<std::size_t>(3 * index.vertex_index + 2) >= attributes.vertices.size()) {
                    throw std::runtime_error("OBJ has invalid position index");
                }
                const auto position = static_cast<std::size_t>(index.vertex_index) * 3;
                triangle[corner].position = {attributes.vertices[position], attributes.vertices[position + 1], attributes.vertices[position + 2]};
                triangle[corner].position += translation;
                if (index.normal_index >= 0) {
                    const auto normal = static_cast<std::size_t>(index.normal_index) * 3;
                    if (normal + 2 >= attributes.normals.size()) throw std::runtime_error("OBJ has invalid normal index");
                    triangle[corner].normal = {attributes.normals[normal], attributes.normals[normal + 1], attributes.normals[normal + 2]};
                }
                if (index.texcoord_index >= 0 && static_cast<std::size_t>(index.texcoord_index) * 2 + 1 < attributes.texcoords.size()) {
                    const auto uv = static_cast<std::size_t>(index.texcoord_index) * 2;
                    triangle[corner].uv = {attributes.texcoords[uv], attributes.texcoords[uv + 1]};
                } else {
                    // Planar fallback in model space, normalized so textures tile once
                    // instead of repeating thousands of times on large models.
                    triangle[corner].uv = {
                        (triangle[corner].position.x - boundsMin.x) / safeExtent.x,
                        (triangle[corner].position.y - boundsMin.y) / safeExtent.y};
                }
            }
            const glm::vec3 cross = glm::cross(triangle[1].position - triangle[0].position, triangle[2].position - triangle[0].position);
            const glm::vec3 faceNormal = glm::length(cross) > 0.00001f ? glm::normalize(cross) : glm::vec3(0, 1, 0);
            for (auto& vertex : triangle) {
                if (roof) {
                    // Align the tile channels down each slope, including the hip ends.
                    // Keep source-space coordinates; the shader controls repeat size.
                    vertex.uv = std::abs(faceNormal.x) > std::abs(faceNormal.z)
                        ? glm::vec2(vertex.position.z, -vertex.position.x)
                        : glm::vec2(vertex.position.x, -vertex.position.z);
                }
                if (glm::length(vertex.normal) < 0.00001f) vertex.normal = faceNormal;
                vertices.push_back(vertex);
            }
        }
    }
    if (vertices.empty()) throw std::runtime_error("OBJ contains no triangles: " + objPath);
    // These source-space anchors are shared by the gate/core/balanced exports.
    // Only add decals to this known gate, not to unrelated custom OBJ models.
    const bool hasPortrait = std::any_of(reader.GetShapes().begin(), reader.GetShapes().end(),
        [](const auto& shape) { return shape.name == "Mao_Zeong"; });
    if (tiananmen && hasPortrait) {
        const auto directory = std::filesystem::u8path(objPath).parent_path() / "textures";
        const auto plaque = [&](const char* filename, glm::vec3 bottomLeft, glm::vec3 bottomRight,
                                glm::vec3 topRight, glm::vec3 topLeft) {
            const GLuint texture = loadDecal(directory / filename);
            if (!texture) return;
            textures.push_back(texture);
            // Also clear the original lettering surfaces retained by the full scene.
            const glm::vec3 offset(0, 0, 12.0f);
            const glm::vec3 normal = glm::normalize(glm::cross(bottomRight - bottomLeft, topLeft - bottomLeft));
            batches.push_back({static_cast<GLint>(vertices.size()), 6, glm::vec3(1), texture, false});
            const Vertex bl{bottomLeft + offset, normal, {0, 0}};
            const Vertex br{bottomRight + offset, normal, {1, 0}};
            const Vertex tr{topRight + offset, normal, {1, 1}};
            const Vertex tl{topLeft + offset, normal, {0, 1}};
            vertices.insert(vertices.end(), {bl, br, tr, bl, tr, tl});
        };
        plaque("slogans_atlas_l.png", {-3771.65f, 512.66f, 1771.18f}, {-1916.72f, 512.66f, 1719.37f},
            {-1916.90f, 706.861f, 1713.08f}, {-3771.83f, 706.861f, 1764.89f});
        plaque("slogans_atlas_r.png", {-666.798f, 512.66f, 1684.46f}, {1188.13f, 512.66f, 1632.64f},
            {1187.96f, 706.861f, 1626.35f}, {-666.974f, 706.861f, 1678.16f});
        plaque("figure.jpg", {-1429.73f, 442.886f, 1703.46f}, {-1153.19f, 442.886f, 1696.17f},
            {-1153.19f, 812.197f, 1696.17f}, {-1429.73f, 812.197f, 1703.46f});
    }
    upload(vertices, vertexArray, vertexBuffer, count);
}

Mesh::Mesh(float halfExtent, int subdivisions) {
    const int steps = std::max(1, subdivisions);
    const float span = 2.0f * halfExtent;
    const float step = span / static_cast<float>(steps);
    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(steps) * static_cast<std::size_t>(steps) * 6);
    const float tiles = 16.0f;
    const auto add = [&](float x, float z) {
        vertices.push_back(Vertex{
            {x, 0.0f, z},
            {0.0f, 1.0f, 0.0f},
            {(x + halfExtent) / span * tiles, (z + halfExtent) / span * tiles}});
    };
    for (int row = 0; row < steps; ++row) {
        for (int column = 0; column < steps; ++column) {
            const float x0 = -halfExtent + column * step;
            const float x1 = x0 + step;
            const float z0 = -halfExtent + row * step;
            const float z1 = z0 + step;
            add(x0, z0);
            add(x0, z1);
            add(x1, z1);
            add(x0, z0);
            add(x1, z1);
            add(x1, z0);
        }
    }
    boundsMin = {-halfExtent, 0.0f, -halfExtent};
    boundsMax = {halfExtent, 0.0f, halfExtent};
    upload(vertices, vertexArray, vertexBuffer, count);
}
Mesh::Mesh(float radius, int slices, int stacks) {
    std::vector<Vertex> vertices;
    const auto vertex = [&](int slice, int stack) {
        const float longitude = 6.2831853f * slice / slices;
        const float latitude = 3.14159265f * stack / stacks;
        const glm::vec3 normal(std::sin(latitude) * std::cos(longitude), std::cos(latitude),
            std::sin(latitude) * std::sin(longitude));
        return Vertex{normal * radius, normal, {static_cast<float>(slice) / slices, static_cast<float>(stack) / stacks}};
    };
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            vertices.insert(vertices.end(), {vertex(slice, stack), vertex(slice, stack + 1), vertex(slice + 1, stack + 1),
                vertex(slice, stack), vertex(slice + 1, stack + 1), vertex(slice + 1, stack)});
        }
    }
    boundsMin = glm::vec3(-radius);
    boundsMax = glm::vec3(radius);
    upload(vertices, vertexArray, vertexBuffer, count);
}
Mesh::Mesh(float radius, float height, int slices) {
    std::vector<Vertex> vertices;
    const glm::vec3 up(0, 1, 0);
    for (int slice = 0; slice < slices; ++slice) {
        const float u = static_cast<float>(slice) / slices;
        const float nextU = static_cast<float>(slice + 1) / slices;
        const float angle = 6.2831853f * u;
        const float nextAngle = 6.2831853f * nextU;
        const glm::vec3 normal(std::cos(angle), 0, std::sin(angle));
        const glm::vec3 nextNormal(std::cos(nextAngle), 0, std::sin(nextAngle));
        const glm::vec3 bottom = normal * radius, nextBottom = nextNormal * radius;
        const glm::vec3 top = bottom + up * height, nextTop = nextBottom + up * height;
        const Vertex a{bottom, normal, {u, 0}}, b{top, normal, {u, 1}};
        const Vertex c{nextBottom, nextNormal, {nextU, 0}}, d{nextTop, nextNormal, {nextU, 1}};
        vertices.insert(vertices.end(), {a, b, c, c, b, d});
        const glm::vec2 uv = glm::vec2(normal.x, normal.z) * 0.5f + 0.5f;
        const glm::vec2 nextUV = glm::vec2(nextNormal.x, nextNormal.z) * 0.5f + 0.5f;
        vertices.insert(vertices.end(), {{{0, 0, 0}, -up, {0.5f, 0.5f}}, {bottom, -up, uv}, {nextBottom, -up, nextUV},
            {{0, height, 0}, up, {0.5f, 0.5f}}, {nextTop, up, nextUV}, {top, up, uv}});
    }
    boundsMin = {-radius, 0, -radius};
    boundsMax = {radius, height, radius};
    upload(vertices, vertexArray, vertexBuffer, count);
}
Mesh::~Mesh() {
    if (!textures.empty()) glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}
void Mesh::draw() const {
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

float Mesh::groundHeight(float x, float z, float fallback) const {
    float height = fallback;
    bool found = false;
    for (std::size_t index = 0; index + 2 < plazaTriangles.size(); index += 3) {
        const auto& first = plazaTriangles[index];
        const auto& second = plazaTriangles[index + 1];
        const auto& third = plazaTriangles[index + 2];
        const float determinant = (second.z - third.z) * (first.x - third.x)
            + (third.x - second.x) * (first.z - third.z);
        if (std::abs(determinant) < 0.0001f) continue;
        const float firstWeight = ((second.z - third.z) * (x - third.x)
            + (third.x - second.x) * (z - third.z)) / determinant;
        const float secondWeight = ((third.z - first.z) * (x - third.x)
            + (first.x - third.x) * (z - third.z)) / determinant;
        const float thirdWeight = 1.0f - firstWeight - secondWeight;
        if (firstWeight >= -0.00001f && secondWeight >= -0.00001f && thirdWeight >= -0.00001f) {
            const float candidate = firstWeight * first.y + secondWeight * second.y + thirdWeight * third.y;
            height = found ? std::max(height, candidate) : candidate;
            found = true;
        }
    }
    return height;
}

void Mesh::draw(const Shader& shader, GLuint fallback, const glm::vec3& tint, GLuint paving) const {
    shader.integer("trackSurface", 0);
    shader.integer("roofSurface", 0);
    shader.integer("pavingSurface", 0);
    shader.integer("architectureSurface", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vertexArray);
    if (batches.empty()) {
        shader.integer("pavingSurface", paving != 0 ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D, fallback);
        shader.vector("tint", tint);
        glDrawArrays(GL_TRIANGLES, 0, count);
    } else {
        for (const auto& batch : batches) {
            shader.integer("plazaTerrain", batch.plazaTerrain ? 1 : 0);
            shader.integer("architectureSurface", batch.architecture);
            shader.integer("roofSurface", batch.roof ? 1 : 0);
            const bool ground = batch.ground && paving != 0;
            shader.integer("pavingSurface", ground ? 1 : 0);
            shader.integer("trackSurface", batch.track ? 1 : 0);
            glBindTexture(GL_TEXTURE_2D, batch.texture ? batch.texture : ground ? paving : fallback);
            shader.vector("tint", ground ? glm::vec3(0.92f) : tint * batch.color);
            glDrawArrays(GL_TRIANGLES, batch.first, batch.count);
        }
    }
    shader.integer("trackSurface", 0);
    shader.integer("pavingSurface", 0);
    shader.integer("roofSurface", 0);
    shader.integer("architectureSurface", 0);
    shader.integer("plazaTerrain", 0);
}

Mesh::Mesh(float width, float height, int columns, int rows) {
    std::vector<Vertex> vertices;
    const auto add = [&](int column, int row) {
        const float horizontal = static_cast<float>(column) / columns;
        const float vertical = static_cast<float>(row) / rows;
        vertices.push_back({{width * horizontal, height * vertical, 0}, {0, 0, 1}, {horizontal, vertical}});
    };
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            add(column, row); add(column + 1, row); add(column + 1, row + 1);
            add(column, row); add(column + 1, row + 1); add(column, row + 1);
        }
    }
    boundsMin = {0, 0, 0};
    boundsMax = {width, height, 0};
    upload(vertices, vertexArray, vertexBuffer, count);
}
