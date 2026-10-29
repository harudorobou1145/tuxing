#include "Shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
GLuint compile(GLenum type, const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open shader: " + path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string source = buffer.str();
    const char* text = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[4096]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(path + ": " + log);
    }
    return shader;
}
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = 0;
    try {
        fragment = compile(GL_FRAGMENT_SHADER, fragmentPath);
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[4096]{};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Shader link: ") + log);
    }
}
Shader::~Shader() { glDeleteProgram(program); }
void Shader::use() const { glUseProgram(program); }
void Shader::matrix(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(value));
}
void Shader::vector(const char* name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
}
void Shader::integer(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(program, name), value);
}