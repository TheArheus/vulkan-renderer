#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_NV_mesh_shader: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_primitives = 126) out;

struct vert
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tx, ty;
};

struct meshlet_t
{
    uint Vertices[64];
    uint8_t Indices[126*3]; // NOTE: Up to 126 triangles
    uint8_t TriangleCount;
    uint8_t VertexCount;
};

layout(binding = 0) readonly buffer Vertices { vert Vert[]; };
layout(binding = 1) readonly buffer Meshlets { meshlet_t Meshlets_[]; };

layout(location = 0) out vec4 Color;
preprimitiveNV out vec3 TriangleNormals[];

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;
    uint TIndex = gl_LocalInvocationID.x;

    uint VertexCount = (uint)Meshlets_[MeshletIndex].VertexCount;
    uint TriangleCount = (uint)Mehslets_[MeshletIndex].TriangleCount;
    uint IndexCount = TriangleCount * 3;

    for(int Index = 0;
        Index < VertexCount;
        Index += 32)
    {
        uint VertexIndex = Meshlets_[MeshletIndex].Vertices[Index];

        vec4 Pos = vec4(Vert[VertexIndex].vx, Vert[VertexIndex].vy, Vert[VertexIndex].vz + 0.5f, 1.0f);
        vec3 Norm = vec3(Vert[VertexIndex].nx, Vert[VertexIndex].ny, Vert[VertexIndex].nz);
        vec2 TextCoord = vec2(Vert[VertexIndex].tx, Vert[VertexIndex].ty);
        
        gl_MeshVerticesNV[Index].gl_Position = Pos;
        Color = vec4(Norm * 0.5f + vec3(0.5f), 1.0f);
    }

    //uint IndexChunkCount = (IndexCount + 7) / 3;
    for(uint Index = 0;
        Index < IndexCount;
        Index += 32)
    {
        gl_PrimitiveIndicesNV[Index] = (uint)Meshlets_[MeshletIndex].Indices[Index];
        //writePackedPrimitiveIndices4x8NV(Index*8+0, Meshlets_[MeshletIndex].Indices[Index*2+0]);
        //writePackedPrimitiveIndices4x8NV(Index*8+4, Meshlets_[MeshletIndex].Indices[Index*2+1]);
    }

    if(TIndex == 0)
    {
        gl_PrimitiveCountNV = (uint)Meshlets_[MeshletIndex].IndexCount / 3;
    }
}
