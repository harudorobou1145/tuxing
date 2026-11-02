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
mat3 rotateX(float angle) {
    float cosine = cos(angle), sine = sin(angle);
    return mat3(1, 0, 0, 0, cosine, sine, 0, -sine, cosine);
}
mat3 rotateZ(float angle) {
    float cosine = cos(angle), sine = sin(angle);
    return mat3(cosine, sine, 0, -sine, cosine, 0, 0, 0, 1);
}
void main() {
    vec3 position = aPosition;
    vec3 normal = aNormal;
    if (animationState.y < -0.5) {
        float sideWeight = sign(aPosition.x);
        vec2 limbWeights = sideWeight * vec2(
            smoothstep(0.20, 0.32, abs(aPosition.x)) * smoothstep(1.12, 1.30, aPosition.y),
            (1.0 - smoothstep(0.76, 0.94, aPosition.y)) * smoothstep(0.015, 0.075, abs(aPosition.x)));
        float stride = sin(animationState.x * 7.0);
        if (abs(limbWeights.x) > 0.001) {
            float side = sign(limbWeights.x);
            vec3 shoulder = vec3(side * 0.21, 1.43, 0);
            mat3 rotation = rotateX(side * stride * 0.48) * rotateZ(-side * 1.35);
            position = mix(position, shoulder + rotation * (position - shoulder), abs(limbWeights.x));
            normal = normalize(mix(normal, rotation * normal, abs(limbWeights.x)));
        }
        if (abs(limbWeights.y) > 0.001) {
            float side = sign(limbWeights.y);
            vec3 hip = vec3(side * 0.10, 0.90, 0);
            mat3 rotation = rotateX(-side * stride * 0.52);
            position = mix(position, hip + rotation * (position - hip), abs(limbWeights.y));
            normal = normalize(mix(normal, rotation * normal, abs(limbWeights.y)));
        }
        position.y += 0.045 * abs(stride);
    }
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