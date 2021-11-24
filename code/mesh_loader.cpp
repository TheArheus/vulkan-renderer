
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

std::array<mesh_t, (1<<4)> Meshes;
int32_t MeshesCount = 0;

static v3
ParseVertex(char* String)
{
    v3 Result = {};

    char ValueToConvert[10] = {' '};
    char FirstChar = ' ';
    int ValueIdx = 0;
    int VertIdx = 0;
    for(char* StringToConv = String; *StringToConv;)
    {
        if (*StringToConv == 'v' || *StringToConv == 't' || *StringToConv == 'n') { FirstChar = *StringToConv++; continue; }
        if((*StringToConv == ' ' || *StringToConv == '\n') && (FirstChar != 'v') && (FirstChar != 't') && (FirstChar != 'n'))
        {
            Result.E[VertIdx] = strtof(ValueToConvert, NULL);
            memset(ValueToConvert, 0, sizeof(ValueToConvert));

            ValueIdx = 0;
            VertIdx++;
            StringToConv++;
            continue;
        }

        ValueToConvert[ValueIdx++] = *StringToConv;
        StringToConv++;
        FirstChar = 0;
    }
    
    return Result;
}


struct parsed_obj 
{
    int32_t VertexIndices[3];
    int32_t NormalIndices[3];
    int32_t TextureIndices[3];
};
static parsed_obj
ParseFace(char* String)
{
    parsed_obj Result = {};

    char ValueToConvert[8] = {' '};
    char FirstChar = 0;
    char PrevChar = 0;
    int  ValueIdx = 0;
    int  MeshIdx = 0;
    int  TextIdx = 0;
    int  NormIdx = 0;
    int  Type = 0;
    for(char* StringToConv = String; *StringToConv;)
    {
        if (*StringToConv == 'f') { FirstChar = *StringToConv++; continue; }
        if ((*StringToConv == ' ' || *StringToConv == '\n') && (FirstChar != 'f'))
        {
            if(ValueToConvert[0] != ' ')
            {
                if (Type == 0)
                {
                    Result.VertexIndices[MeshIdx++] = atoi(ValueToConvert)-1;
                    memset(ValueToConvert, 0, sizeof(ValueToConvert));
                }
                if (Type == 2)
                {
                    Result.TextureIndices[TextIdx++] = atoi(ValueToConvert)-1;
                    memset(ValueToConvert, 0, sizeof(ValueToConvert));
                }
                if (Type == 1)
                {
                    Result.NormalIndices[NormIdx++] = atoi(ValueToConvert)-1;
                    memset(ValueToConvert, 0, sizeof(ValueToConvert));
                }
            }

            ValueIdx = 0;
            Type = 0;
            PrevChar = *StringToConv;
            StringToConv++;
            continue;
        }
        else if(*StringToConv == '/')
        {
            if (PrevChar == '/') { *StringToConv++; continue; };
            if(Type == 0)
            {
                Result.VertexIndices[MeshIdx++] = atoi(ValueToConvert)-1;
                memset(ValueToConvert, 0, sizeof(ValueToConvert));
            }
            if(Type == 2)
            {
                Result.TextureIndices[TextIdx++] = atoi(ValueToConvert)-1;
                memset(ValueToConvert, 0, sizeof(ValueToConvert));
            }
            if(Type == 1)
            {
                Result.NormalIndices[NormIdx++] = atoi(ValueToConvert)-1;
                memset(ValueToConvert, 0, sizeof(ValueToConvert));
            }

            Type = (Type + 1) % 3;
            ValueIdx = 0;
            PrevChar = *StringToConv;
            StringToConv++;
            continue;
        }
        
        if(FirstChar != 'f')
        {
            PrevChar = *StringToConv;
            if (Type == 0)
            {
                ValueToConvert[ValueIdx++] = *StringToConv;
                StringToConv++;
            }
            else if (Type == 2)
            {
                ValueToConvert[ValueIdx++] = *StringToConv;
                StringToConv++;
            }
            else if (Type == 1)
            {
                ValueToConvert[ValueIdx++] = *StringToConv;
                StringToConv++;
            }
            else
            {
                StringToConv++;
            }
        }
        
        FirstChar = 0;
    }
    return Result;
}

void LoadMeshData(mesh_t* Mesh, char* FileName)
{
    FILE* file = NULL;
    fopen_s(&file, FileName, "r");
    
    std::vector<tex2d> TextureCoords;
    char Line[256];
    while(fgets(Line, 256, (FILE*)file) != NULL)
    {
        int32_t CharIdx = 0;
        char PrevChar = 0;
        char Char = Line[CharIdx];

        while(Char)
        {
            Char = Line[CharIdx++];
            if(Char == '#' || Char == 'm' || Char == 'o' && Char == 'u' || Char == 's') break;
            if((PrevChar == 'v' && Char == 't')) 
            {
                v3 Coords = ParseVertex(Line);
                tex2d UVCoords = V3ToUVs(Coords);
                Mesh->TextureCoords.push_back(UVCoords);
                break;
            }
            if((PrevChar == 'v' && Char == 'n'))
            {
                v3 Normal = ParseVertex(Line);
                Mesh->Normals.push_back(Normal);
                break;
            }

            if((PrevChar == 'v' && Char == ' '))
            {
                v3 Vert = ParseVertex(Line);
                Mesh->Vertices.push_back(Vert);
                break;
            } 
            if((PrevChar == 'f' && Char == ' '))
            {
                face_t Face = {};
                face_t NormalFace = {};
                face_t TextureCoordFace = {};
                parsed_obj Obj = ParseFace(Line);

#if 1
                memcpy(Face.E, Obj.VertexIndices, sizeof(uint32_t)*3);
                memcpy(NormalFace.E, Obj.NormalIndices, sizeof(uint32_t)*3);
                memcpy(TextureCoordFace.E, Obj.TextureIndices, sizeof(uint32_t)*3);

                Mesh->VertexIndices.push_back(Face);
                Mesh->NormalIndices.push_back(NormalFace);
                Mesh->TextureIndices.push_back(TextureCoordFace);
#else
                Mesh->VertexIndices.push_back(Obj.VertexIndices[0]);
                Mesh->VertexIndices.push_back(Obj.VertexIndices[1]);
                Mesh->VertexIndices.push_back(Obj.VertexIndices[2]);

                Mesh->NormalIndices.push_back(Obj.NormalIndices[0]);
                Mesh->NormalIndices.push_back(Obj.NormalIndices[1]);
                Mesh->NormalIndices.push_back(Obj.NormalIndices[2]);

                Mesh->TextureIndices.push_back(Obj.TextureIndices[0]);
                Mesh->TextureIndices.push_back(Obj.TextureIndices[1]);
                Mesh->TextureIndices.push_back(Obj.TextureIndices[2]);
#endif
                break;
            }
            PrevChar = Char;
        }
    }
}

void 
BuildMeshlets(mesh_t* Mesh)
{
    meshlet_t CurrentMeshlet = {};
    std::vector<uint8_t> MeshletVertices(Mesh->Vertices.size(), 0xff);

    for(size_t FaceIndex = 0;
        FaceIndex < Mesh->VertexIndices.size();
        FaceIndex += 1)
    {
        face_t Face = Mesh->VertexIndices[FaceIndex];

        uint8_t& av = MeshletVertices[Face.a];
        uint8_t& bv = MeshletVertices[Face.b];
        uint8_t& cv = MeshletVertices[Face.c];

        if(CurrentMeshlet.VertexCount + (av == 0xff) + (bv == 0xff) + (cv == 0xff) > 64 || CurrentMeshlet.IndexCount + 3 > 126)
        {
            Mesh->Meshlets.push_back(CurrentMeshlet);
            CurrentMeshlet = {};
            memset(MeshletVertices.data(), 0xff, Mesh->Vertices.size());
        }

        if(av != 0xff)
        {
            av = CurrentMeshlet.VertexCount;
            CurrentMeshlet.Vertices[CurrentMeshlet.VertexCount++] = Face.a;
        }

        if(bv != 0xff)
        {
            bv = CurrentMeshlet.VertexCount;
            CurrentMeshlet.Vertices[CurrentMeshlet.VertexCount++] = Face.b;
        }

        if(cv != 0xff)
        {
            cv = CurrentMeshlet.VertexCount;
            CurrentMeshlet.Vertices[CurrentMeshlet.VertexCount++] = Face.c;
        }

        CurrentMeshlet.Indices[CurrentMeshlet.IndexCount++] = av;
        CurrentMeshlet.Indices[CurrentMeshlet.IndexCount++] = bv;
        CurrentMeshlet.Indices[CurrentMeshlet.IndexCount++] = cv;
    }

    if(CurrentMeshlet.IndexCount)
    {
        Mesh->Meshlets.push_back(CurrentMeshlet);
    }
}

void 
LoadMesh(char* ObjectPath)
{
    mesh_t NewMesh = {};

    LoadMeshData(&NewMesh, ObjectPath);
    size_t IndexCount = NewMesh.VertexIndices.size();
    std::vector<vertex> NewVertexBuffer(IndexCount*3);
    std::vector<uint32_t> NewIndexBuffer(IndexCount*3);

    for(uint32_t Index = 0;
        Index < (IndexCount);
        Index += 1)
    {
#if 1
        vertex& NewVertex1 = NewVertexBuffer[Index*3+0];
        vertex& NewVertex2 = NewVertexBuffer[Index*3+1];
        vertex& NewVertex3 = NewVertexBuffer[Index*3+2];

        face_t vi  = NewMesh.VertexIndices[Index];
        face_t vni = NewMesh.NormalIndices[Index];
        face_t vti = NewMesh.TextureIndices[Index];

        NewVertex1.Vertex = NewMesh.Vertices[vi.a];
        NewVertex2.Vertex = NewMesh.Vertices[vi.b];
        NewVertex3.Vertex = NewMesh.Vertices[vi.c];

        if (NewMesh.Normals.size() != 0)
        {
            NewVertex1.Normal = NewMesh.Normals[vni.a];
            NewVertex2.Normal = NewMesh.Normals[vni.b];
            NewVertex3.Normal = NewMesh.Normals[vni.c];
        }
        if (NewMesh.TextureCoords.size() != 0)
        {
            NewVertex1.TextCoord = NewMesh.TextureCoords[vti.a];
            NewVertex2.TextCoord = NewMesh.TextureCoords[vti.b];
            NewVertex3.TextCoord = NewMesh.TextureCoords[vti.c];
        }
#else
        vertex& NewVertex = NewVertexBuffer[Index];

        int vi = NewMesh.VertexIndices[Index];
        int vti = NewMesh.TextureIndices[Index];
        int vni = NewMesh.NormalIndices[Index];
    
        NewVertex.vx = NewMesh.Vertices[vi * 3 + 0];
        NewVertex.vy = NewMesh.Vertices[vi * 3 + 1];
        NewVertex.vz = NewMesh.Vertices[vi * 3 + 2];
    
        if (NewMesh.Normals.size() != 0)
        {
            NewVertex.nx = NewMesh.Normals[vni * 3 + 0];
            NewVertex.ny = NewMesh.Normals[vni * 3 + 1];
            NewVertex.nz = NewMesh.Normals[vni * 3 + 2];
        }
        if (NewMesh.TextureCoords.size() != 0)
        {
            NewVertex.tu = NewMesh.TextureCoords[vti * 2 + 0];
            NewVertex.tv = NewMesh.TextureCoords[vti * 2 + 1];
        }
#endif
    }

    for(uint32_t Index = 0;
        Index < NewIndexBuffer.size();
        ++Index)
    {
        NewIndexBuffer[Index] = Index;
    }

    NewMesh.Mesh = NewVertexBuffer;
    NewMesh.ConvertedVertexIndices = NewIndexBuffer;

#if RTX
    BuildMeshlets(&NewMesh);
#endif

    Meshes[MeshesCount++] = NewMesh;
}


