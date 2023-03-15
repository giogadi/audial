#pragma once

#include <vector>

#include "matrix.h"

// Expects 8 values per vertex (ppp nnn uv)
class BoundMeshPNU {
public:
    // ppp nnn uv
    static int constexpr kNumValuesPerVertex = 8;

    void Init(float* vertexData, int numVertices, uint32_t* indexData, int numIndices);

    // This assumes numVertices == numIndices
    // We make a full copy of vertexData, so no need for it to live beyond this function call.
    void Init(float* vertexData, int numVertices);

    bool Init(char const* objFilename);

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _numVerts = -1;
    int _numIndices = -1;

    bool _useTriangleFan = false;

    struct SubMesh {
        int _startIndex;  // index into the index buffer (not the vertex buffer)
        int _numIndices;
        Vec4 _color;
    };
    std::vector<SubMesh> _subMeshes;
};

struct BoundMeshPB {
    // ppp bbb
    static int constexpr kNumValuesPerVertex = 6;

    bool Init(char const* objFilename);

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;
    int _numVerts = -1;
    int _numIndices = -1;
};
