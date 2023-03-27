#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uMvpTrans;
uniform mat4 uModelTrans;
uniform mat3 uModelInvTrans;
uniform vec3 uOffset;

uniform sampler2D uNormalMap;
uniform sampler2D uHeightMap;

out vec2 texCoord;
out vec3 fragPos;
out vec3 normal;

void main() {
    gl_Position = uMvpTrans * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    fragPos = vec3(uModelTrans * vec4(aPos, 1.0f));
    // texCoord = 2 * aTexCoord + vec2(0.25 * uTime, 0);
    vec3 scaledOffset = 0.05f * uOffset;
    texCoord = aTexCoord + vec2(scaledOffset.x, -scaledOffset.z);
    normal = uModelInvTrans * aNormal;
}

// void main() {
//     vec3 p = aPos;
//     texCoord = 2 * aTexCoord + vec2(0.25 * uTime, 0);
//     p.y += texture(uHeightMap, texCoord).x;
//     gl_Position = uMvpTrans * vec4(p, 1.f);
//     fragPos = vec3(uModelTrans * vec4(p, 1.0f));
//     normal = uModelInvTrans * aNormal;
// }
