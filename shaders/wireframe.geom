#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 bary;
} gs_in[];

out vec3 bary;

void main() {
    //bary = gs_in[0].bary;
    
    gl_Position = gl_in[0].gl_Position;
    bary = vec3(1,0,0);
    EmitVertex();

    //bary = gs_in[1].bary;
    gl_Position = gl_in[1].gl_Position;
    bary = vec3(0,1,0);
    EmitVertex();

    //bary = gs_in[2].bary;
    gl_Position = gl_in[2].gl_Position;
    bary = vec3(0,0,1);
    EmitVertex();

    EndPrimitive();
}
