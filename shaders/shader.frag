#version 330 core

in vec2 texCoord;

uniform sampler2D myTexture;

out vec4 FragColor;

void main() {
    //FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    FragColor = texture(myTexture, texCoord);
}