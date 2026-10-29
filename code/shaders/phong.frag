#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
uniform sampler2D diffuseMap;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform vec3 tint;
out vec4 fragmentColor;
void main() {
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(lightPosition - worldPosition);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);
    vec3 reflected = reflect(-lightDirection, normal);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(viewDirection, reflected), 0.0), 32.0) : 0.0;
    vec3 albedo = texture(diffuseMap, uv).rgb * tint;
    vec3 color = albedo * (0.24 + 0.76 * diffuse) + vec3(1.0, 0.9, 0.7) * 0.35 * specular;
    fragmentColor = vec4(color, 1.0);
}