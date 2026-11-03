#version 330 core
uniform mat4 inverseProjection;
uniform mat4 inverseView;
out vec3 skyDirection;
void main() {
    // Oversized fullscreen triangle; no geometry or camera translation needed.
    vec2 positions[3] = vec2[](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
    vec2 point = positions[gl_VertexID];
    vec4 viewRay = inverseProjection * vec4(point, 1, 1);
    skyDirection = mat3(inverseView) * viewRay.xyz;
    gl_Position = vec4(point, 1, 1);
}
