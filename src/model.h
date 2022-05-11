#pragma once

#include <iostream>

#include <glad/glad.h> 

#include "stb_image.h"
#include "glm/ext/matrix_clip_space.hpp"

#include "shader.h"

class Texture {
public:
    static std::unique_ptr<Texture> CreateTextureFromFile(char const* filename) {
        stbi_set_flip_vertically_on_load(true);
        int texWidth, texHeight, texNumChannels;
        unsigned char *texData = stbi_load(filename, &texWidth, &texHeight, &texNumChannels, 0);
        if (texData == nullptr) {
            std::cout << "Error: could not load texture " << filename << std::endl;
            return nullptr;
        }

        auto t = std::make_unique<Texture>();
        glGenTextures(1, &(t->_handle));
        // TODO: Do I need this here, or only in rendering?
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, t->_handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D, /*mipmapLevel=*/0, /*textureFormat=*/GL_RGB, texWidth, texHeight, /*legacy=*/0,
            /*sourceFormat=*/GL_RGB, /*sourceDataType=*/GL_UNSIGNED_BYTE, texData);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(texData);

        return t;
    }

    unsigned int _handle = 0;
};

class Material {
public:
    Shader _shader;
    Texture const* _texture;
};

class BoundMesh {
public:
    // ASSUMES 8 VALUES PER VERTEX
    void Init(float* vertexData, int numVertices, Material const* m) {
        _numVerts = numVertices;
        _mat = m;

        int const numValuesPerVertex = 8;

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);
        
        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(
            GL_ARRAY_BUFFER, sizeof(float) * numValuesPerVertex * numVertices, vertexData,
            GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(
            /*attributeIndex=*/0, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false,
            /*stride=*/numValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)0);
        // This vertex attribute will be read from the VBO that is currently bound
        // to GL_ARRAY_BUFFER, which was bound above to "vbo".
        glEnableVertexAttribArray(/*attributeIndex=*/0);

        // Normal attribute
        glVertexAttribPointer(
            /*attributeIndex=*/1, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false,
            /*stride=*/numValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)(3*sizeof(float)));
        // This vertex attribute will be read from the VBO that is currently bound
        // to GL_ARRAY_BUFFER, which was bound above to "vbo".
        glEnableVertexAttribArray(/*attributeIndex=*/1);

        // (s,t) attribute
        glVertexAttribPointer(
            /*attributeIndex=*/2, /*numValues=*/2, /*valueType=*/GL_FLOAT, /*normalized=*/false,
            /*stride=*/numValuesPerVertex*sizeof(float), /*offsetOfFirstValue=*/(void*)(6*sizeof(float)));
        glEnableVertexAttribArray(/*attributeIndex=*/2);
    }

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    int _numVerts = -1;
    Material const* _mat = nullptr;
};