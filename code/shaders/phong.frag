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
uniform sampler2D shadowMap;
uniform mat4 lightSpace;
uniform int depthOnly;
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
    if (depthOnly == 1) {
        fragmentColor = vec4(1.0);
        return;
    }
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
    vec3 lightDirection = normalize(lightPosition);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);
    vec3 reflected = reflect(-lightDirection, normal);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec4 lightClip = lightSpace * vec4(worldPosition, 1.0);
    vec3 shadowPosition = lightClip.xyz / lightClip.w * 0.5 + 0.5;
    float shadow = 0.0;
    if (all(greaterThan(shadowPosition, vec3(0.0))) && all(lessThan(shadowPosition, vec3(1.0)))) {
        float bias = max(0.0012 * (1.0 - diffuse), 0.0003);
        vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
        for (int offsetY = -1; offsetY <= 1; ++offsetY) {
            for (int offsetX = -1; offsetX <= 1; ++offsetX) {
                float storedDepth = texture(shadowMap, shadowPosition.xy + vec2(offsetX, offsetY) * texel).r;
                shadow += shadowPosition.z - bias > storedDepth ? 1.0 / 9.0 : 0.0;
            }
        }
    }
    float visibility = 1.0 - shadow * 0.78;
    float shininess = roofSurface == 1 ? 48.0 : pavingSurface == 1 ? 12.0 : 32.0;
    float specular = diffuse > 0.0 ? pow(max(dot(viewDirection, reflected), 0.0), shininess) : 0.0;
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
    float skyExposure = normal.y * 0.5 + 0.5;
    vec3 ambientLight = mix(vec3(0.21, 0.205, 0.19), vec3(0.32, 0.36, 0.41), skyExposure);
    float highlightStrength = roofSurface == 1 ? 0.24 : pavingSurface == 1 ? 0.035 : 0.12;
    if (trackSurface == 1) highlightStrength = 0.045;
    vec3 color = albedo * (ambientLight + vec3(0.79, 0.76, 0.70) * diffuse * visibility)
        + vec3(1.0, 0.96, 0.86) * highlightStrength * specular * visibility;
    if (surfaceState.y > 0.5) {
        float weave = sin(uv.x * 1200.0) * sin(uv.y * 800.0);
        float weaveVisibility = 1.0 - smoothstep(0.001, 0.006, max(fwidth(uv.x), fwidth(uv.y)));
        float foldShade = 0.78 + 0.22 * abs(normal.z);
        float transmission = pow(max(dot(-lightDirection, viewDirection), 0.0), 3.0);
        float backLight = max(dot(-normal, lightDirection), 0.0);
        float fabricSheen = pow(1.0 - abs(dot(normal, viewDirection)), 4.0);
        color = albedo * (0.20 + 0.74 * diffuse * visibility) * foldShade
            * (1.0 + 0.035 * weave * weaveVisibility)
            + albedo * vec3(1.0, 0.35, 0.22) * (0.28 * backLight + 0.22 * transmission)
            + vec3(1.0, 0.65, 0.45) * (0.10 * specular + 0.10 * fabricSheen);
    }
    if (unlit == 0) {
        vec3 direction = normalize(worldPosition - cameraPosition);
        const float pi = 3.14159265359;
        vec2 skyUV = vec2(atan(direction.z, direction.x) / (2.0 * pi) + 0.5,
                          asin(clamp(direction.y, -1.0, 1.0)) / pi + 0.5);
        float distanceToCamera = length(worldPosition - cameraPosition);
        float opticalDepth = max(distanceToCamera - 38.0, 0.0) * 0.0028;
        float haze = 1.0 - exp(-opticalDepth);
        if (pavingSurface == 1)
            haze = max(haze, smoothstep(90.0, 280.0, length(worldPosition.xz - cameraPosition.xz)));
        color = mix(color, texture(skyEnvironmentMap, skyUV).rgb, haze);
    }
    if (unlit == 1) {
        float radial = 1.0 - smoothstep(0.35, 1.0, distance(uv, vec2(0.5)) * 2.0);
        fragmentColor = vec4(albedo, smokeAlpha * radial);
    } else {
        fragmentColor = vec4(color, 1.0);
    }
}
