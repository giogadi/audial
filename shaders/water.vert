#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uMvpTrans;
uniform mat4 uModelTrans;
uniform float uTime;

out vec2 texCoord;
out vec3 fragPos;
out vec3 normal;

void main() {
    float heightOffset = 0.5 * sin(aPos.x + uTime);
    vec3 p = aPos;
    p.y += heightOffset;
    gl_Position = uMvpTrans * vec4(p,1.0);
    fragPos = vec3(uModelTrans * vec4(p, 1.0f));
    texCoord = aTexCoord;
    normal = vec3(0.5*cos(aPos.x + uTime), 1.0, 0.0);
}
