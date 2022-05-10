#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uMvpTrans;
uniform mat4 uModelTrans;
uniform mat3 uModelInvTrans;

out vec2 texCoord;
out vec3 fragPos;
out vec3 normal;

void main() {
    gl_Position = uMvpTrans * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    fragPos = vec3(uModelTrans * vec4(aPos, 1.0f));
    texCoord = aTexCoord;
    normal = uModelInvTrans * aNormal;
}