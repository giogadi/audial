#version 330 core

in vec2 texCoord;
in vec3 fragPos;
in vec3 normalNonNorm;
in vec4 fragPosLightSpace;

#define NUM_POINT_LIGHTS 5 

struct PointLight {
    vec4 _pos;
    vec4 _color;
    float _ambient;
    float _diffuse;
    float _specular;
    float _constant;
    float _linear;
    float _quadratic;
    float _padding0;
    float _padding1;
};
uniform PointLight uPointLights[NUM_POINT_LIGHTS];

struct DirLight {
    vec3 _dir;
    vec3 _color;
    float _ambient;
    float _diffuse;    
    bool _shadows;
};
uniform DirLight uDirLight;

uniform sampler2D uMyTexture;
uniform sampler2D uShadowMap;

uniform vec4 uColor;
uniform vec3 uViewPos;
uniform float uLightingFactor;

out vec4 FragColor;

float shininess = 32.f;

void CalcPointLight(in PointLight light, in vec3 viewDir, in vec3 normal, inout vec3 totalAmbient, inout vec3 totalDiffuse, inout vec3 totalSpecular) {
    vec3 lightDir = normalize(light._pos.xyz - fragPos);
           
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), shininess);
    vec3 specular = spec * light._color.xyz * light._specular;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff*light._color.xyz*light._diffuse;

    float lightDist = length(light._pos.xyz - fragPos); // TODO waste of invsqrt
    float atten = 1.0f / (light._constant + lightDist*light._linear + lightDist*lightDist*light._quadratic);
    diffuse *= atten;
    specular *= atten;
    vec3 ambient = light._color.xyz * light._ambient * atten;
    //result += diffuse + ambient + specular;
    totalAmbient += ambient;
    totalDiffuse += diffuse;
    totalSpecular += specular;
}

void main() {
    vec3 normal = normalize(normalNonNorm);

    
    vec3 viewDir = normalize(uViewPos - fragPos);

    vec4 albedo = texture(uMyTexture, texCoord);
    albedo *= uColor;
    
    // vec3 result = vec3(0, 0, 0);
    vec3 totalAmbient = vec3(0,0,0);
    vec3 totalDiffuse = vec3(0,0,0);
    vec3 totalSpecular = vec3(0,0,0);
    // Directional light
    {
        float diff = max(dot(normal, -uDirLight._dir), 0.0);
        vec3 diffuse = diff * uDirLight._color * uDirLight._diffuse;
        vec3 ambient = uDirLight._color * uDirLight._ambient;
        totalAmbient += ambient;
        totalDiffuse += diffuse;
        //result += diffuse + ambient; 
    }

    // Point lights
#if 1
    for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
        CalcPointLight(uPointLights[i], viewDir, normal, totalAmbient, totalDiffuse, totalSpecular);
    }
#else
    {

    }
#endif

    // shadow
    float shadow = 0.0f;
    if (uDirLight._shadows) {
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;  // [-1,1] -> [0,1]
        float closestDepth = texture(uShadowMap, projCoords.xy).r;
        float currentDepth = projCoords.z;
        // TODO: CURRENTLY USING DIR LIGHT! This may not always be true
        // float bias = max(0.05 * (1.0 - dot(normal, uDirLight._dir)), 0.0025);
        float bias = 0.001;
        vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0f;
        if (projCoords.z > 1.f) {
            shadow = 0.0f;
        }
    }

    vec3 lighting = totalAmbient + (1-shadow)*(totalDiffuse + totalSpecular);

    //FragColor = vec4(mix(lighting, vec3(1.0), uLightingFactor), 1.0) * albedo;
    vec3 lightAdjusted = mix(vec3(1.0), lighting, uLightingFactor);
    FragColor = vec4(lightAdjusted, 1.0) * albedo;
}
