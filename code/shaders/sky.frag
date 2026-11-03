#version 330 core
in vec3 skyDirection;
uniform sampler2D skyMap;
out vec4 fragmentColor;
void main() {
    vec3 direction = normalize(skyDirection);
    const float pi = 3.14159265359;
    vec2 skyUV = vec2(atan(direction.z, direction.x) / (2.0 * pi) + 0.5,
                      asin(clamp(direction.y, -1.0, 1.0)) / pi + 0.5);
    fragmentColor = vec4(texture(skyMap, skyUV).rgb, 1);
}
