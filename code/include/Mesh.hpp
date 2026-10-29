#pragma once
#include <GL/glew.h>
#include <string>

class Mesh {
public:
    explicit Mesh(const std::string& objPath);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    void draw() const;
private:
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLsizei count = 0;
};