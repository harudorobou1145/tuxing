#include "Mesh.hpp"
#include "Shader.hpp"
#include "ScenePalette.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Architectural decals use their own UVs and clamp at the plaque edges.
GLuint loadDecal(const std::filesystem::path& path) {
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
}

Mesh::Mesh(const std::string& objPath) {
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
    bool firstPosition = true;
    for (const auto& shape : reader.GetShapes()) {
        if (soldier && shape.name == "Cube") continue;
        if (isFence(shape.name)) continue;
        for (const auto& index : shape.mesh.indices) {
            if (index.vertex_index < 0 || static_cast<std::size_t>(3 * index.vertex_index + 2) >= attributes.vertices.size()) {
                throw std::runtime_error("OBJ has invalid position index");
            }
            const auto position = static_cast<std::size_t>(index.vertex_index) * 3;
            const glm::vec3 point{attributes.vertices[position], attributes.vertices[position + 1], attributes.vertices[position + 2]};
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

    std::vector<Vertex> vertices;
    for (const auto& shape : reader.GetShapes()) {
        if (soldier && shape.name == "Cube") continue;
        if (isFence(shape.name)) continue;
        const bool track = shape.name.find("TrackMesh") != std::string::npos;
        for (std::size_t offset = 0; offset + 2 < shape.mesh.indices.size(); offset += 3) {
            const int material = offset / 3 < shape.mesh.material_ids.size() ? shape.mesh.material_ids[offset / 3] : -1;
            const bool validMaterial = material >= 0 && static_cast<std::size_t>(material) < materials.size();
            const GLuint texture = validMaterial ? materialTextures[material] : 0;
            const glm::vec3 color = soldier && texture ? glm::vec3(1.0f) : validMaterial
                ? glm::vec3(materials[material].diffuse[0], materials[material].diffuse[1], materials[material].diffuse[2])
                : tiananmen ? palette::tiananmenColor(shape.name) : glm::vec3(1.0f);
            if (batches.empty() || batches.back().color != color || batches.back().texture != texture || batches.back().track != track) {
                batches.push_back({static_cast<GLint>(vertices.size()), 0, color, texture, track});
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
Mesh::~Mesh() {
    if (!textures.empty()) glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}
void Mesh::draw() const {
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void Mesh::draw(const Shader& shader, GLuint fallback, const glm::vec3& tint) const {
    shader.integer("trackSurface", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vertexArray);
    if (batches.empty()) {
        glBindTexture(GL_TEXTURE_2D, fallback);
        shader.vector("tint", tint);
        glDrawArrays(GL_TRIANGLES, 0, count);
    } else {
        for (const auto& batch : batches) {
            shader.integer("trackSurface", batch.track ? 1 : 0);
            glBindTexture(GL_TEXTURE_2D, batch.texture ? batch.texture : fallback);
            shader.vector("tint", tint * batch.color);
            glDrawArrays(GL_TRIANGLES, batch.first, batch.count);
        }
    }
    shader.integer("trackSurface", 0);
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
