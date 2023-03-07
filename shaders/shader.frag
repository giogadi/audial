#version 330 core

in vec2 texCoord;
in vec3 fragPos;
in vec3 normal;

struct DirLight {
    vec3 _dir;
    vec3 _ambient;
    vec3 _diffuse;    
};
uniform DirLight uDirLight;

uniform sampler2D uMyTexture;

out vec4 FragColor;

void main() {
    vec4 albedo = texture(uMyTexture, texCoord);
    vec3 norm = normalize(normal);
    float diff = max(dot(norm, -uDirLight._dir), 0.0);
    vec3 diffuse = diff*uDirLight._diffuse;

    FragColor = vec4((uDirLight._ambient + diffuse), 1.0) * albedo;
}
