#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
in vec3 localPosition;
uniform int trackSurface;
uniform int pavingSurface;
uniform int plazaLayout;
uniform vec3 paradeRoadCenter;
uniform vec3 paradeRoadSize;
uniform float sourceRoadPadding;
uniform int asphaltSurface;
uniform int roofSurface;
uniform int architectureSurface;
uniform int bumpEnabled;
uniform int unlit;
uniform int metalSurface;
uniform float smokeAlpha;
uniform vec3 animationState;
uniform sampler2D diffuseMap;
uniform sampler2D skyEnvironmentMap;
uniform samplerCube reflectionMap;
uniform sampler2D shadowMap;
uniform int receiveShadows;
uniform mat4 lightSpace;
uniform int depthOnly;
uniform vec3 lightPosition;
uniform float lightingSceneScale;
uniform vec3 cameraPosition;
uniform vec3 tint;
uniform vec3 surfaceState;
out vec4 fragmentColor;
vec3 skyColor(vec3 direction) {
    const float pi = 3.14159265359;
    vec2 skyUV = vec2(atan(direction.z, direction.x) / (2.0 * pi) + 0.5,
                      asin(clamp(direction.y, -1.0, 1.0)) / pi + 0.5);
    return texture(skyEnvironmentMap, skyUV).rgb;
}

float star(vec2 point, vec2 center, float radius, float rotation) {
    vec2 offset = point - center;
    float sector = 3.14159265 / 5.0;
    float angle = mod(atan(offset.y, offset.x) - rotation + sector, 2.0 * sector) - sector;
    float edge = radius * 0.381966 / (cos(angle) * 0.381966 + abs(sin(angle)) * 1.175571);
    return 1.0 - smoothstep(edge - 0.004, edge + 0.004, length(offset));
}
void main() {
    // Clear the uneven imported terrain beneath the road and its flat shoulders.
    if (pavingSurface == 1 && all(lessThan(abs(worldPosition.xz - paradeRoadCenter.xz),
        paradeRoadSize.xz * 0.5 + vec2(0.0, sourceRoadPadding))))
        discard;
    if (depthOnly == 1) {
        fragmentColor = vec4(1.0);
        return;
    }
    if (unlit == 4) {
        fragmentColor = vec4(0.0, 0.0, 0.0, smokeAlpha);
        return;
    }
    if (metalSurface != 0) {
        vec3 view = normalize(cameraPosition - worldPosition);
        vec3 direction = reflect(-view, normalize(worldNormal));
        vec3 environment;
        if (metalSurface == 2) {
            environment = texture(reflectionMap, direction).rgb;
        } else {
            environment = mix(vec3(0.24, 0.25, 0.25), skyColor(direction), smoothstep(-0.18, 0.08, direction.y));
        }
        fragmentColor = vec4(environment * vec3(0.94, 0.96, 0.98) + vec3(0.025), 1.0);
        return;
    }
    // World-space tiling keeps the exported plaza and generated ground consistent.
    vec2 surfaceUV = pavingSurface == 1 ? worldPosition.xz / 1.6 : uv;
    // One asphalt repeat spans 4.8 x 3 metres, matching road.jpg's aspect ratio.
    if (asphaltSurface == 1) surfaceUV = worldPosition.xz / vec2(4.8, 3.0);
    // Source model units per texture repeat. Larger values produce larger roof tiles.
    if (roofSurface == 1) surfaceUV = uv / 650.0;
    vec3 normal = normalize(worldNormal);
    if (surfaceState.x > 0.5 || ((pavingSurface == 1 || asphaltSurface == 1) && bumpEnabled == 1)) {
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
    if (receiveShadows == 1 && all(greaterThan(shadowPosition, vec3(0.0))) && all(lessThan(shadowPosition, vec3(1.0)))) {
        vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
        float bias = max(0.0012 * (1.0 - diffuse), 0.0003) / lightingSceneScale;
        // Compare neighboring samples against the receiver's plane, avoiding self-shadow stripes.
        vec3 shadowX = dFdx(shadowPosition), shadowY = dFdy(shadowPosition);
        float determinant = shadowX.x * shadowY.y - shadowX.y * shadowY.x;
        vec2 depthGradient = vec2(0.0);
        if (abs(determinant) > 1e-12) {
            depthGradient = vec2(shadowX.z * shadowY.y - shadowY.z * shadowX.y,
                                 shadowY.z * shadowX.x - shadowX.z * shadowY.x) / determinant;
        }
        vec2 sampleCenter = (floor(shadowPosition.xy / texel) + 0.5) * texel;
        for (int offsetY = -1; offsetY <= 1; ++offsetY) {
            for (int offsetX = -1; offsetX <= 1; ++offsetX) {
                vec2 sampleUV = sampleCenter + vec2(offsetX, offsetY) * texel;
                float storedDepth = texture(shadowMap, sampleUV).r;
                float receiverDepth = shadowPosition.z + dot(depthGradient, sampleUV - shadowPosition.xy);
                shadow += receiverDepth - bias > storedDepth ? 1.0 / 9.0 : 0.0;
            }
        }
    }
    float visibility = 1.0 - shadow * 0.78;
    float shininess = roofSurface == 1 ? 48.0 : (pavingSurface == 1 || asphaltSurface == 1) ? 12.0 : 32.0;
    float specular = diffuse > 0.0 ? pow(max(dot(viewDirection, reflected), 0.0), shininess) : 0.0;
    vec3 albedo = texture(diffuseMap, surfaceUV).rgb * tint;
    if (plazaLayout == 1 && pavingSurface == 1 && abs(worldPosition.x) < 118.0
        && worldPosition.z > 168.0 && worldPosition.z < 382.0) {
        vec2 point = worldPosition.xz;
        float stone = texture(diffuseMap, point / 2.4).r;
        albedo = vec3(0.67, 0.69, 0.68) * (0.88 + 0.18 * stone);
        if (abs(point.x) < 43.0)
            albedo = vec3(0.79, 0.80, 0.77) * (0.88 + 0.16 * stone);
        float axisBorder = abs(abs(point.x) - 43.0);
        float crossBand = min(abs(point.y - 174.0), min(abs(point.y - 258.0), abs(point.y - 370.0)));
        if (axisBorder < 0.7 || crossBand < 1.1)
            albedo = vec3(0.43, 0.47, 0.47);
        vec2 slab = abs(fract(point / vec2(6.0, 9.0)) - 0.5);
        vec2 filtering = max(fwidth(point / vec2(6.0, 9.0)), vec2(0.001));
        vec2 joint = smoothstep(vec2(0.492) - filtering, vec2(0.5), slab);
        albedo *= 1.0 - 0.10 * max(joint.x, joint.y);
        float lawnDepth = min(abs(point.y - 220.0), abs(point.y - 296.0));
        vec2 lawnOffset = vec2(abs(abs(point.x) - 71.0) - 18.0, lawnDepth - 30.0);
        float lawnEdge = max(lawnOffset.x, lawnOffset.y);
        if (lawnEdge < 1.2) {
            albedo = vec3(0.82, 0.81, 0.73) * (0.94 + 0.06 * stone);
            if (lawnEdge < 0.0) {
                float stripes = 0.5 + 0.5 * sin(point.y * 0.65);
                albedo = mix(vec3(0.24, 0.40, 0.22), vec3(0.31, 0.47, 0.26), stripes)
                    * (0.90 + 0.14 * stone);
                if (lawnEdge > -1.4)
                    albedo = vec3(0.55, 0.34, 0.29) * (0.90 + 0.12 * stone);
            }
        }
    }
    if (architectureSurface > 0) {
        vec2 stoneUV = abs(normal.x) > abs(normal.z) ? worldPosition.zy : worldPosition.xy;
        vec2 blockUV = stoneUV / vec2(0.85, 0.38);
        blockUV.x += mod(floor(blockUV.y), 2.0) * 0.5;
        vec2 edge = min(fract(blockUV), 1.0 - fract(blockUV));
        vec2 filtering = max(fwidth(blockUV), vec2(0.001));
        vec2 joints = 1.0 - smoothstep(vec2(0.012), vec2(0.012) + filtering, edge);
        float grain = sin(stoneUV.x * 83.0) * sin(stoneUV.y * 97.0);
        grain *= 1.0 - smoothstep(0.01, 0.06, max(fwidth(stoneUV.x), fwidth(stoneUV.y)));
        albedo = (architectureSurface == 2 ? vec3(0.80, 0.82, 0.79) : vec3(0.73, 0.72, 0.65))
            * (1.0 - 0.13 * max(joints.x, joints.y) + 0.025 * grain);
        if (architectureSurface == 1 && abs(normal.y) < 0.3) {
            vec2 windowUV = stoneUV / vec2(1.45, 1.6);
            vec2 cell = fract(windowUV);
            vec2 width = max(fwidth(windowUV), vec2(0.001));
            vec2 opening = smoothstep(vec2(0.24, 0.22), vec2(0.24, 0.22) + width, cell)
                * (1.0 - smoothstep(vec2(0.76, 0.82) - width, vec2(0.76, 0.82), cell));
            float windowMask = opening.x * opening.y * step(0.8, worldPosition.y) * (1.0 - step(3.9, worldPosition.y));
            vec3 glass = mix(vec3(0.12, 0.18, 0.20), vec3(0.29, 0.39, 0.43), cell.y);
            albedo = mix(albedo, glass, windowMask);
        }
    }
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
    float highlightStrength = roofSurface == 1 ? 0.24 : (pavingSurface == 1 || asphaltSurface == 1) ? 0.035 : 0.12;
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
        // Keep main's atmospheric falloff in the enlarged scene's coordinate system.
        float distanceToCamera = length(worldPosition - cameraPosition) / lightingSceneScale;
        float opticalDepth = max(distanceToCamera - 38.0, 0.0) * 0.0028;
        float haze = 1.0 - exp(-opticalDepth);
        if (pavingSurface == 1)
            haze = max(haze, smoothstep(90.0, 280.0,
                length(worldPosition.xz - cameraPosition.xz) / lightingSceneScale));
        color = mix(color, skyColor(direction), haze);
    }
    if (unlit >= 1) {
        float radial = 1.0 - smoothstep(0.35, 1.0, distance(uv, vec2(0.5)) * 2.0);
        if (unlit == 3) {
            float radius = length((uv - 0.5) * 2.0);
            radial = exp(-radius * radius * 7.0) * (1.0 - smoothstep(0.65, 1.0, radius));
        }
        if (unlit == 2) {
            float density = 0.80 + 0.12 * sin(uv.x * 19.0 + sin(uv.y * 23.0))
                + 0.08 * sin(uv.y * 37.0 + uv.x * 13.0);
            radial = exp(-dot((uv - 0.5) * vec2(3.0, 4.0), (uv - 0.5) * vec2(3.0, 4.0)))
                * (1.0 - smoothstep(0.35, 0.5, max(abs(uv.x - 0.5), abs(uv.y - 0.5)))) * density;
        }
        fragmentColor = vec4(albedo, smokeAlpha * radial);
    } else {
        fragmentColor = vec4(color, 1.0);
    }
}
