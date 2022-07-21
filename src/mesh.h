#pragma once

#include <vector>

#include "matrix.h"

// Expects 8 values per vertes (ppp nnn uv)
class BoundMeshPNU {
public:
    // ppp nnn uv
    static int constexpr kNumValuesPerVertex = 8;

    // We make a full copy of vertexData, so no need for it to live beyond this function call.ß
    void Init(float* vertexData, int numVertices);

    bool Init(char const* objFilename);

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    int _numVerts = -1;

    struct SubMesh {
        int _startIndex;
        int _numVerts;
        Vec4 _color;
    };
    std::vector<SubMesh> _subMeshes;
};