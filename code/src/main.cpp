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
#include <random>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iterator>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_image.h>

namespace {
class Texture {
public:
    explicit Texture(const std::string& path) {
        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
        if (!file) throw std::runtime_error("Texture unavailable: " + path);
        std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        unsigned char* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
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
    static constexpr int resolution = 4096;
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

class ReflectionMap {
public:
    static constexpr int resolution = 512;
    static constexpr int faceCount = 6;
    ReflectionMap() {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        for (int face = 0; face < faceCount; ++face) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB8,
                resolution, resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glGenRenderbuffers(faceCount, depth.data());
        for (const GLuint buffer : depth) {
            glBindRenderbuffer(GL_RENDERBUFFER, buffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glGenFramebuffers(1, &framebuffer);
        bindFace(0);
        const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (!complete) {
            release();
            throw std::runtime_error("Reflection framebuffer is incomplete");
        }
    }
    ~ReflectionMap() { release(); }
    ReflectionMap(const ReflectionMap&) = delete;
    ReflectionMap& operator=(const ReflectionMap&) = delete;

    void bindFace(int face, GLenum target = GL_FRAMEBUFFER) const {
        glBindFramebuffer(target, framebuffer);
        glFramebufferTexture2D(target, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0);
        glFramebufferRenderbuffer(target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth[face]);
    }
    void copyFace(const ReflectionMap& background, int face) const {
        background.bindFace(face, GL_READ_FRAMEBUFFER);
        bindFace(face, GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, resolution, resolution, 0, 0, resolution, resolution,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }
    static glm::mat4 faceView(glm::vec3 center, int face) {
        constexpr std::array<glm::vec3, faceCount> directions{{
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
        constexpr std::array<glm::vec3, faceCount> up{{
            {0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}}};
        return glm::lookAt(center, center + directions[face], up[face]);
    }

    GLuint texture = 0;
private:
    void release() {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteRenderbuffers(faceCount, depth.data());
        glDeleteTextures(1, &texture);
    }
    GLuint framebuffer = 0;
    std::array<GLuint, faceCount> depth{};
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
    return point - std::abs(wing) * 12.0f * forward + wing * 10.0f * right;
}

constexpr float balloonRelease = animation::balloonRelease;
constexpr float verticalFov = glm::radians(44.0f);
constexpr int balloonColumns = 14;
constexpr int balloonsPerColumn = 6;

struct Balloon {
    float across;
    float depth;
    glm::vec3 color;
    float radius;
    float speed;
    float phase;
};

using Balloons = std::array<Balloon, balloonColumns * balloonsPerColumn>;

Balloons makeBalloons() {
    constexpr std::array<glm::vec3, 6> colors{{{0.96f, 0.12f, 0.16f}, {1.0f, 0.71f, 0.10f},
        {0.17f, 0.62f, 0.97f}, {0.22f, 0.82f, 0.47f}, {0.93f, 0.48f, 0.73f}, {0.96f, 0.95f, 0.89f}}};
    std::mt19937 random(73);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    Balloons balloons;
    for (std::size_t index = 0; index < balloons.size(); ++index) {
        const auto column = index / balloonsPerColumn;
        const auto speedBand = index % balloonsPerColumn;
        const float depth = animation::balloonDepthMin + animation::balloonDepthRange * unit(random);
        const float across = 1.84f * (column + 0.12f + 0.76f * unit(random)) / balloonColumns - 0.92f;
        const float speed = 0.45f + 0.55f * (speedBand + unit(random)) / balloonsPerColumn;
        balloons[index] = {
            across, depth, colors[static_cast<std::size_t>(random()) % colors.size()],
            depth * (0.009f + 0.005f * unit(random)), depth * speed, 6.2831853f * unit(random),
        };
    }
    return balloons;
}

void drawBalloons(const Balloons& balloons, float seconds, float aspect, const Mesh& sphere,
                  const Shader& shader, GLuint texture) {
    // Fix the launch area ahead of the 29-second view, rather than attaching it to the camera.
    const auto framing = animation::shot(balloonRelease);
    const glm::vec3 eye = vector(framing.camera);
    glm::vec3 forward = vector(framing.target) - eye;
    forward.y = 0.0f;
    forward = glm::normalize(forward);
    const glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
    const float halfWidth = std::tan(verticalFov * 0.5f) * aspect;
    const float age = (seconds - balloonRelease) * animation::balloonRiseScale;
    glBindTexture(GL_TEXTURE_2D, texture);
    // The existing procedural sphere is wound clockwise when viewed from outside.
    glFrontFace(GL_CW);
    for (const auto& balloon : balloons) {
        const float phase = age * 1.4f + balloon.phase;
        glm::vec3 position = eye + forward * balloon.depth
            + right * (balloon.across * balloon.depth * halfWidth + 1.1f * age * std::sin(phase))
            + glm::vec3(0, 0, 0.35f * age * std::cos(age * 0.8f + balloon.phase));
        position.y = animation::balloonLaunchHeight(balloon.depth, balloon.radius)
            + balloon.speed * age + 0.5f * std::sin(age * 1.1f + balloon.phase * 2.0f);
        const glm::mat4 pose = glm::rotate(glm::translate(glm::mat4(1), position),
            0.16f * std::sin(phase), glm::vec3(0, 0, 1));
        const float radius = balloon.radius;
        shader.vector("tint", balloon.color);
        shader.matrix("model", glm::scale(pose, radius * glm::vec3(1.0f, 1.25f, 1.0f)));
        sphere.draw();

        const glm::mat4 knot = glm::translate(pose, glm::vec3(0, -1.28f * radius, 0));
        shader.matrix("model", glm::scale(knot, radius * glm::vec3(0.12f, 0.15f, 0.12f)));
        sphere.draw();

        const float halfString = 1.1f * radius;
        const glm::mat4 stringPose = glm::rotate(knot, 0.24f * std::sin(phase - 0.8f), glm::vec3(0, 0, 1));
        shader.vector("tint", glm::vec3(0.62f, 0.60f, 0.56f));
        shader.matrix("model", glm::scale(glm::translate(stringPose, glm::vec3(0, -halfString, 0)),
            glm::vec3(0.009f, halfString, 0.009f)));
        sphere.draw();
    }
    glFrontFace(GL_CCW);
}

void run(GLFWwindow* window, const std::string& modelPath, const std::string& captureDirectory) {
    const std::string root = PROJECT_ROOT;
    Shader shader(root + "/code/shaders/phong.vert", root + "/code/shaders/phong.frag");
    Sky sky(root);
    ShadowMap shadows;
    ReflectionMap reflectionBackground;
    ReflectionMap reflectionMap;
    const glm::vec3 reflectionCenter = vector(animation::reflectionBall);
    const glm::mat4 reflectionProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 24000.0f);
    constexpr float lightingSceneScale = 6.0f;
    const glm::vec3 sunlight(-45, 80, 43);
    const glm::vec3 shadowCenter(0, 0, animation::paradeDepth);
    const glm::mat4 lightView = glm::lookAt(shadowCenter + sunlight * lightingSceneScale, shadowCenter, glm::vec3(0, 1, 0));
    const glm::mat4 lightProjection = glm::ortho(-72.0f * lightingSceneScale, 72.0f * lightingSceneScale,
        -52.0f * lightingSceneScale, 65.0f * lightingSceneScale, lightingSceneScale, 220.0f * lightingSceneScale);
    constexpr float roadLength = animation::paradeRoadLast - animation::paradeRoadFirst;
    const glm::vec3 roadCenter((animation::paradeRoadFirst + animation::paradeRoadLast) * 0.5f, 0.08f, animation::paradeDepth);
    const glm::vec3 roadSize(roadLength + 120.0f, 0.08f, animation::paradeRoadWidth);
    std::cout << "Loading OBJ: " << modelPath << std::endl;
    const bool coreScene = modelPath.find("tiananmen_core.obj") != std::string::npos;
    const bool fullScene = coreScene || modelPath.find("tiananmen_balanced.obj") != std::string::npos
        || modelPath.find("Tiananmen+Square") != std::string::npos;
    Mesh gate(modelPath, fullScene, fullScene ? -8.0f * 7877.01f / 108.0f : 0.0f);
    std::unique_ptr<Mesh> monument;
    glm::mat4 monumentModel(1.0f);
    glm::mat4 gateModel = placement(gate, {0, 0, -3}, 18.0f);
    if (fullScene) {
        gateModel = glm::translate(glm::mat4(1), glm::vec3(0, 0, -3));
        gateModel = glm::scale(gateModel, glm::vec3(108.0f / 7877.01f));
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
    std::cout << "Loading /model/missile_launcher/KJ-S1_vehicle.obj" << std::endl;
    Mesh launcher(root + "/model/missile_launcher/KJ-S1_vehicle.obj");
    {
        const auto extent = launcher.maximum() - launcher.minimum();
        std::cout << "Launcher extent: " << extent.x << ", " << extent.y << ", " << extent.z << std::endl;
    }
    std::cout << "Loading /model/nuclear_launcher/launcher.obj" << std::endl;
    Mesh nuclearLauncher(root + "/model/nuclear_launcher/launcher.obj");
    {
        const auto extent = nuclearLauncher.maximum() - nuclearLauncher.minimum();
        std::cout << "Nuclear launcher extent: " << extent.x << ", " << extent.y << ", " << extent.z << std::endl;
    }
    Mesh aircraft(root + "/model/j20/j-20_stealth_fighter_aircraft.obj");
    Mesh soldier(root + "/model/solider/source/\xe6\x9c\xaa\xe5\x91\xbd\xe5\x90\x8d.obj");
    Mesh cube(root + "/model/demo/cube.obj");
    Mesh ground(12000.0f, 64);
    Mesh roadPlane(1.0f);
    Mesh flag(14.4f, 9.6f, 64, 32);
    Mesh smokeQuad(1.0f, 1.0f, 1, 1);
    Mesh foliage(1.0f, 10, 7);
    Mesh treeMesh(root + "/model/tree/tree.obj");
    Mesh balloonSphere(1.0f, 18, 12);
    Mesh metalSphere(1.0f, 40, 28);
    Mesh postCylinder(1.0f, 1.0f, 32);
    const auto balloons = makeBalloons();
    Texture white(1);
    Texture paving(root + "/model/tile.jpeg");
    Texture asphalt(root + "/model/road.jpg");
    const auto drawVehicles = [&](float seconds) {
        for (int column = 0; column < 4; ++column) {
            const float depth = animation::paradeDepth + (column - 1.5f) * 6.0f;
            for (int index = 0; index < 8; ++index) {
                const auto& tank = *tanks[index % tanks.size()];
                const float horizontal = animation::tankPosition(seconds, index);
                const auto extent = tank.maximum() - tank.minimum();
                const float localSpeed = animation::tankSpeed * std::max({extent.x, extent.y, extent.z}) / 6.0f;
                shader.vector("animationState", glm::vec3(seconds, 0, localSpeed));
                shader.matrix("model", placement(tank, {horizontal, 0.08f, depth}, 6.0f, glm::radians(180.0f)));
                tank.draw(shader, white.handle, glm::vec3(1));
            }
            shader.vector("animationState", glm::vec3(seconds, 0, 0));
            shader.matrix("model", placement(launcher, {animation::tankPosition(seconds, 8), 0.08f, depth},
                10.0f, glm::radians(90.0f)));
            launcher.draw(shader, white.handle, glm::vec3(1));
            shader.matrix("model", placement(nuclearLauncher, {animation::tankPosition(seconds, 9), 0.08f, depth},
                12.0f, glm::radians(180.0f)));
            nuclearLauncher.draw(shader, white.handle, glm::vec3(1));
        }
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
    };
    std::cout << "Ready. Space: pause, R: replay, arrows: shots, B: bump toggle, Esc: exit" << std::endl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, white.handle);
    glActiveTexture(GL_TEXTURE0);
    double previous = glfwGetTime();
    float seconds = 0.0f;
    bool paused = false;
    bool previousSpace = false;
    bool previousLeft = false, previousRight = false, previousBump = false;
    bool bump = true;
    bool reflectionBackgroundReady = false;
    const std::array<float, 22> captureTimes = {4.8f, 4.9f, 5.0f, 5.3f, 7.6f, 10, 12, 14.4f, 15.5f,
        animation::marchStart, 18.3f, animation::marchLiftStart, 19.4f, animation::marchPanMid,
        21.4f, animation::aircraftTrack, 24, 26, balloonRelease, 32, 35.5f, animation::duration};
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
        const bool reflectionShot = seconds >= animation::reflectionStart && seconds < animation::marchStart;
        const int backgroundPasses = reflectionBackgroundReady ? 0 : ReflectionMap::faceCount;
        const int reflectionPasses = reflectionShot ? ReflectionMap::faceCount : 0;
        for (int renderPass = 0; renderPass < 2 + backgroundPasses + reflectionPasses; ++renderPass) {
        const bool depthPass = renderPass == 0;
        const bool backgroundPass = renderPass > 0 && renderPass <= backgroundPasses;
        const bool movingReflectionPass = renderPass > backgroundPasses && renderPass <= backgroundPasses + reflectionPasses;
        const bool reflectionPass = backgroundPass || movingReflectionPass;
        const int face = backgroundPass ? renderPass - 1 : renderPass - backgroundPasses - 1;
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthPass ? white.handle : shadows.texture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, movingReflectionPass ? reflectionBackground.texture : reflectionMap.texture);
        glActiveTexture(GL_TEXTURE0);
        if (reflectionPass) {
            if (backgroundPass) reflectionBackground.bindFace(face);
            else reflectionMap.copyFace(reflectionBackground, face);
            glViewport(0, 0, ReflectionMap::resolution, ReflectionMap::resolution);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, depthPass ? shadows.framebuffer : 0);
            glViewport(0, 0, depthPass ? ShadowMap::resolution : width, depthPass ? ShadowMap::resolution : height);
        }
        glClearColor(0.46f, 0.70f, 0.88f, 1.0f);
        if (!movingReflectionPass) glClear(depthPass ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto shot = animation::shot(seconds);
        const glm::vec3 camera = reflectionPass ? reflectionCenter : vector(shot.camera);
        const glm::vec3 target = vector(shot.target);
        const float aspect = static_cast<float>(width) / height;
        const glm::mat4 view = reflectionPass ? ReflectionMap::faceView(camera, face)
            : glm::lookAt(camera, target, glm::vec3(0, 1, 0));
        const float marchPan = glm::smoothstep(animation::marchLiftStart, animation::aircraftTrack, seconds);
        const float shotFov = seconds >= animation::marchStart && seconds < animation::aircraftTrack
            ? glm::mix(glm::radians(30.0f), verticalFov, marchPan) : verticalFov;
        const glm::mat4 projection = reflectionPass ? reflectionProjection
            : glm::perspective(reflectionShot ? glm::radians(30.0f) : shotFov, aspect, 0.1f, 24000.0f);
        if (!depthPass && !movingReflectionPass) sky.draw(view, projection);
        shader.use();
        shader.matrix("view", depthPass ? lightView : view);
        shader.matrix("projection", depthPass ? lightProjection : projection);
        shader.matrix("lightSpace", lightProjection * lightView);
        shader.integer("depthOnly", depthPass ? 1 : 0);
        shader.integer("shadowMap", 2);
        shader.integer("receiveShadows", backgroundPass ? 0 : 1);
        shader.integer("unlit", 0);
        shader.integer("metalSurface", 0);
        shader.integer("reflectionMap", 3);
        shader.vector("cameraPosition", camera);
        shader.vector("lightPosition", glm::normalize(sunlight));
        shader.floating("lightingSceneScale", lightingSceneScale);
        shader.integer("diffuseMap", 0);
        shader.integer("skyEnvironmentMap", 1);
        shader.vector("paradeRoadCenter", roadCenter);
        shader.vector("paradeRoadSize", roadSize);
        shader.floating("sourceRoadPadding", 0.0f);
        shader.integer("asphaltSurface", 0);
        shader.integer("bumpEnabled", bump && !backgroundPass ? 1 : 0);
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        shader.vector("surfaceState", glm::vec3(0));
        if (movingReflectionPass) {
            drawVehicles(seconds);
            continue;
        }
        shader.vector("surfaceState", glm::vec3(bump && !backgroundPass ? 1 : 0, 0, 0));
        // A large dark base plane fills any holes in the source scene's terrain
        // (between the gate and the buildings behind it, and beyond the plaza).
        shader.integer("pavingSurface", 0);
        shader.matrix("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.55f, 0.0f)));
        ground.draw(shader, white.handle, glm::vec3(0.30f, 0.32f, 0.28f), 0);
        shader.matrix("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.02f, 0.0f)));
        ground.draw(shader, paving.handle, glm::vec3(0.92f), paving.handle);
        shader.vector("surfaceState", glm::vec3(0));
        shader.matrix("model", gateModel);
        shader.floating("sourceRoadPadding", 1.5f);
        gate.draw(shader, white.handle, glm::vec3(1), paving.handle);
        shader.floating("sourceRoadPadding", 0.0f);
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
            const float sceneScale = 108.0f / 7877.01f;
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
        // The asphalt is the road's only top surface. A nearly coplanar black box
        // underneath it caused depth-buffer fighting in distant approach views.
        shader.integer("asphaltSurface", 1);
        shader.matrix("model", glm::scale(glm::translate(glm::mat4(1),
            glm::vec3(roadCenter.x, roadCenter.y + roadSize.y * 0.5f, roadCenter.z)),
            glm::vec3(roadSize.x * 0.5f, 1, roadSize.z * 0.5f)));
        roadPlane.draw(shader, asphalt.handle, glm::vec3(0.9f));
        shader.integer("asphaltSurface", 0);
        for (int stripe = -34; stripe <= 12; ++stripe)
            box({stripe * 2.5f, 0.13f, animation::paradeDepth + 2.2f}, {1.2f, 0.015f, 0.08f}, {0.92f, 0.86f, 0.6f});
        const auto postSection = [&](glm::vec3 base, float radius, float height, glm::vec3 color) {
            shader.matrix("model", glm::scale(glm::translate(glm::mat4(1), base), {radius, height, radius}));
            glBindTexture(GL_TEXTURE_2D, white.handle);
            shader.vector("tint", color);
            postCylinder.draw();
        };
        for (int side : {-1, 1}) {
            const float edge = animation::paradeDepth + side * animation::roadCurbOffset;
            box({roadCenter.x, 0.075f, edge + side * 0.55f}, {roadLength, 0.06f, 0.9f}, {0.62f, 0.64f, 0.62f});
            for (int section = static_cast<int>(animation::paradeRoadFirst) + 1;
                 section < animation::paradeRoadLast; ++section) {
                const float horizontal = static_cast<float>(section);
                box({horizontal, 0.13f, edge}, {0.97f, 0.16f, 0.28f},
                    section % 2 == 0 ? glm::vec3(0.71f, 0.73f, 0.70f) : glm::vec3(0.62f, 0.65f, 0.64f));
            }
            for (int drain = -10; drain <= 5; ++drain) {
                for (int slot = 0; slot < 6; ++slot)
                    box({drain * 8.0f + slot * 0.08f, 0.13f, edge - side * 0.22f},
                        {0.035f, 0.012f, 0.18f}, {0.12f, 0.15f, 0.16f});
            }
            for (int post = -10; post <= 5; ++post) {
                const float horizontal = post * 8.0f;
                const bool reflectionPost = side == 1 && horizontal == animation::reflectionBall.x;
                const float depth = animation::paradeDepth + side * animation::roadPostOffset;
                const glm::vec3 center(horizontal, animation::reflectionBall.y, depth);
                if (backgroundPass && reflectionPost) continue;
                postSection({horizontal, 0.055f, depth}, 0.18f, 0.08f, {0.29f, 0.32f, 0.33f});
                postSection({horizontal, 0.10f, depth}, 0.145f, 0.60f, {0.27f, 0.31f, 0.32f});
                constexpr float capBase = 0.69f;
                postSection({horizontal, capBase, depth}, 0.165f, animation::roadPostCapTop - capBase,
                    {0.78f, 0.74f, 0.57f});
                // One hero post carries a polished metal ball that reflects the environment.
                if (!reflectionPass) {
                    shader.integer("metalSurface", reflectionShot && reflectionPost ? 2 : 1);
                    shader.matrix("model", glm::scale(glm::translate(glm::mat4(1), center), glm::vec3(animation::roadBallRadius)));
                    glFrontFace(GL_CW);
                    metalSphere.draw();
                    glFrontFace(GL_CCW);
                    shader.integer("metalSurface", 0);
                }
            }
        }
        const auto crown = [&](glm::vec3 position, glm::vec3 size, glm::vec3 color) {
            shader.matrix("model", glm::scale(glm::translate(glm::mat4(1), position), size));
            shader.vector("tint", color);
            glFrontFace(GL_CW);
            foliage.draw();
            glFrontFace(GL_CCW);
        };
        for (int side : {-1, 1}) {
            for (int cluster = -37; cluster <= 22; ++cluster) {
                const float horizontal = cluster * 13.0f;
                // Leave gaps at the approach turn and at the infantry-to-aircraft pan.
                if (cluster == 0 || (side == 1 && horizontal > -136.0f && horizontal < -65.0f)) continue;
                const float depth = animation::paradeDepth + side * (animation::roadCurbOffset + 5.0f);
                const float size = 0.60f + 0.07f * (0.5f + 0.5f * std::sin(cluster * 2.37f + side));
                box({horizontal, 0.20f, depth}, {2.8f, 0.30f, 1.8f}, {0.62f, 0.65f, 0.62f});
                box({horizontal, 0.37f, depth}, {2.55f, 0.06f, 1.55f}, {0.22f, 0.24f, 0.16f});
                // Real tree model (trunk + branches + leaves), base planted in the planter.
                shader.matrix("model", placement(treeMesh, {horizontal, 0.35f, depth}, 11.4f * size));
                treeMesh.draw(shader, white.handle, glm::vec3(1));
                for (int shrub : {-1, 1}) {
                    crown({horizontal + shrub * 0.9f, 0.60f, depth + 0.3f}, {0.36f, 0.28f, 0.36f}, {0.32f, 0.46f, 0.23f});
                    for (int bloom = 0; bloom < 3; ++bloom)
                        crown({horizontal + shrub * 0.9f + (bloom - 1) * 0.15f, 0.82f, depth + 0.35f},
                            {0.08f, 0.06f, 0.08f}, {0.78f, 0.24f + bloom * 0.12f, 0.24f});
                }
            }
        }
        if (backgroundPass) continue;
        drawVehicles(seconds);
        // 步兵方阵：24 列 × 16 排，扩大后的阅兵方阵。
        for (int row = 0; row < 24; ++row) {
            for (int column = 0; column < 16; ++column) {
                const float horizontal = animation::soldierPosition(seconds, row);
                const float depth = animation::paradeDepth + (column - 7.5f) * 1.1f;
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
        box({animation::flagPoleX, 21.9f + animation::flagBaseOffset, animation::flagPoleZ},
            {0.39f, 43.2f, 0.39f}, {0.7f, 0.72f, 0.75f});
        shader.matrix("model", glm::translate(glm::mat4(1),
            glm::vec3(animation::flagPoleX, animation::flagHeight(seconds), animation::flagPoleZ)));
        shader.vector("animationState", glm::vec3(seconds, 1, 0));
        shader.vector("surfaceState", glm::vec3(0, 1, 0));
        flag.draw(shader, white.handle, glm::vec3(1));
        shader.vector("animationState", glm::vec3(seconds, 0, 0));
        shader.vector("surfaceState", glm::vec3(0));
        if (!depthPass && seconds >= balloonRelease)
            drawBalloons(balloons, seconds, aspect, balloonSphere, shader, white.handle);
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
                {glm::vec3(-84, 12, -60), glm::vec3(-78, 72, -54),  glm::vec3(1.0f, 0.80f, 0.30f), 3.6f, 0.0f, 1.3f, 36.0f, 26},
                {glm::vec3(-36, 12, -78),  glm::vec3(-30, 54, -72),   glm::vec3(1.0f, 0.30f, 0.25f), 2.9f, 0.7f, 1.0f, 27.0f, 20},
                {glm::vec3(12, 12, -90),   glm::vec3(18, 78, -84),   glm::vec3(0.35f, 0.60f, 1.0f), 4.3f, 1.5f, 1.5f, 45.0f, 30},
                {glm::vec3(72, 12, -48),   glm::vec3(78, 60, -42),   glm::vec3(0.30f, 0.95f, 0.40f), 3.2f, 2.2f, 1.1f, 30.0f, 22},
                {glm::vec3(-54, 12, -42),   glm::vec3(-48, 66, -36),   glm::vec3(1.0f, 0.40f, 0.65f), 3.8f, 1.1f, 1.4f, 39.0f, 28},
                {glm::vec3(48, 12, -96),   glm::vec3(54, 48, -90),    glm::vec3(1.0f, 0.90f, 0.35f), 2.6f, 1.9f, 0.9f, 21.0f, 16},
            }};
            const glm::vec3 fwForward = glm::normalize(target - camera);
            const glm::vec3 fwRight = glm::normalize(glm::cross(fwForward, glm::vec3(0, 1, 0)));
            const glm::vec3 fwUp = glm::cross(fwRight, fwForward);
            const auto finaleFraming = animation::shot(animation::flightEnd - 0.75f);
            const glm::vec3 finaleEye = vector(finaleFraming.camera);
            glm::vec3 finaleForward = vector(finaleFraming.target) - finaleEye;
            finaleForward.y = 0;
            finaleForward = glm::normalize(finaleForward);
            const glm::vec3 finaleRight = glm::cross(finaleForward, glm::vec3(0, 1, 0));
            const float finaleBlend = glm::smoothstep(balloonRelease - 2.0f, balloonRelease, seconds);
            const auto puff = [&](const glm::vec3& pos, float size, const glm::vec3& color, float alpha) {
                shader.vector("tint", color);
                shader.floating("smokeAlpha", alpha);
                glm::mat4 b(1.0f);
                b[0] = glm::vec4(fwRight * size * 6.0f, 0.0f);
                b[1] = glm::vec4(fwUp * size * 6.0f, 0.0f);
                b[2] = glm::vec4(fwForward, 0.0f);
                const glm::vec3 finalePosition = finaleEye + finaleForward * (210.0f - pos.z * 0.25f)
                    + finaleRight * (pos.x * 1.5f)
                    + glm::vec3(0, pos.y + 32.0f + 25.0f * std::max(seconds - animation::flightEnd, 0.0f), 0);
                b[3] = glm::vec4(glm::mix(pos, finalePosition, finaleBlend), 1.0f);
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
                    const int particles = fw.streaks * 4;
                    for (int particle = 0; particle < particles; ++particle) {
                        const float seed = 0.5f + 0.5f * std::sin(particle * 127.1f + fw.offset * 311.7f);
                        const float lifetime = burst * (0.65f + 0.35f * seed);
                        if (age >= lifetime) continue;
                        const float vertical = 1.0f - 2.0f * (particle + 0.5f) / particles;
                        const float horizontal = std::sqrt(std::max(0.0f, 1.0f - vertical * vertical));
                        const float angle = particle * 2.399963f + fw.offset;
                        const glm::vec3 direction(horizontal * std::cos(angle), vertical, horizontal * std::sin(angle));
                        const bool innerShell = particle % 4 == 0;
                        const glm::vec3 velocity = direction * (fw.radius * (0.85f + 0.3f * seed)
                            * (innerShell ? 0.58f : 1.0f));
                        const auto trajectory = [&](float elapsed) {
                            return fw.peak + velocity * ((1.0f - std::exp(-1.15f * elapsed)) / 1.15f)
                                + glm::vec3(0.72f * elapsed, -7.8f * elapsed * elapsed, 0);
                        };
                        const float remaining = 1.0f - age / lifetime;
                        const float flicker = 0.85f + 0.15f * std::sin(age * 35.0f + particle * 2.7f);
                        const float alpha = std::pow(remaining, 0.7f) * flicker;
                        const glm::vec3 shellColor = innerShell ? glm::vec3(1.0f, 0.82f, 0.42f) : fw.color;
                        const glm::vec3 color = glm::mix(glm::vec3(1.0f, 0.48f, 0.12f), shellColor, remaining);
                        for (int sample = 14; sample >= 1; --sample) {
                            const float history = age - sample * 0.014f;
                            if (history < 0.0f) continue;
                            const float fade = 1.0f - sample / 15.0f;
                            puff(trajectory(history), 0.04f + 0.07f * fade, color, alpha * fade * 0.28f);
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
                const glm::vec3 offset = -std::abs(wing) * 12.0f * forward + wing * 10.0f * right;
                glm::mat4 flight = glm::translate(glm::mat4(1), point + offset);
                flight = glm::rotate(flight, yaw, glm::vec3(0, 1, 0));
                flight = glm::rotate(flight, pitch, glm::vec3(1, 0, 0));
                flight = glm::rotate(flight, glm::radians(60.0f), glm::vec3(0, 1, 0));
                shader.matrix("model", flight * placement(aircraft, glm::vec3(0), 16.0f));
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
                    glm::vec3 mid = (trailStart + trailEnd) * 0.5f - trailForward * 6.0f;
                    mid += glm::vec3(1.5f * smokeAge, 0.55f * smokeAge, 0);
                    mid += glm::vec3(std::sin(emission * 4.1f + wing), std::sin(emission * 3.3f),
                        std::cos(emission * 3.7f + wing)) * (0.95f * smokeAge);
                    const float puffWidth = 1.0f + 3.2f * std::sqrt(smokeAge);
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
        const float fade = animation::finaleFade(seconds);
        if (fade > 0.0f) {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            shader.matrix("view", glm::mat4(1));
            shader.matrix("projection", glm::mat4(1));
            shader.matrix("model", glm::scale(glm::translate(glm::mat4(1), glm::vec3(-1, -1, 0)), glm::vec3(2, 2, 1)));
            shader.integer("unlit", 4);
            shader.vector("tint", glm::vec3(0));
            shader.floating("smokeAlpha", fade);
            glBindTexture(GL_TEXTURE_2D, white.handle);
            smokeQuad.draw();
            shader.integer("unlit", 0);
            shader.floating("smokeAlpha", 1);
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
        }
        const auto shot = animation::shot(seconds);
        const char* names[] = {"Approach", "Armor", "Reflection", "March", "Flypast", "Finale"};
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
        if (!reflectionBackgroundReady) {
            reflectionBackgroundReady = true;
            previous = glfwGetTime();
        }
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
