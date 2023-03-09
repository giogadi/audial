#version 330 core

in vec3 bary;

uniform vec4 uColor;

out vec4 FragColor;

// From catlike coding on wireframe shading
void main() {
    vec3 deltas = fwidth(bary);
    vec3 barys = smoothstep(deltas, 2 * deltas, bary);
    float minBary = min(barys.x, min(barys.y, barys.z));
    FragColor = mix(uColor, vec4(0,0,0,0), minBary);
}
