#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_NV_mesh_shader: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_primitives = 4) out;

struct vert
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tx, ty;
};

struct meshlet_t
{
    uint Vertices[64];
    uint8_t Indices[126]; // NOTE: Up to 42 triangles
    uint8_t IndexCount;
    uint8_t VertexCount;
};

layout(binding = 0) readonly buffer Vertices { vert Vert[]; };
layout(binding = 1) readonly buffer Meshlets { meshlet_t Meshlets_[]; };

layout(location = 0) out vec4 Color;

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;

    for(int Index = 0;
        Index < (uint)Meshlets_[MeshletIndex].VertexCount;
        ++Index)
    {
        uint VertexIndex = Meshlets_[MeshletIndex].Vertices[Index];

        vec4 Pos = vec4(Vert[VertexIndex].vx, Vert[VertexIndex].vy, Vert[VertexIndex].vz + 0.5f, 1.0f);
        vec3 Norm = vec3(Vert[VertexIndex].nx, Vert[VertexIndex].ny, Vert[VertexIndex].nz);
        vec2 TextCoord = vec2(Vert[VertexIndex].tx, Vert[VertexIndex].ty);
        
        gl_MeshVerticesNV[Index].gl_Position = Pos;
        Color = vec4(Norm * 0.5f + vec3(0.5f), 1.0f);
    }

    gl_PrimitiveCountNV = (uint)Meshlets_[MeshletIndex].IndexCount / 3;

    for(uint Index = 0;
        Index < (uint)Meshlets_[MeshletIndex].IndexCount;
        ++Index)
    {
        gl_PrimitiveIndicesNV[Index] = (uint)Meshlets_[MeshletIndex].Indices[Index];
    }
}
