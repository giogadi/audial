#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aBary;
layout (location = 2) in float aArea;

uniform mat4 uMvpTrans;
uniform vec4 uColor;

out vec3 bary;
out float area;

void main() {
    gl_Position = uMvpTrans * vec4(aPos.xyz, 1.f);
    bary = aBary;
    area = aArea;
}
