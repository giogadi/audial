#version 330 core

in vec2 texCoord;
in vec3 fragPos;
in vec3 normal;

uniform sampler2D uMyTexture;
uniform vec3 uLightPos;
uniform vec3 uAmbient;
uniform vec3 uDiffuse;

out vec4 FragColor;

void main() {
    vec4 albedo = texture(uMyTexture, texCoord);
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(uLightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff*uDiffuse;

    FragColor = vec4((uAmbient + diffuse), 1.0) * albedo;
}