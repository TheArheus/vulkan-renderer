#version 450

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TextCoord;

layout(location = 0) out vec4 Color;

void main()
{
    gl_Position = vec4(Pos, 1.0f);
    Color = vec4(Normal * 0.5f + vec3(0.5f), 1.0f);
}
