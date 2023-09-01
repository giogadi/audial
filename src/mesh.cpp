#include "mesh.h"

#include <iostream>

#include <glad/glad.h>
#include "OBJ_Loader.h"

void BoundMeshPNU::Init(float* vertexData, int numVertices) {
    int numIndices = numVertices;
    uint32_t* indices = new uint32_t[numVertices];
    for (int i = 0; i < numIndices; ++i) {
        indices[i] = (uint32_t)i;
    }
    Init(vertexData, numVertices, indices, numIndices);
    delete[] indices;
}

void BoundMeshPNU::Init(float* vertexData, int numVertices, uint32_t* indexData, int numIndices) {
    _numVerts = numVertices;

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    
    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(float) * kNumValuesPerVertex * numVertices, vertexData,
        GL_STATIC_DRAW);
    
    _numIndices = numIndices;
    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numIndices, indexData, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(
        /*attributeIndex=*/0, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false,
        /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)0);
    // This vertex attribute will be read from the VBO that is currently bound
    // to GL_ARRAY_BUFFER, which was bound above to "vbo".
    glEnableVertexAttribArray(/*attributeIndex=*/0);

    // Normal attribute
    glVertexAttribPointer(
        /*attributeIndex=*/1, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false,
        /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)(3*sizeof(float)));
    // This vertex attribute will be read from the VBO that is currently bound
    // to GL_ARRAY_BUFFER, which was bound above to "vbo".
    glEnableVertexAttribArray(/*attributeIndex=*/1);

    // (s,t) attribute
    glVertexAttribPointer(
        /*attributeIndex=*/2, /*numValues=*/2, /*valueType=*/GL_FLOAT, /*normalized=*/false,
        /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(/*attributeIndex=*/2);
}

bool BoundMeshPNU::Init(char const* objFilename) {
    objl::Loader loader;
    bool success = loader.LoadFile(objFilename);
    if (!success) {
        return false;
    }
    
    _numIndices = 0;
    _numVerts = 0;
    _subMeshes.reserve(loader.LoadedMeshes.size());
    std::vector<uint32_t> indexData;    
    indexData.reserve(loader.LoadedIndices.size());
    std::vector<float> vertexData;
    vertexData.reserve(kNumValuesPerVertex * loader.LoadedVertices.size());
    for (objl::Mesh const& mesh : loader.LoadedMeshes) {
        _subMeshes.emplace_back();
        SubMesh& subMesh = _subMeshes.back(); 
        subMesh._startIndex = _numIndices;        
        subMesh._numIndices = mesh.Indices.size();
        subMesh._color = Vec4(
            mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z, 1.f);

        for (objl::Vertex const& v : mesh.Vertices) {
            vertexData.push_back(v.Position.X);
            vertexData.push_back(v.Position.Y);
            vertexData.push_back(v.Position.Z);

            subMesh._centroid += Vec3(v.Position.X, v.Position.Y, v.Position.Z);

            vertexData.push_back(v.Normal.X);
            vertexData.push_back(v.Normal.Y);
            vertexData.push_back(v.Normal.Z);

            vertexData.push_back(v.TextureCoordinate.X);
            vertexData.push_back(v.TextureCoordinate.Y);
        }
        if (!mesh.Vertices.empty()) {
            subMesh._centroid /= mesh.Vertices.size();
        }

        for (uint32_t index : mesh.Indices) {
            indexData.push_back(index + _numVerts);
        }

        _numVerts += mesh.Vertices.size();
        _numIndices += mesh.Indices.size();
    }

    assert(_numVerts == loader.LoadedVertices.size());
    assert(indexData.size() == _numIndices);

    Init(vertexData.data(), _numVerts, indexData.data(), _numIndices);

    return true;
}

bool BoundMeshPB::Init(char const* objFilename) {
    objl::Loader loader;
    bool success = loader.LoadFile(objFilename);
    if (!success) {
        return false;
    }

    if (loader.LoadedMeshes.size() != 1) {
        printf("BoundMeshPB: ERROR, I can only handle one mesh right now! (found %zu meshes)\n", loader.LoadedMeshes.size());
        return false;
    }

    objl::Mesh const& mesh = loader.LoadedMeshes.front();

    if (mesh.Indices.size() % 3 != 0) {
        printf("BoundMeshPB: ERROR, imported mesh did not have # indices divisible by 3! Num indices: %zu\n", mesh.Indices.size());
        return false;
    }

    int const numTriangles = mesh.Indices.size() / 3;
    std::vector<float> vertexData;
    vertexData.reserve(numTriangles * 3 * kNumValuesPerVertex);

    std::vector<uint32_t> indexData;
    indexData.reserve(numTriangles * 3);

    Vec3 baryCoords[3] = {
        Vec3(1.f, 0.f, 0.f),
        Vec3(0.f, 1.f, 0.f),
        Vec3(0.f, 0.f, 1.f)
    };
    for (int triangleIx = 0; triangleIx < numTriangles; ++triangleIx) {
        Vec3 verts[3];
        for (int i = 0; i < 3; ++i) {
            int vIdx = mesh.Indices[3*triangleIx + i];
            auto const& meshV = mesh.Vertices[vIdx];
            verts[i].Set(meshV.Position.X, meshV.Position.Y, meshV.Position.Z);
        }       

        for (int i = 0; i < 3; ++i) {
            vertexData.push_back(verts[i]._x);
            vertexData.push_back(verts[i]._y);
            vertexData.push_back(verts[i]._z);

            vertexData.push_back(baryCoords[i]._x);
            vertexData.push_back(baryCoords[i]._y);
            vertexData.push_back(baryCoords[i]._z);

            indexData.push_back(3*triangleIx + i);
        }
    }

    _numVerts = numTriangles * 3;
    _numIndices = _numVerts;

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * kNumValuesPerVertex * _numVerts, vertexData.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * _numIndices, indexData.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(
        /*attributeIndex=*/0, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false, /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)0);
    glEnableVertexAttribArray(/*attributeIndex=*/0);

    // Barycentric attribute
    glVertexAttribPointer(
        /*attributeIndex=*/1, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false, /*stride=*/kNumValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(/*attributeIndex=*/1);

    return true;
}
