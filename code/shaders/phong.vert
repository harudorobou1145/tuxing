#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 animationState;
out vec3 worldPosition;
out vec3 worldNormal;
out vec2 uv;
out vec3 localPosition;
void main() {
    vec3 position = aPosition;
    vec3 normal = aNormal;
    if (animationState.y > 0.5) {
        float phase = position.x * 4.5 - animationState.x * 4.0 + position.y * 2.0;
        position.z += 0.16 * position.x * sin(phase);
        normal = normalize(vec3(-0.16 * sin(phase) - 0.72 * position.x * cos(phase),
                                -0.32 * position.x * cos(phase), 1.0));
    }
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    worldNormal = mat3(transpose(inverse(model))) * normal;
    uv = aUV;
    localPosition = aPosition;
    gl_Position = projection * view * world;
}