#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Mesh {
public:
    explicit Mesh(const std::string& objPath);
    // Flat XZ plane at y = 0, centred on the origin, normal facing up.
    explicit Mesh(float halfExtent, int subdivisions = 1);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    void draw() const;
    const glm::vec3& minimum() const { return boundsMin; }
    const glm::vec3& maximum() const { return boundsMax; }
private:
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLsizei count = 0;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};