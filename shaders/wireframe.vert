#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aBary;

uniform mat4 uMvpTrans;
uniform vec4 uColor;

out VS_OUT {
    vec3 bary;
} vs_out;

void main() {
    gl_Position = uMvpTrans * vec4(aPos.xyz, 1.f);
    vs_out.bary = aBary;
}
