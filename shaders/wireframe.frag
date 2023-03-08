#version 330 core

in vec3 bary;
in float area;

uniform vec4 uColor;

out vec4 FragColor;

void main() {
    float kEpsilon = 0.05f;
    if (bary.x < kEpsilon || bary.y < kEpsilon || bary.z < kEpsilon) {
        FragColor = uColor;
    } else {
        FragColor = vec4(0,0,0,0);
    }
    // FragColor = vec4(bary, 1.f);
}
