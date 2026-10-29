#include "Mesh.hpp"
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};
}

Mesh::Mesh(const std::string& objPath) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    if (!reader.ParseFromFile(objPath, config)) {
        throw std::runtime_error("OBJ load failed: " + reader.Error());
    }
    const auto& attributes = reader.GetAttrib();
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
                    triangle[corner].uv = {triangle[corner].position.x, triangle[corner].position.y};
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
Mesh::~Mesh() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
}
void Mesh::draw() const {
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, count);
}