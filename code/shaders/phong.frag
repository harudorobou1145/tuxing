#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
in vec3 localPosition;
uniform int trackSurface;
uniform vec3 animationState;
uniform sampler2D diffuseMap;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform vec3 tint;
uniform vec3 surfaceState;
out vec4 fragmentColor;
float star(vec2 point, vec2 center, float radius, float rotation) {
    vec2 offset = point - center;
    float sector = 3.14159265 / 5.0;
    float angle = mod(atan(offset.y, offset.x) - rotation + sector, 2.0 * sector) - sector;
    float edge = radius * 0.381966 / (cos(angle) * 0.381966 + abs(sin(angle)) * 1.175571);
    return 1.0 - smoothstep(edge - 0.004, edge + 0.004, length(offset));
}
void main() {
    vec3 normal = normalize(worldNormal);
    if (surfaceState.x > 0.5) {
        vec3 positionX = dFdx(worldPosition), positionY = dFdy(worldPosition);
        vec3 crossX = cross(positionY, normal), crossY = cross(normal, positionX);
        float determinant = dot(positionX, crossX);
        float height = texture(diffuseMap, uv).r;
        vec3 gradient = sign(determinant) * (dFdx(height) * crossX + dFdy(height) * crossY);
        normal = normalize(abs(determinant) * normal - 0.12 * gradient);
    }
    if (!gl_FrontFacing) normal = -normal;
    vec3 lightDirection = normalize(lightPosition - worldPosition);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);
    vec3 reflected = reflect(-lightDirection, normal);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(viewDirection, reflected), 0.0), 32.0) : 0.0;
    vec3 albedo = texture(diffuseMap, uv).rgb * tint;
    if (trackSurface == 1) {
        float loop = atan((localPosition.y - 1.35) / 1.35, localPosition.x / 6.2);
        float tread = smoothstep(0.18, 0.28, fract(loop * 12.0 - animationState.x * animationState.z * 12.0 / 6.2));
        albedo = mix(vec3(0.055), vec3(0.30, 0.32, 0.29), tread);
    }
    if (surfaceState.y > 0.5) {
        vec2 point = uv * vec2(3.0, 2.0);
        vec2 big = vec2(0.5, 1.5);
        float stars = star(point, big, 0.30, 1.5707963);
        vec2 centers[4] = vec2[](vec2(1.0, 1.8), vec2(1.2, 1.6), vec2(1.2, 1.3), vec2(1.0, 1.1));
        for (int index = 0; index < 4; ++index) {
            vec2 direction = big - centers[index];
            stars = max(stars, star(point, centers[index], 0.10, atan(direction.y, direction.x)));
        }
        albedo = mix(vec3(0.85, 0.025, 0.045), vec3(1.0, 0.85, 0.04), stars);
    }
    vec3 color = albedo * (0.24 + 0.76 * diffuse) + vec3(1.0, 0.9, 0.7) * 0.35 * specular;
    fragmentColor = vec4(color, 1.0);
}