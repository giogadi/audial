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

struct PointLight {
    vec3 _pos;
    vec3 _ambient;
    vec3 _diffuse;
};
#define NUM_POINT_LIGHTS 2
uniform PointLight uPointLights[NUM_POINT_LIGHTS];

uniform sampler2D uMyTexture;

uniform vec4 uColor;
uniform vec3 uViewPos;

out vec4 FragColor;

void main() {
    float specularStrength = 0.5f;
    float attenConstant = 1.0f;
    float attenLinear = 0.35f;
    float attenQuad = 0.44;

    vec3 viewDir = normalize(uViewPos - fragPos);

    vec4 albedo = texture(uMyTexture, texCoord);
    albedo *= uColor;
    vec3 norm = normalize(normal);
    vec3 result = vec3(0, 0, 0);
    // Directional light
    {
        float diff = max(dot(norm, -uDirLight._dir), 0.0);
        vec3 diffuse = diff * uDirLight._diffuse;
        //vec3 reflectDir = reflect(-uDirLight._dir, normal);
        //float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
        //vec3 specular = specularStrength * uDirLight._diffuse;
        //result += diffuse + uDirLight._ambient + specular;
        result += diffuse + uDirLight._ambient;
    }

    // Point lights
    for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
        PointLight light = uPointLights[i];        
        vec3 lightDir = normalize(light._pos - fragPos);
               
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
        vec3 specular = specularStrength * light._diffuse;

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff*light._diffuse;

        float lightDist = length(light._pos - fragPos); // TODO waste of invsqrt
        float atten = 1.0f / (attenConstant + lightDist*attenLinear + lightDist*lightDist*attenQuad);
        diffuse *= atten;
        specular *= atten;
        vec3 ambient = light._ambient * atten;
        result += diffuse + ambient + specular;
    }
    FragColor = vec4(result, 1.0) * albedo;
}
