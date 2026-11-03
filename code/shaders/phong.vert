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
        float stride = sin(animationState.x * 4.8);
        stride = sign(stride) * smoothstep(0.0, 0.85, abs(stride));
        if (abs(limbWeights.x) > 0.001) {
            float side = sign(limbWeights.x);
            vec3 shoulder = vec3(side * 0.24, 1.43, 0);
            float forwardSwing = smoothstep(-0.65, 0.65, side * stride);
            float elbowWeight = smoothstep(0.46, 0.58, abs(aPosition.x));
            vec3 elbow = vec3(side * 0.52, 1.43, 0);
            mat3 elbowRotation = rotateZ(side * forwardSwing * 1.45);
            vec3 bentPosition = mix(aPosition, elbow + elbowRotation * (aPosition - elbow), elbowWeight);
            vec3 bentNormal = normalize(mix(aNormal, elbowRotation * aNormal, elbowWeight));
            mat3 rotation = rotateX(mix(-0.32, 0.90, forwardSwing)) * rotateZ(-side * 1.45);
            position = mix(position, shoulder + rotation * (bentPosition - shoulder), abs(limbWeights.x));
            normal = normalize(mix(normal, rotation * bentNormal, abs(limbWeights.x)));
        }
        else if (abs(limbWeights.y) > 0.001) {
            float side = sign(limbWeights.y);
            vec3 hip = vec3(side * 0.10, 0.90, 0);
            float swing = side * stride;
            float angle = -max(swing, 0.0) * 0.58 - min(swing, 0.0) * 0.12;
            mat3 rotation = rotateX(angle);
            position = mix(position, hip + rotation * (position - hip), abs(limbWeights.y));
            normal = normalize(mix(normal, rotation * normal, abs(limbWeights.y)));
        }
        position.y += 0.025;
    }
    if (animationState.y > 0.5) {
        float time = animationState.x;
        float gust = 0.78 + 0.16 * sin(time * 1.3) + 0.06 * sin(time * 3.7);
        float phase = position.x * 4.5 - time * 4.0 + position.y * 2.0;
        float ripple = position.x * 13.0 - time * 8.5 + position.y * 5.0;
        float flutter = position.x * 23.0 - time * 13.0 - position.y * 7.0;
        float wave = 0.13 * sin(phase) + 0.025 * sin(ripple) + 0.009 * sin(flutter);
        float slopeX = gust * (wave + position.x * (0.585 * cos(phase)
            + 0.325 * cos(ripple) + 0.207 * cos(flutter)));
        float slopeY = gust * position.x * (0.26 * cos(phase)
            + 0.125 * cos(ripple) - 0.063 * cos(flutter));
        position.z += gust * position.x * wave;
        normal = normalize(vec3(-slopeX, -slopeY, 1.0));
    }
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    worldNormal = mat3(transpose(inverse(model))) * normal;
    uv = aUV;
    localPosition = aPosition;
    gl_Position = projection * view * world;
}