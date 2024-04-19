#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uViewProjT;
uniform mat4 uModelT;

void main() {
   gl_Position = uViewProjT * uModelT * vec4(aPos.x, aPos.y, aPos.z, 1.0); 
}
