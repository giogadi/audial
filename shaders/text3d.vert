#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uMvpTrans;

out vec2 texCoord;

void main() {
    gl_Position = uMvpTrans * vec4(aPos, 1.0f);
    texCoord = aTexCoord;
}
