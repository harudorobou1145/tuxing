#include "Mesh.hpp"
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

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
}

Mesh::Mesh(const std::string& objPath) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    if (!reader.ParseFromFile(objPath, config)) {
        throw std::runtime_error("OBJ load failed: " + reader.Error());
    }
    const auto& attributes = reader.GetAttrib();
    bool firstPosition = true;
    for (const auto& shape : reader.GetShapes()) {
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
    const glm::vec3 extent = boundsMax - boundsMin;
    const glm::vec3 safeExtent{
        extent.x > 0.0f ? extent.x : 1.0f,
        extent.y > 0.0f ? extent.y : 1.0f,
        extent.z > 0.0f ? extent.z : 1.0f};

    std::vector<Vertex> vertices;
    for (const auto& shape : reader.GetShapes()) {
        for (std::size_t offset = 0; offset + 2 < shape.mesh.indices.size(); offset += 3) {
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
                if (index.texcoord_index >= 0) {
                    const auto uv = static_cast<std::size_t>(index.texcoord_index) * 2;
                    if (uv + 1 >= attributes.texcoords.size()) throw std::runtime_error("OBJ has invalid UV index");
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
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}
void Mesh::draw() const {
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, count);
}