#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader;

class Mesh {
public:
    explicit Mesh(const std::string& objPath);
    // Flat XZ plane at y = 0, centred on the origin, normal facing up.
    explicit Mesh(float halfExtent, int subdivisions = 1);
    Mesh(float width, float height, int columns, int rows);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    void draw() const;
    void draw(const Shader& shader, GLuint fallback, const glm::vec3& tint, GLuint paving = 0) const;
    const glm::vec3& minimum() const { return boundsMin; }
    const glm::vec3& maximum() const { return boundsMax; }
private:
    struct Batch {
        GLint first;
        GLsizei count;
        glm::vec3 color;
        GLuint texture;
        bool track;
        bool ground = false;
        bool roof = false;
    };
    std::vector<Batch> batches;
    std::vector<GLuint> textures;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLsizei count = 0;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};
