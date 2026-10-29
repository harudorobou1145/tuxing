#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    void use() const;
    void matrix(const char* name, const glm::mat4& value) const;
    void vector(const char* name, const glm::vec3& value) const;
    void integer(const char* name, int value) const;
private:
    GLuint program = 0;
};