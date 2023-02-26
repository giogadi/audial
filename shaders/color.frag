#version 330 core

in vec3 fragPos;
in vec3 normal;

struct DirLight {
    vec3 _dir;
    vec3 _ambient;
    vec3 _diffuse;    
};
uniform DirLight uDirLight;

struct PointLight {
    vec3 _pos;
    vec3 _ambient;
    vec3 _diffuse;
};
#define NUM_POINT_LIGHTS 2
uniform PointLight uPointLights[NUM_POINT_LIGHTS];

uniform vec4 uColor;

out vec4 FragColor;

void main() {
    vec4 albedo = uColor;
    vec3 norm = normalize(normal);
    vec3 result = vec3(0, 0, 0);
    // Directional light
    {
        float diff = max(dot(norm, -uDirLight._dir), 0.0);
        vec3 diffuse = diff * uDirLight._diffuse;
        result += diffuse + uDirLight._ambient;
    }

    // Point lights
    for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
        PointLight light = uPointLights[i];        
        vec3 lightDir = normalize(light._pos - fragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff*light._diffuse;
        result += diffuse + light._ambient;
    }
    FragColor = vec4(result, 1.0) * albedo;
}