#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec4 uColor;
uniform vec3 uLightPos;
uniform vec3 uAmbient;
uniform vec3 uDiffuse;

out vec4 FragColor;

void main() {
    vec4 albedo = uColor;
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(uLightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff*uDiffuse;

    FragColor = vec4((uAmbient + diffuse), 1.0) * albedo;
}