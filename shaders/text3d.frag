#version 330 core

in vec2 texCoord;

uniform sampler2D uMyTexture;
uniform vec4 uColor;

out vec4 FragColor;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(uMyTexture, texCoord).r);
    FragColor = uColor * sampled;
}
