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
    _subMeshes.reserve(loader.LoadedMeshes.size());
    for (objl::Mesh const& mesh : loader.LoadedMeshes) {
        _subMeshes.emplace_back();
        SubMesh& subMesh = _subMeshes.back(); 
        subMesh._startIndex = _numIndices;        
        subMesh._numIndices = mesh.Indices.size();
        _numIndices += mesh.Indices.size();
        subMesh._color = Vec4(
            mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z, 1.f);
    }

    // TODO: JUST USE THE MINIMUM NUMBER OF VERTICES AND INDICES

    std::vector<float> vertexData;
    vertexData.reserve(kNumValuesPerVertex * _numIndices);
    for (objl::Mesh const& mesh : loader.LoadedMeshes) {
        for (unsigned int index : mesh.Indices) {
            objl::Vertex const& v = mesh.Vertices[index];
            vertexData.push_back(v.Position.X);
            vertexData.push_back(v.Position.Y);
            vertexData.push_back(v.Position.Z);

            vertexData.push_back(v.Normal.X);
            vertexData.push_back(v.Normal.Y);
            vertexData.push_back(v.Normal.Z);

            vertexData.push_back(v.TextureCoordinate.X);
            vertexData.push_back(v.TextureCoordinate.Y);
        }        
    }

    Init(vertexData.data(), _numIndices);

    return true;
}