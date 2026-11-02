#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Timeline.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_image.h>

namespace {
class Texture {
public:
    explicit Texture(const std::string& path) {
        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!pixels) throw std::runtime_error("Ground texture unavailable: " + path);
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    explicit Texture(int kind = 0) {
        std::array<unsigned char, 64 * 64 * 3> pixels{};
        for (int row = 0; row < 64; ++row) {
            for (int column = 0; column < 64; ++column) {
                const bool trim = column % 16 < 2 || row < 4 || row >= 60;
                const int offset = (row * 64 + column) * 3;
                const bool joint = row % 16 == 0 || column % 16 == 0;
                pixels[offset] = kind == 1 ? 255 : kind == 2 ? (joint ? 85 : 165) : (trim ? 240 : 185);
                pixels[offset + 1] = kind == 1 ? 255 : kind == 2 ? (joint ? 90 : 170) : (trim ? 190 : 38);
                pixels[offset + 2] = kind == 1 ? 255 : kind == 2 ? (joint ? 95 : 175) : (trim ? 75 : 45);
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

glm::vec3 vector(animation::Point point) {
    return {point.x, point.y, point.z};
}

glm::mat4 placement(const Mesh& mesh, glm::vec3 position, float size, float yaw = 0) {
    const glm::vec3 extent = mesh.maximum() - mesh.minimum();
    const float largest = std::max({extent.x, extent.y, extent.z});
    if (!std::isfinite(largest) || largest < 0.00001f) throw std::runtime_error("Invalid model bounds");
    const glm::vec3 center = (mesh.maximum() + mesh.minimum()) * 0.5f;
    glm::mat4 transform = glm::translate(glm::mat4(1), position);
    transform = glm::rotate(transform, yaw, glm::vec3(0, 1, 0));
    transform = glm::scale(transform, glm::vec3(size / largest));
    return glm::translate(transform, glm::vec3(-center.x, -mesh.minimum().y, -center.z));
}

void run(GLFWwindow* window, const std::string& modelPath, const std::string& captureDirectory) {
    const std::string root = PROJECT_ROOT;
    Shader shader(root + "/code/shaders/phong.vert", root + "/code/shaders/phong.frag");
    std::cout << "Loading OBJ: " << modelPath << std::endl;
    Mesh gate(modelPath);
    const bool coreScene = modelPath.find("tiananmen_core.obj") != std::string::npos;
    const bool fullScene = coreScene || modelPath.find("tiananmen_balanced.obj") != std::string::npos
        || modelPath.find("Tiananmen+Square") != std::string::npos;
    glm::mat4 gateModel = placement(gate, {0, 0, -3}, 18.0f);
    if (fullScene) {
        gateModel = glm::translate(glm::mat4(1), glm::vec3(0, 0, -3));
        gateModel = glm::scale(gateModel, glm::vec3(18.0f / 7877.01f));
        gateModel = glm::translate(gateModel, glm::vec3(1331.935f, 0, -487.13f));
    }
    const std::array<std::string, 4> tankPaths = {"/model/tank/Tank.obj", "/model/tank2/Tank2.obj",
        "/model/tank3/Tank3.obj", "/model/tank4/Tank4.obj"};
    std::vector<std::unique_ptr<Mesh>> tanks;
    for (const auto& path : tankPaths) {
        std::cout << "Loading " << path << std::endl;
        tanks.push_back(std::make_unique<Mesh>(root + path));
        const auto extent = tanks.back()->maximum() - tanks.back()->minimum();
        std::cout << "Extent: " << extent.x << ", " << extent.y << ", " << extent.z << std::endl;
    }
    Mesh aircraft(root + "/model/j20/j-20_stealth_fighter_aircraft.obj");
    Mesh soldier(root + "/model/solider/source/\xe6\x9c\xaa\xe5\x91\xbd\xe5\x90\x8d.obj");
    Mesh cube(root + "/model/demo/cube.obj");
    Mesh ground(55.0f, 36);
    Mesh flag(2.4f, 1.6f, 64, 32);
    Texture white(1);
    Texture paving(root + "/model/tile.jpeg");
    std::cout << "Ready. Space: pause, R: replay, arrows: shots, B: bump toggle, Esc: exit" << std::endl;
    glEnable(GL_DEPTH_TEST);
    double previous = glfwGetTime();
    float seconds = 0.0f;
    bool paused = false;
    bool previousSpace = false;
    bool previousLeft = false, previousRight = false, previousBump = false;
    bool bump = true;
    const std::array<float, 5> captureTimes = {2.5f, 7.5f, 12.5f, 19.5f, 23.0f};
    std::size_t captureIndex = 0;
    if (!captureDirectory.empty()) std::filesystem::create_directories(captureDirectory);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float delta = static_cast<float>(now - previous);
        previous = now;
        const bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space && !previousSpace) paused = !paused;
        previousSpace = space;
        if (!paused) seconds = std::min(seconds + delta, animation::duration);
        if (!captureDirectory.empty()) seconds = captureTimes[captureIndex];
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) seconds = 0.0f;
        const bool left = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
        const bool right = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
        const bool toggleBump = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (left && !previousLeft) seconds = std::max(0.0f, (animation::shot(seconds).index - 1) * 5.0f);
        if (right && !previousRight) seconds = std::min(24.0f, (animation::shot(seconds).index + 1) * 5.0f);
        if (toggleBump && !previousBump) bump = !bump;
        previousLeft = left; previousRight = right; previousBump = toggleBump;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEventsTimeout(0.05);
            continue;
        }
        glViewport(0, 0, width, height);
        glClearColor(0.46f, 0.70f, 0.88f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto shot = animation::shot(seconds);
        const glm::vec3 camera = vector(shot.camera);
        const glm::vec3 target = vector(shot.target);
        const float aspect = static_cast<float>(width) / height;
        shader.use();
        shader.matrix("view", glm::lookAt(camera, target, glm::vec3(0, 1, 0)));
        shader.matrix("projection", glm::perspective(glm::radians(50.0f), aspect, 0.1f, 1800.0f));
        shader.vector("cameraPosition", camera);
        shader.vector("lightPosition", glm::vec3(-15, 35, 20));
        shader.integer("diffuseMap", 0);
        shader.integer("bumpEnabled", bump ? 1 : 0);
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        shader.vector("surfaceState", glm::vec3(bump ? 1 : 0, 0, 0));
        shader.matrix("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.02f, 0.0f)));
        if (!fullScene || coreScene) ground.draw(shader, paving.handle, glm::vec3(0.92f), paving.handle);
        shader.vector("surfaceState", glm::vec3(0));
        shader.matrix("model", gateModel);
        gate.draw(shader, white.handle, glm::vec3(1), paving.handle);
        const auto box = [&](glm::vec3 position, glm::vec3 size, glm::vec3 color) {
            const glm::vec3 extent = cube.maximum() - cube.minimum();
            glm::mat4 transform = glm::translate(glm::mat4(1), position);
            transform = glm::scale(transform, size / extent);
            transform = glm::translate(transform, -(cube.maximum() + cube.minimum()) * 0.5f);
            shader.matrix("model", transform);
            glBindTexture(GL_TEXTURE_2D, white.handle);
            shader.vector("tint", color);
            cube.draw();
        };
        box({0, 0.005f, 6}, {70, 0.02f, 5.0f}, {0.23f, 0.25f, 0.27f});
        for (int stripe = -12; stripe <= 12; ++stripe)
            box({stripe * 2.5f, 0.025f, 6}, {1.2f, 0.015f, 0.08f}, {0.92f, 0.86f, 0.6f});
        for (std::size_t index = 0; index < tanks.size(); ++index) {
            const float horizontal = -10.0f + (seconds - 3.0f) * 1.7f - index * 5.0f;
            const auto extent = tanks[index]->maximum() - tanks[index]->minimum();
            const float localSpeed = 1.7f * std::max({extent.x, extent.y, extent.z}) / 3.5f;
            shader.vector("animationState", glm::vec3(seconds, 0, localSpeed));
            shader.matrix("model", placement(*tanks[index], {horizontal, 0.03f, 5}, 3.5f, glm::radians(180.0f)));
            tanks[index]->draw(shader, white.handle, glm::vec3(1));
        }
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        for (int row = 0; row < 5; ++row) {
            for (int column = 0; column < 4; ++column) {
                const float lastTank = -10.0f + (seconds - 3.0f) * 1.7f - 15.0f;
                const float horizontal = lastTank - 4.0f - row * 0.9f;
                const float depth = 5.0f + (column - 1.5f) * 0.65f;
                shader.vector("animationState", glm::vec3(seconds, -1, 0));
                shader.matrix("model", placement(soldier, {horizontal, 0.03f, depth}, 1.8f, glm::radians(90.0f)));
                soldier.draw(shader, white.handle, glm::vec3(1));
            }
        }
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        box({2.5f, 3.6f, 8}, {0.065f, 7.2f, 0.065f}, {0.7f, 0.72f, 0.75f});
        shader.matrix("model", glm::translate(glm::mat4(1), glm::vec3(2.5f, 5.5f, 8)));
        shader.vector("animationState", glm::vec3(seconds, 1, 0));
        shader.vector("surfaceState", glm::vec3(0, 1, 0));
        flag.draw(shader, white.handle, glm::vec3(1));
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        shader.vector("surfaceState", glm::vec3(0));
        if (seconds >= 19.5f) {
            const auto point = vector(animation::aircraft(seconds));
            const auto tangent = vector(animation::aircraft(std::min(25.0f, seconds + 0.02f)))
                - vector(animation::aircraft(std::max(20.0f, seconds - 0.02f)));
            const float yaw = glm::length(tangent) > 0.001f ? std::atan2(tangent.x, tangent.z) : glm::radians(90.0f);
            const glm::vec3 forward = glm::length(tangent) > 0.001f ? glm::normalize(tangent) : glm::vec3(1, 0, 0);
            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            const float pitch = -std::atan2(forward.y, glm::length(glm::vec2(forward.x, forward.z)));
            for (int wing = -1; wing <= 1; ++wing) {
                const glm::vec3 offset = (wing == 0 ? 0.0f : -2.5f) * forward + wing * 2.8f * right;
                glm::mat4 flight = glm::translate(glm::mat4(1), point + offset);
                flight = glm::rotate(flight, yaw, glm::vec3(0, 1, 0));
                flight = glm::rotate(flight, pitch, glm::vec3(1, 0, 0));
                flight = glm::rotate(flight, glm::radians(60.0f), glm::vec3(0, 1, 0));
                shader.matrix("model", flight * placement(aircraft, glm::vec3(0), 3.4f));
                aircraft.draw(shader, white.handle, glm::vec3(0.88f));
            }
        }
        const char* names[] = {"Approach", "Armor", "March", "Flag", "Flypast"};
        const std::string title = "Parade Animation | " + std::string(names[shot.index]) + " | "
            + std::to_string(seconds).substr(0, 4) + " / 25 s" + (paused ? " | Paused" : "");
        glfwSetWindowTitle(window, title.c_str());
        if (!captureDirectory.empty()) {
            std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            stbi_flip_vertically_on_write(true);
            const auto path = std::filesystem::path(captureDirectory) / (std::to_string(captureIndex) + ".png");
            if (!stbi_write_png(path.string().c_str(), width, height, 4, pixels.data(), width * 4))
                throw std::runtime_error("Cannot save screenshot: " + path.string());
            std::cout << "Captured " << path << std::endl;
            if (++captureIndex == captureTimes.size()) glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        glfwSwapBuffers(window);
    }
}
}

int main(int argc, char* argv[]) {
    glfwSetErrorCallback([](int, const char* message) { std::cerr << message << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Parade Animation", nullptr, nullptr);
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
    try {
        std::string modelPath = std::string(PROJECT_ROOT) + "/model/tiananmen/tiananmen_balanced.obj";
        std::string captureDirectory;
        for (int argument = 1; argument < argc; ++argument) {
            if (std::string(argv[argument]) == "--capture") {
                if (++argument >= argc) throw std::runtime_error("--capture requires an output directory");
                captureDirectory = argv[argument];
            } else modelPath = argv[argument];
        }
        run(window, modelPath, captureDirectory);
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; result = 1; }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
