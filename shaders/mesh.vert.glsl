#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require

#define Pi32 3.14159265359

/*
struct glsl_vert
{
    float VertexX, VertexY, VertexZ;
    float NormalX, NormalY, NormalZ;
    float TextureCoordU, TextureCoordV;
};

layout(binding = 0) buffer Vertices 
{
    glsl_vert VerticesIn[];
};
*/

mat4 GetIdentity()
{
    mat4 M = 
    mat4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    
    return M;
}
mat4 RotateZ(float D)
{
    float c = cos(D);
    float s = sin(D);
    
    mat4 M = 
    mat4(
        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    
    return M;
}


mat4 RotateY(float D)
{
    float c = cos(D);
    float s = sin(D);
    
    mat4 M = 
    mat4(
        c, 0, s, 0,
        -s, 1, c, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    
    return M;
}


mat4 RotateX(float D)
{
    float c = cos(D);
    float s = sin(D);
    
    mat4 M = 
    mat4(
        1, 0, 0, 0,
        0, c, -s, 0,
        0, s, c, 0,
        0, 0, 0, 1
    );
    
    return M;
}


mat4 Scale(vec3 S)
{
    mat4 M = 
    mat4(
        S.x, 0, 0, 0,
        0, S.y, 0, 0,
        0, 0, S.z, 0,
        0, 0,   0, 1
    );
    
    return M;
}

mat4 Translate(vec3 T)
{
    mat4 M = 
    mat4(
        1, 0, 0, T.x,
        0, 1, 0, T.y,
        0, 0, 1, T.z,
        0, 0, 0, 1
    );
    
    return M;
}

struct vert
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tx, ty;
};

layout(binding = 0) buffer Vertices { vert Vert[]; };

layout(location = 0) out vec4 Color;

void main()
{
    vert V = Vert[gl_VertexIndex];
    vec4 Pos = vec4(Vert[gl_VertexIndex].vx, Vert[gl_VertexIndex].vy, Vert[gl_VertexIndex].vz + 0.5f, 1.0f);
    vec3 Norm = vec3(Vert[gl_VertexIndex].nx, Vert[gl_VertexIndex].ny, Vert[gl_VertexIndex].nz);
    
    /*
    mat4 World = GetIdentity();
    mat4 S = Scale(vec3(1));
    mat4 RX = RotateX(0);
    mat4 RY = RotateY(0);
    mat4 RZ = RotateZ(0);
    mat4 T = Translate(vec3(0, 0, 0.8));
    
    World = World * S;
    World = World * RX;
    World = World * RY;
    World = World * RZ;
    World = World * T;
    
    Pos = World * Pos;
    */
    
    gl_Position = Pos;
    Color = vec4(Norm * 0.5f + vec3(0.5f), 1.0f);
    //Color = vec4(1, 1, 0, 1);
}


