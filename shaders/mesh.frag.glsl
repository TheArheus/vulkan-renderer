#version 450

#extension GL_NV_mesh_shader: require

layout(location = 0) out vec4 OutputColor;

layout(location = 0) in vec4 Color;
layout(location = 1) preprimitiveNV in vec3 TriangleNormal;

void main()
{
    OutputColor = vec4(TriangleNormal*0.5f + 0.5f, 1.0f);
}
