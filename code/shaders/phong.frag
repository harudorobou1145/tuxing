#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
in vec3 localPosition;
uniform int trackSurface;
uniform int pavingSurface;
uniform int roofSurface;
uniform int bumpEnabled;
uniform int unlit;
uniform float smokeAlpha;
uniform vec3 animationState;
uniform sampler2D diffuseMap;
uniform sampler2D skyEnvironmentMap;
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
    // World-space tiling keeps the exported plaza and generated ground consistent.
    vec2 surfaceUV = pavingSurface == 1 ? worldPosition.xz / 1.6 : uv;
    // Source model units per texture repeat. Larger values produce larger roof tiles.
    if (roofSurface == 1) surfaceUV = uv / 650.0;
    vec3 normal = normalize(worldNormal);
    if (surfaceState.x > 0.5 || (pavingSurface == 1 && bumpEnabled == 1)) {
        vec3 positionX = dFdx(worldPosition), positionY = dFdy(worldPosition);
        vec3 crossX = cross(positionY, normal), crossY = cross(normal, positionX);
        float determinant = dot(positionX, crossX);
        float height = texture(diffuseMap, surfaceUV).r;
        vec3 gradient = sign(determinant) * (dFdx(height) * crossX + dFdy(height) * crossY);
        normal = normalize(abs(determinant) * normal - 0.12 * gradient);
    }
    if (!gl_FrontFacing) normal = -normal;
    vec3 lightDirection = normalize(lightPosition - worldPosition);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);
    vec3 reflected = reflect(-lightDirection, normal);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(viewDirection, reflected), 0.0), 32.0) : 0.0;
    vec3 albedo = texture(diffuseMap, surfaceUV).rgb * tint;
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
    if (pavingSurface == 1) {
        // The exported terrain spans hundreds of scene units. Fade its remote
        // horizon into the same panorama instead of showing a dark backdrop band.
        vec3 direction = normalize(worldPosition - cameraPosition);
        const float pi = 3.14159265359;
        vec2 skyUV = vec2(atan(direction.z, direction.x) / (2.0 * pi) + 0.5,
                          asin(clamp(direction.y, -1.0, 1.0)) / pi + 0.5);
        float haze = smoothstep(90.0, 280.0, length(worldPosition.xz - cameraPosition.xz));
        color = mix(color, texture(skyEnvironmentMap, skyUV).rgb, haze);
    }
    if (surfaceState.y > 0.5) {
        float weave = sin(uv.x * 1200.0) * sin(uv.y * 800.0);
        float weaveVisibility = 1.0 - smoothstep(0.001, 0.006, max(fwidth(uv.x), fwidth(uv.y)));
        float foldShade = 0.78 + 0.22 * abs(normal.z);
        float transmission = pow(max(dot(-lightDirection, viewDirection), 0.0), 3.0);
        float backLight = max(dot(-normal, lightDirection), 0.0);
        float fabricSheen = pow(1.0 - abs(dot(normal, viewDirection)), 4.0);
        color = albedo * (0.20 + 0.74 * diffuse) * foldShade
            * (1.0 + 0.035 * weave * weaveVisibility)
            + albedo * vec3(1.0, 0.35, 0.22) * (0.28 * backLight + 0.22 * transmission)
            + vec3(1.0, 0.65, 0.45) * (0.10 * specular + 0.10 * fabricSheen);
    }
    if (unlit == 1) {
        float radial = 1.0 - smoothstep(0.35, 1.0, distance(uv, vec2(0.5)) * 2.0);
        fragmentColor = vec4(albedo, smokeAlpha * radial);
    } else {
        fragmentColor = vec4(color, 1.0);
    }
}
