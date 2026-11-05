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
        if (!pixels) throw std::runtime_error("Texture unavailable: " + path);
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

class ShadowMap {
public:
    static constexpr int resolution = 2048;
    ShadowMap() {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float border[] = {1, 1, 1, 1};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (!complete) {
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &texture);
            throw std::runtime_error("Shadow framebuffer is incomplete");
        }
    }
    ~ShadowMap() {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
    }
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    GLuint texture = 0;
    GLuint framebuffer = 0;
};

class Sky {
public:
    explicit Sky(const std::string& root)
        : shader(root + "/code/shaders/sky.vert", root + "/code/shaders/sky.frag"),
          panorama(root + "/model/sky/day_sky.png") {
        glBindTexture(GL_TEXTURE_2D, panorama.handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenVertexArrays(1, &vertexArray);
    }
    ~Sky() { glDeleteVertexArrays(1, &vertexArray); }
    Sky(const Sky&) = delete;
    Sky& operator=(const Sky&) = delete;
    void draw(const glm::mat4& view, const glm::mat4& projection) const {
        shader.use();
        shader.matrix("inverseView", glm::inverse(view));
        shader.matrix("inverseProjection", glm::inverse(projection));
        shader.integer("skyMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, panorama.handle);
        // Keep a second binding for the distant terrain's atmospheric fade.
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, panorama.handle);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vertexArray);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }
private:
    Shader shader;
    Texture panorama;
    GLuint vertexArray = 0;
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

glm::vec3 aircraftPosition(float t, int wing) {
    const auto point = vector(animation::aircraft(t));
    const auto tangent = vector(animation::aircraft(std::min(animation::duration, t + 0.02f)))
        - vector(animation::aircraft(std::max(animation::aircraftStart, t - 0.02f)));
    const glm::vec3 forward = glm::length(tangent) > 0.001f ? glm::normalize(tangent) : glm::vec3(1, 0, 0);
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    return point - std::abs(wing) * 2.5f * forward + wing * 2.2f * right;
}

void run(GLFWwindow* window, const std::string& modelPath, const std::string& captureDirectory) {
    const std::string root = PROJECT_ROOT;
    Shader shader(root + "/code/shaders/phong.vert", root + "/code/shaders/phong.frag");
    Sky sky(root);
    ShadowMap shadows;
    const glm::mat4 lightView = glm::lookAt(glm::vec3(-45, 80, 55), glm::vec3(0, 0, 12), glm::vec3(0, 1, 0));
    const glm::mat4 lightProjection = glm::ortho(-72.0f, 72.0f, -52.0f, 65.0f, 1.0f, 220.0f);
    std::cout << "Loading OBJ: " << modelPath << std::endl;
    const bool coreScene = modelPath.find("tiananmen_core.obj") != std::string::npos;
    const bool fullScene = coreScene || modelPath.find("tiananmen_balanced.obj") != std::string::npos
        || modelPath.find("Tiananmen+Square") != std::string::npos;
    Mesh gate(modelPath, fullScene);
    std::unique_ptr<Mesh> monument;
    glm::mat4 monumentModel(1.0f);
    glm::mat4 gateModel = placement(gate, {0, 0, -3}, 18.0f);
    if (fullScene) {
        gateModel = glm::translate(glm::mat4(1), glm::vec3(0, 0, -3));
        gateModel = glm::scale(gateModel, glm::vec3(18.0f / 7877.01f));
        gateModel = glm::translate(gateModel, glm::vec3(1331.935f, 0, -487.13f));
        monument = std::make_unique<Mesh>(root + "/model/tiananmen/heroes_monument.obj");
        // Replace Cube_3's hollow remnants with the extracted monument and its base.
        const auto extent = monument->maximum() - monument->minimum();
        const float platformWidth = 2653.689f;
        const float scale = 0.90f * platformWidth / std::max(extent.x, extent.z);
        const float size = scale * std::max({extent.x, extent.y, extent.z});
        // Local terrain peaks at 132.94; place the foundation just above it.
        monumentModel = gateModel * placement(*monument, {-1509.614f, 134.0f, 30016.281f}, size);
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
    Mesh ground(600.0f, 36);
    Mesh flag(2.4f, 1.6f, 64, 32);
    Mesh smokeQuad(1.0f, 1.0f, 1, 1);
    Mesh foliage(1.0f, 10, 7);
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
    const std::array<float, 12> captureTimes = {0.0f, 2.5f, 8.5f, 16.5f, 23.5f, 24.8f, 26.0f, 27.0f, 28.5f, 33.0f, 34.0f, 35.0f};
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
        if (left && !previousLeft) seconds = animation::shotTimes[std::max(0, animation::shot(seconds).index - 1)];
        if (right && !previousRight) seconds = animation::shotTimes[std::min(5, animation::shot(seconds).index + 1)];
        if (toggleBump && !previousBump) bump = !bump;
        previousLeft = left; previousRight = right; previousBump = toggleBump;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEventsTimeout(0.05);
            continue;
        }
        for (int renderPass = 0; renderPass < 2; ++renderPass) {
        const bool depthPass = renderPass == 0;
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthPass ? 0 : shadows.texture);
        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, depthPass ? shadows.framebuffer : 0);
        glViewport(0, 0, depthPass ? ShadowMap::resolution : width, depthPass ? ShadowMap::resolution : height);
        glClearColor(0.46f, 0.70f, 0.88f, 1.0f);
        glClear(depthPass ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto shot = animation::shot(seconds);
        const glm::vec3 camera = vector(shot.camera);
        const glm::vec3 target = vector(shot.target);
        const float aspect = static_cast<float>(width) / height;
        const glm::mat4 view = glm::lookAt(camera, target, glm::vec3(0, 1, 0));
        const glm::mat4 projection = glm::perspective(glm::radians(44.0f), aspect, 0.1f, 1800.0f);
        if (!depthPass) sky.draw(view, projection);
        shader.use();
        shader.matrix("view", depthPass ? lightView : view);
        shader.matrix("projection", depthPass ? lightProjection : projection);
        shader.matrix("lightSpace", lightProjection * lightView);
        shader.integer("depthOnly", depthPass ? 1 : 0);
        shader.integer("shadowMap", 2);
        shader.integer("unlit", 0);
        shader.vector("cameraPosition", camera);
        shader.vector("lightPosition", glm::normalize(glm::vec3(-45, 80, 43)));
        shader.integer("diffuseMap", 0);
        shader.integer("skyEnvironmentMap", 1);
        shader.integer("bumpEnabled", bump ? 1 : 0);
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        shader.vector("surfaceState", glm::vec3(bump ? 1 : 0, 0, 0));
        shader.matrix("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.02f, 0.0f)));
        ground.draw(shader, paving.handle, glm::vec3(0.92f), paving.handle);
        shader.vector("surfaceState", glm::vec3(0));
        shader.matrix("model", gateModel);
        gate.draw(shader, white.handle, glm::vec3(1), paving.handle);
        if (monument) {
            shader.matrix("model", monumentModel);
            monument->draw(shader, white.handle, glm::vec3(1));
        }
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
        if (fullScene) {
            const float sceneScale = 18.0f / 7877.01f;
            const auto sourceBox = [&](glm::vec3 center, glm::vec3 size, glm::vec3 color) {
                box(glm::vec3(gateModel * glm::vec4(center, 1)), size * sceneScale, color);
            };
            for (int column = 0; column < 19; ++column) {
                const float depth = 17400.0f + column * 1060.0f;
                sourceBox({14060, 1000, depth}, {260, 1700, 280}, {0.78f, 0.77f, 0.70f});
                sourceBox({14060, 160, depth}, {390, 180, 420}, {0.65f, 0.65f, 0.60f});
                sourceBox({14060, 1860, depth}, {410, 180, 440}, {0.85f, 0.83f, 0.74f});
            }
            for (int tier = 0; tier < 3; ++tier)
                sourceBox({14100, 2010.0f + tier * 110.0f, 27000},
                    {700.0f + tier * 110.0f, 85, 21800.0f + tier * 100.0f}, {0.72f, 0.71f, 0.64f});
            const glm::vec3 monumentCenter = glm::vec3(gateModel * glm::vec4(-1509.614f, 134, 30016.281f, 1));
            for (int side : {-1, 1}) {
                for (int post = -5; post <= 5; ++post) {
                    const float along = post * 0.42f;
                    box(monumentCenter + glm::vec3(along, 0.48f, side * 2.36f), {0.10f, 0.62f, 0.10f}, {0.82f, 0.83f, 0.79f});
                    box(monumentCenter + glm::vec3(side * 2.36f, 0.48f, along), {0.10f, 0.62f, 0.10f}, {0.82f, 0.83f, 0.79f});
                }
                box(monumentCenter + glm::vec3(0, 0.65f, side * 2.36f), {4.65f, 0.09f, 0.10f}, {0.87f, 0.86f, 0.81f});
                box(monumentCenter + glm::vec3(side * 2.36f, 0.65f, 0), {0.10f, 0.09f, 4.65f}, {0.87f, 0.86f, 0.81f});
            }
        }
        box({0, 0.055f, animation::paradeDepth}, {100, 0.02f, 5.0f}, {0.23f, 0.25f, 0.27f});
        for (int stripe = -12; stripe <= 12; ++stripe)
            box({stripe * 2.5f, 0.075f, animation::paradeDepth + 2.2f}, {1.2f, 0.015f, 0.08f}, {0.92f, 0.86f, 0.6f});
        for (int side : {-1, 1}) {
            const float edge = animation::paradeDepth + side * 2.85f;
            box({0, 0.075f, edge + side * 0.55f}, {100, 0.06f, 0.9f}, {0.62f, 0.64f, 0.62f});
            for (int section = -49; section <= 49; ++section) {
                const float horizontal = static_cast<float>(section);
                box({horizontal, 0.13f, edge}, {0.97f, 0.16f, 0.28f},
                    section % 2 == 0 ? glm::vec3(0.71f, 0.73f, 0.70f) : glm::vec3(0.62f, 0.65f, 0.64f));
            }
            for (int drain = -5; drain <= 5; ++drain) {
                for (int slot = 0; slot < 6; ++slot)
                    box({drain * 8.0f + slot * 0.08f, 0.081f, edge - side * 0.22f},
                        {0.035f, 0.012f, 0.18f}, {0.12f, 0.15f, 0.16f});
            }
            for (int post = -5; post <= 5; ++post) {
                const float horizontal = post * 8.0f;
                box({horizontal, 0.36f, edge + side * 1.2f}, {0.14f, 0.6f, 0.14f}, {0.27f, 0.31f, 0.32f});
                box({horizontal, 0.60f, edge + side * 1.2f}, {0.16f, 0.07f, 0.16f}, {0.78f, 0.74f, 0.57f});
            }
        }
        const auto crown = [&](glm::vec3 position, glm::vec3 size, glm::vec3 color) {
            shader.matrix("model", glm::scale(glm::translate(glm::mat4(1), position), size));
            shader.vector("tint", color);
            foliage.draw();
        };
        for (int side : {-1, 1}) {
            for (int cluster = -3; cluster <= 3; ++cluster) {
                if (cluster == 0) continue;
                const float horizontal = cluster * 12.0f;
                const float depth = animation::paradeDepth + side * 9.0f;
                box({horizontal, 0.23f, depth}, {4.2f, 0.35f, 2.4f}, {0.62f, 0.65f, 0.62f});
                box({horizontal, 0.42f, depth}, {3.9f, 0.08f, 2.1f}, {0.22f, 0.24f, 0.16f});
                box({horizontal, 1.45f, depth}, {0.22f, 2.1f, 0.22f}, {0.30f, 0.25f, 0.19f});
                crown({horizontal, 3.0f, depth}, {1.35f, 1.65f, 1.25f}, {0.19f, 0.36f, 0.21f});
                crown({horizontal - 0.8f, 2.7f, depth + 0.15f}, {0.85f, 1.1f, 0.9f}, {0.25f, 0.42f, 0.24f});
                crown({horizontal + 0.75f, 2.95f, depth - 0.1f}, {0.9f, 1.2f, 0.85f}, {0.22f, 0.39f, 0.25f});
                for (int shrub : {-1, 1}) {
                    crown({horizontal + shrub * 1.4f, 0.75f, depth + 0.45f}, {0.55f, 0.42f, 0.55f}, {0.32f, 0.46f, 0.23f});
                    for (int bloom = 0; bloom < 3; ++bloom)
                        crown({horizontal + shrub * 1.4f + (bloom - 1) * 0.22f, 1.07f, depth + 0.5f},
                            {0.12f, 0.09f, 0.12f}, {0.78f, 0.24f + bloom * 0.12f, 0.24f});
                }
            }
        }
        for (std::size_t index = 0; index < tanks.size(); ++index) {
            const float horizontal = animation::tankPosition(seconds, static_cast<int>(index));
            const auto extent = tanks[index]->maximum() - tanks[index]->minimum();
            const float localSpeed = animation::tankSpeed * std::max({extent.x, extent.y, extent.z}) / 6.0f;
            shader.vector("animationState", glm::vec3(seconds, 0, localSpeed));
            shader.matrix("model", placement(*tanks[index], {horizontal, 0.08f, animation::paradeDepth}, 6.0f, glm::radians(180.0f)));
            tanks[index]->draw(shader, white.handle, glm::vec3(1));
        }
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        for (int row = 0; row < 25; ++row) {
            for (int column = 0; column < 4; ++column) {
                const float horizontal = animation::soldierPosition(seconds, row);
                const float depth = animation::paradeDepth + (column - 1.0f) * 1.1f;
                shader.vector("animationState", glm::vec3(seconds, -1, 0));
                shader.matrix("model", placement(soldier, {horizontal, 0.08f, depth}, 1.7f, glm::radians(90.0f)));
                soldier.draw(shader, white.handle, glm::vec3(1));
            }
        }
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        for (int step = 0; step < 3; ++step) {
            const float width = 2.8f - step * 0.5f;
            box({animation::flagPoleX, animation::flagBaseOffset + 0.035f + step * 0.055f, animation::flagPoleZ},
                {width, 0.055f, width}, {0.77f + step * 0.035f, 0.79f + step * 0.03f, 0.76f + step * 0.03f});
        }
        for (int seam = -2; seam <= 2; ++seam) {
            box({animation::flagPoleX + seam * 0.32f, animation::flagBaseOffset + 0.175f, animation::flagPoleZ},
                {0.009f, 0.004f, 1.75f}, {0.53f, 0.56f, 0.55f});
            box({animation::flagPoleX, animation::flagBaseOffset + 0.175f, animation::flagPoleZ + seam * 0.32f},
                {1.75f, 0.004f, 0.009f}, {0.53f, 0.56f, 0.55f});
        }
        box({animation::flagPoleX, animation::flagBaseOffset + 0.24f, animation::flagPoleZ},
            {0.28f, 0.15f, 0.28f}, {0.52f, 0.56f, 0.58f});
        for (float collar : {0.36f, 1.1f, 6.9f})
            box({animation::flagPoleX, animation::flagBaseOffset + collar, animation::flagPoleZ},
                {0.09f, 0.055f, 0.09f}, {0.82f, 0.84f, 0.83f});
        box({animation::flagPoleX, 3.65f + animation::flagBaseOffset, animation::flagPoleZ},
            {0.065f, 7.2f, 0.065f}, {0.7f, 0.72f, 0.75f});
        shader.matrix("model", glm::translate(glm::mat4(1),
            glm::vec3(animation::flagPoleX, animation::flagHeight(seconds), animation::flagPoleZ)));
        shader.vector("animationState", glm::vec3(seconds, 1, 0));
        shader.vector("surfaceState", glm::vec3(0, 1, 0));
        flag.draw(shader, white.handle, glm::vec3(1));
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        shader.vector("surfaceState", glm::vec3(0));
        // Fireworks behind Tiananmen: launch -> rise -> burst, varied rhythm/size.
        if (!depthPass) {
            struct Firework {
                glm::vec3 launch;
                glm::vec3 peak;
                glm::vec3 color;
                float period;
                float offset;
                float rise;
                float radius;
                int streaks;
            };
            const std::array<Firework, 6> fireworks = {{
                {glm::vec3(-14, 2, -10), glm::vec3(-13, 12, -9),  glm::vec3(1.0f, 0.80f, 0.30f), 3.6f, 0.0f, 1.3f, 6.0f, 26},
                {glm::vec3(-6, 2, -13),  glm::vec3(-5, 9, -12),   glm::vec3(1.0f, 0.30f, 0.25f), 2.9f, 0.7f, 1.0f, 4.5f, 20},
                {glm::vec3(2, 2, -15),   glm::vec3(3, 13, -14),   glm::vec3(0.35f, 0.60f, 1.0f), 4.3f, 1.5f, 1.5f, 7.5f, 30},
                {glm::vec3(12, 2, -8),   glm::vec3(13, 10, -7),   glm::vec3(0.30f, 0.95f, 0.40f), 3.2f, 2.2f, 1.1f, 5.0f, 22},
                {glm::vec3(-9, 2, -7),   glm::vec3(-8, 11, -6),   glm::vec3(1.0f, 0.40f, 0.65f), 3.8f, 1.1f, 1.4f, 6.5f, 28},
                {glm::vec3(8, 2, -16),   glm::vec3(9, 8, -15),    glm::vec3(1.0f, 0.90f, 0.35f), 2.6f, 1.9f, 0.9f, 3.5f, 16},
            }};
            const glm::vec3 fwForward = glm::normalize(target - camera);
            const glm::vec3 fwRight = glm::normalize(glm::cross(fwForward, glm::vec3(0, 1, 0)));
            const glm::vec3 fwUp = glm::cross(fwRight, fwForward);
            const auto puff = [&](const glm::vec3& pos, float size, const glm::vec3& color, float alpha) {
                shader.vector("tint", color);
                shader.floating("smokeAlpha", alpha);
                glm::mat4 b(1.0f);
                b[0] = glm::vec4(fwRight * size, 0.0f);
                b[1] = glm::vec4(fwUp * size, 0.0f);
                b[2] = glm::vec4(fwForward, 0.0f);
                b[3] = glm::vec4(pos, 1.0f);
                b = glm::translate(b, glm::vec3(-0.5f, -0.5f, 0.0f));
                shader.matrix("model", b);
                smokeQuad.draw();
            };
            glBindTexture(GL_TEXTURE_2D, white.handle);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            shader.integer("unlit", 3);
            for (const auto& fw : fireworks) {
                const float t = std::fmod(seconds + fw.offset, fw.period);
                if (t < fw.rise) {
                    const float p = t / fw.rise;
                    const auto ascent = [&](float progress) {
                        return glm::mix(fw.launch, fw.peak, progress * (1.6f - 0.6f * progress));
                    };
                    for (int sample = 16; sample >= 1; --sample) {
                        const float history = p - sample * 0.012f;
                        if (history < 0.0f) continue;
                        const float fade = 1.0f - sample / 17.0f;
                        puff(ascent(history), 0.10f + 0.12f * fade,
                            glm::vec3(1.0f, 0.55f, 0.18f), 0.48f * fade);
                    }
                    puff(ascent(p), 0.38f, glm::vec3(1.0f, 0.93f, 0.72f), 1.0f);
                } else {
                    const float burst = fw.period - fw.rise;
                    const float age = t - fw.rise;
                    const int particles = fw.streaks * 3;
                    for (int particle = 0; particle < particles; ++particle) {
                        const float seed = 0.5f + 0.5f * std::sin(particle * 127.1f + fw.offset * 311.7f);
                        const float lifetime = burst * (0.65f + 0.35f * seed);
                        if (age >= lifetime) continue;
                        const float vertical = 1.0f - 2.0f * (particle + 0.5f) / particles;
                        const float horizontal = std::sqrt(std::max(0.0f, 1.0f - vertical * vertical));
                        const float angle = particle * 2.399963f + fw.offset;
                        const glm::vec3 direction(horizontal * std::cos(angle), vertical, horizontal * std::sin(angle));
                        const glm::vec3 velocity = direction * (fw.radius * (0.85f + 0.3f * seed));
                        const auto trajectory = [&](float elapsed) {
                            return fw.peak + velocity * ((1.0f - std::exp(-1.15f * elapsed)) / 1.15f)
                                + glm::vec3(0.12f * elapsed, -1.3f * elapsed * elapsed, 0);
                        };
                        const float remaining = 1.0f - age / lifetime;
                        const float flicker = 0.85f + 0.15f * std::sin(age * 35.0f + particle * 2.7f);
                        const float alpha = std::pow(remaining, 0.7f) * flicker;
                        const glm::vec3 color = glm::mix(glm::vec3(1.0f, 0.48f, 0.12f), fw.color, remaining);
                        for (int sample = 9; sample >= 1; --sample) {
                            const float history = age - sample * 0.018f;
                            if (history < 0.0f) continue;
                            const float fade = 1.0f - sample / 10.0f;
                            puff(trajectory(history), 0.08f + 0.09f * fade, color, alpha * fade * 0.45f);
                        }
                        const glm::vec3 position = trajectory(age);
                        puff(position, 0.48f, color, alpha * 0.12f);
                        puff(position, 0.16f, glm::mix(color, glm::vec3(1.0f), 0.55f), alpha);
                    }
                    if (age < 0.18f) {
                        const float flash = 1.0f - age / 0.18f;
                        puff(fw.peak, 1.8f + age * 4.0f, fw.color, flash * 0.45f);
                        puff(fw.peak, 0.65f, glm::vec3(1.0f), flash);
                    }
                }
            }
            shader.floating("smokeAlpha", 1.0f);
            shader.integer("unlit", 0);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
        if (seconds >= animation::aircraftStart) {
            const auto point = vector(animation::aircraft(seconds));
            const auto tangent = vector(animation::aircraft(std::min(animation::duration, seconds + 0.02f)))
                - vector(animation::aircraft(std::max(animation::aircraftStart, seconds - 0.02f)));
            const float yaw = glm::length(tangent) > 0.001f ? std::atan2(tangent.x, tangent.z) : glm::radians(90.0f);
            const glm::vec3 forward = glm::length(tangent) > 0.001f ? glm::normalize(tangent) : glm::vec3(1, 0, 0);
            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            const float pitch = -std::atan2(forward.y, glm::length(glm::vec2(forward.x, forward.z)));
            for (int wing = -2; wing <= 2; ++wing) {
                const glm::vec3 offset = -std::abs(wing) * 2.5f * forward + wing * 2.2f * right;
                glm::mat4 flight = glm::translate(glm::mat4(1), point + offset);
                flight = glm::rotate(flight, yaw, glm::vec3(0, 1, 0));
                flight = glm::rotate(flight, pitch, glm::vec3(1, 0, 0));
                flight = glm::rotate(flight, glm::radians(60.0f), glm::vec3(0, 1, 0));
                shader.matrix("model", flight * placement(aircraft, glm::vec3(0), 3.4f));
                aircraft.draw(shader, white.handle, glm::vec3(0.88f));
            }
            if (!depthPass) {
            // Colored smoke trails behind each aircraft.
            const std::array<glm::vec3, 5> smokeColors = {
                glm::vec3(0.85f, 0.10f, 0.10f),
                glm::vec3(0.95f, 0.82f, 0.10f),
                glm::vec3(0.05f, 0.15f, 0.75f),
                glm::vec3(0.10f, 0.80f, 0.30f),
                glm::vec3(0.95f, 0.95f, 0.95f),
            };
            const glm::vec3 camForward = glm::normalize(target - camera);
            const glm::vec3 camRight = glm::normalize(glm::cross(camForward, glm::vec3(0, 1, 0)));
            const glm::vec3 camUp = glm::cross(camRight, camForward);
            glBindTexture(GL_TEXTURE_2D, white.handle);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            shader.integer("unlit", 1);
            shader.integer("unlit", 2);
            const int segments = 160;
            const float trailDuration = 3.2f;
            for (int wing = -2; wing <= 2; ++wing) {
                shader.vector("tint", smokeColors[wing + 2]);
                for (int k = segments - 1; k >= 0; --k) {
                    const float tFar = seconds - trailDuration * (k + 1) / segments;
                    const float tNear = seconds - trailDuration * k / segments;
                    const float age = (k + 0.5f) / segments;
                    if (tFar < animation::aircraftStart) continue;
                    const glm::vec3 trailStart = aircraftPosition(tFar, wing);
                    const glm::vec3 trailEnd = aircraftPosition(tNear, wing);
                    const glm::vec3 trailStep = trailEnd - trailStart;
                    const glm::vec3 trailForward = glm::length(trailStep) > 0.001f ? glm::normalize(trailStep) : forward;
                    const float smokeAge = seconds - (tFar + tNear) * 0.5f;
                    const float emission = (tFar + tNear) * 0.5f;
                    glm::vec3 mid = (trailStart + trailEnd) * 0.5f - trailForward * 1.35f;
                    mid += glm::vec3(0.32f * smokeAge, 0.12f * smokeAge, 0);
                    mid += glm::vec3(std::sin(emission * 4.1f + wing), std::sin(emission * 3.3f),
                        std::cos(emission * 3.7f + wing)) * (0.20f * smokeAge);
                    const float puffWidth = 0.20f + 0.68f * std::sqrt(smokeAge);
                    const glm::vec2 projected(glm::dot(trailStep, camRight), glm::dot(trailStep, camUp));
                    const float projectedLength = glm::length(projected);
                    const glm::vec2 axis = projectedLength > 0.001f ? projected / projectedLength : glm::vec2(1, 0);
                    const glm::vec3 ribbonAlong = camRight * axis.x + camUp * axis.y;
                    const glm::vec3 ribbonAcross = -camRight * axis.y + camUp * axis.x;
                    shader.floating("smokeAlpha", 0.38f * std::pow(1.0f - age, 1.4f)
                        * std::exp(-glm::length(mid - camera) * 0.0028f));
                    glm::mat4 billboard(1.0f);
                    billboard[0] = glm::vec4(ribbonAlong * (projectedLength + puffWidth * 1.8f), 0.0f);
                    billboard[1] = glm::vec4(ribbonAcross * puffWidth, 0.0f);
                    billboard[2] = glm::vec4(camForward, 0.0f);
                    billboard[3] = glm::vec4(mid, 1.0f);
                    billboard = glm::translate(billboard, glm::vec3(-0.5f, -0.5f, 0.0f));
                    shader.matrix("model", billboard);
                    smokeQuad.draw();
                }
            }
            shader.floating("smokeAlpha", 1.0f);
            shader.integer("unlit", 0);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            }
        }
        }
        const auto shot = animation::shot(seconds);
        const char* names[] = {"Approach", "Armor", "March", "Flag", "Flypast", "Finale"};
        const std::string title = "Parade Animation | " + std::string(names[shot.index]) + " | "
            + std::to_string(seconds).substr(0, 4) + " / "
            + std::to_string(static_cast<int>(animation::duration)) + " s" + (paused ? " | Paused" : "");
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
