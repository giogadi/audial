#version 330 core

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D uDepthMap;

void main() {
    float depthValue = texture(uDepthMap, texCoords).r;
    FragColor = vec4(vec3(depthValue), 1.0);
}
