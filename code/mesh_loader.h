#if !defined(MESH_H)
#define MESH_H

union tex2d
{
    struct
    {
        float u;
        float v;
    };
    float E[2];
};

union face_t
{
    struct
    {
        uint32_t a;
        uint32_t b;
        uint32_t c;
    };
    uint32_t E[3];
};

union v3
{
    struct
    {
        float x;
        float y;
        float z;
    };
    float E[3];
};

v3 V3(float a, float b, float c)
{
    v3 Result = {};

    Result.x = a;
    Result.y = b;
    Result.z = c;

    return Result;
}

struct vertex
{
    v3 Vertex;
    v3 Normal;
    tex2d TextCoord;
};

typedef struct
{
    std::vector<vertex> Vertices;
    std::vector<face_t> VertexIndices;
} object_t;

struct meshlet_t
{
    uint32_t Vertices[64];
    uint32_t Indices[126*3]; // NOTE: Up to 126 triangles
    uint8_t TriangleCount;
    uint8_t VertexCount;
};

typedef struct
{
    std::vector<v3>    Vertices;
    std::vector<v3>    Normals;
    std::vector<tex2d> TextureCoords;

    std::vector<face_t> VertexIndices;
    std::vector<face_t> NormalIndices;
    std::vector<face_t> TextureIndices;

    std::vector<vertex> Mesh;
    std::vector<uint32_t> ConvertedVertexIndices;

    std::vector<meshlet_t> Meshlets;
} mesh_t;

extern std::array<mesh_t, (1 << 4)> Meshes;
extern int32_t MeshesCount;

void LoadMeshData(mesh_t*, char*);
static void LoadTexture(mesh_t* Mesh, char* Filename);
void LoadMesh(char* ObjectPath);

static tex2d 
V3ToUVs(v3 Vert)
{
    tex2d Result = {};
    Result.u = Vert.x;
    Result.v = Vert.y;
    return Result;
}

#endif
