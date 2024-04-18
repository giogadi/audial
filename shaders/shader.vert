#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uMvpTrans;
uniform mat4 uModelTrans;
uniform mat3 uModelInvTrans;
uniform vec3 uExplodeVec;

out vec2 texCoord;
out vec3 fragPos;
out vec3 normalNonNorm;

void main() {
    vec3 p = aPos;
    p += uExplodeVec;
    gl_Position = uMvpTrans * vec4(p.x, p.y, p.z, 1.0);
    fragPos = vec3(uModelTrans * vec4(p, 1.0f));
    texCoord = aTexCoord;
    normalNonNorm = uModelInvTrans * aNormal;
}
