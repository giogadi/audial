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

uniform vec4 uColor;
uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;

out vec4 FragColor;

void main() {
    vec4 albedo = uColor * vec4(10.f/255.f, 20.f/255.f, 80.f/255.f, 1.f);
    vec3 norm = normalize(texture(uNormalMap, texCoord).xzy * 2 - 1);
    norm.z = -norm.z;
    
    float diff = max(dot(norm, -uDirLight._dir), 0.0);
    vec3 diffuse = diff*uDirLight._diffuse * vec3(albedo);

    // vec3 ambient = uDirLight._ambient;
    vec3 ambient = vec3(0.17f, 0.17f, 0.17f);

    FragColor = vec4(ambient + diffuse, 1.0);
}
