#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Timeline.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
class Texture {
public:
    Texture() {
        std::array<unsigned char, 64 * 64 * 3> pixels{};
        for (int row = 0; row < 64; ++row) {
            for (int column = 0; column < 64; ++column) {
                const bool trim = column % 16 < 2 || row < 4 || row >= 60;
                const int offset = (row * 64 + column) * 3;
                pixels[offset] = trim ? 240 : 185;
                pixels[offset + 1] = trim ? 190 : 38;
                pixels[offset + 2] = trim ? 75 : 45;
            }
        }
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    ~Texture() { glDeleteTextures(1, &handle); }
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    GLuint handle = 0;
};

void run(GLFWwindow* window) {
    const std::string root = PROJECT_ROOT;
    Shader shader(root + "/code/shaders/phong.vert", root + "/code/shaders/phong.frag");
    Mesh lantern(root + "/model/lantern.obj");
    Mesh ground(root + "/model/ground.obj");
    Texture texture;
    glEnable(GL_DEPTH_TEST);
    double previous = glfwGetTime();
    float seconds = 0.0f;
    bool paused = false;
    bool previousSpace = false;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float delta = static_cast<float>(now - previous);
        previous = now;
        const bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space && !previousSpace) paused = !paused;
        previousSpace = space;
        if (!paused) seconds = std::min(seconds + delta, animation::duration);
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) seconds = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEventsTimeout(0.05);
            continue;
        }
        glViewport(0, 0, width, height);
        glClearColor(0.075f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const float progress = animation::progress(seconds);
        const glm::vec3 camera(9.0f * std::sin(0.35f + progress * 0.5f), 5.0f, 11.0f * std::cos(0.35f + progress * 0.5f));
        shader.use();
        shader.matrix("view", glm::lookAt(camera, glm::vec3(0, 2, 0), glm::vec3(0, 1, 0)));
        shader.matrix("projection", glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f));
        shader.vector("cameraPosition", camera);
        shader.vector("lightPosition", glm::vec3(2, 7, 5));
        shader.integer("diffuseMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.handle);
        shader.vector("tint", glm::vec3(0.32f, 0.65f, 0.8f));
        shader.matrix("model", glm::mat4(1.0f));
        ground.draw();
        shader.vector("tint", glm::vec3(1.0f));
        for (int index = 0; index < 7; ++index) {
            const auto point = animation::bezier(progress);
            const glm::vec3 position = index == 0
                ? glm::vec3(point.x, point.y, point.z)
                : glm::vec3((index - 3.5f) * 1.5f, 1.5f + 0.15f * std::sin(seconds + index), -2.5f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
            model = glm::rotate(model, 0.1f * std::sin(seconds + index), glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(0.6f, 0.85f, 0.6f));
            shader.matrix("model", model);
            lantern.draw();
        }
        const std::string title = "Lantern Festival | " + std::to_string(seconds).substr(0, 4) + " / 25 s" + (paused ? " | Paused" : "");
        glfwSetWindowTitle(window, title.c_str());
        glfwSwapBuffers(window);
    }
}
}

int main() {
    glfwSetErrorCallback([](int, const char* message) { std::cerr << message << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Lantern Festival", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Cannot initialize GLEW\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwSwapInterval(1);
    int result = 0;
    try { run(window); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; result = 1; }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}