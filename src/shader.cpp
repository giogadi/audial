#include "shader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include <glad/glad.h>

bool LoadShader(char const* shaderPath, GLenum shaderType, unsigned int* shaderId) {
    assert(shaderId != nullptr);
    std::stringstream shaderStream;
    {
        std::ifstream shaderFile(shaderPath);
        if (!shaderFile.is_open()) {
            printf("Failed to read shader file at \"%s\"\n", shaderPath);
            return false;
        }
        shaderStream << shaderFile.rdbuf();
    }
    std::string shaderCode = shaderStream.str();
    char const* shaderCodeCStr = shaderCode.c_str();

    *shaderId = glCreateShader(shaderType);
    glShaderSource(*shaderId, 1, &shaderCodeCStr, NULL);
    glCompileShader(*shaderId);
    int success;
    glGetShaderiv(*shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(*shaderId, 512, NULL, infoLog);
        printf("ERROR: compilation of shader \"%s\" failed: %s\n", shaderPath, infoLog);
        return false;
    }

    return true;
}

bool Shader::Init(char const* vertexPath, char const* fragmentPath, char const* geometryPath) {
    unsigned int vertexId;
    bool success = LoadShader(vertexPath, GL_VERTEX_SHADER, &vertexId);
    if (!success) {
        return false;
    }
    bool hasGeometryShader = false;
    unsigned int geometryId;
    if (geometryPath[0] != '\0') {
        success = LoadShader(geometryPath, GL_GEOMETRY_SHADER, &geometryId);
        if (!success) {
            return false;
        }
        hasGeometryShader = true;
    }
    unsigned int fragmentId;
    success = LoadShader(fragmentPath, GL_FRAGMENT_SHADER, &fragmentId);
    if (!success) {
        return false;
    }

    _id = glCreateProgram();
    glAttachShader(_id, vertexId);
    if (hasGeometryShader) {
        glAttachShader(_id, geometryId);
    }
    glAttachShader(_id, fragmentId);
    glLinkProgram(_id);
    // print linking errors if any
    int linkSuccess;
    glGetProgramiv(_id, GL_LINK_STATUS, &linkSuccess);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(_id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexId);
    if (hasGeometryShader) {
        glDeleteShader(geometryId);
    }
    glDeleteShader(fragmentId);

    return true;
}

void Shader::Use() const {
    glUseProgram(_id);
}

int Shader::GetUniformLocation(char const* name) const {
    int loc = glGetUniformLocation(_id, name);
    if (loc < 0) {
        printf("UNRECOGNIZED UNIFORM NAME \"%s\"\n", name);
    }
    return loc;
}

int Shader::SetBool(const char* name, bool value) const
{
    int location = glGetUniformLocation(_id, name);
    SetBool(location, value);
    return location;

}
int Shader::SetInt(const char* name, int value) const
{
    int location = glGetUniformLocation(_id, name);
    SetInt(location, value);
    return location;
}
int Shader::SetFloat(const char* name, float value) const
{
    int location = glGetUniformLocation(_id, name);
    SetFloat(location, value);
    return location;
}
int Shader::SetMat3(const char* name, Mat3 const& mat) const {
    int location = glGetUniformLocation(_id, name);
    SetMat3(location, mat);
    return location;
}
int Shader::SetMat4(const char* name, Mat4 const& mat) const {
    int location = glGetUniformLocation(_id, name);
    SetMat4(location, mat);
    return location;
}
int Shader::SetVec3(const char* name, Vec3 const& vec) const {
    int location = glGetUniformLocation(_id, name);
    SetVec3(location, vec);
    return location;
}
int Shader::SetVec4(const char* name, Vec4 const& vec) const {
    int location = glGetUniformLocation(_id, name);
    SetVec4(location, vec);
    return location;
}

void Shader::SetBool(int location, bool value) const
{         
    glUniform1i(location, (int)value); 
}
void Shader::SetInt(int location, int value) const
{ 
    glUniform1i(location, value); 
}
void Shader::SetFloat(int location, float value) const
{ 
    glUniform1f(location, value); 
}
void Shader::SetMat3(int location, Mat3 const& mat) const {
    glUniformMatrix3fv(
        location, /*count=*/1, /*transpose=*/GL_FALSE, mat._data);
}
void Shader::SetMat4(int location, Mat4 const& mat) const {
    glUniformMatrix4fv(
        location, /*count=*/1, /*transpose=*/GL_FALSE, mat._data);
}
void Shader::SetVec3(int location, Vec3 const& vec) const {
    float data[3];
    data[0] = vec._x;
    data[1] = vec._y;
    data[2] = vec._z;
    glUniform3fv(
        location, /*count=*/1, data);
}
void Shader::SetVec4(int location, Vec4 const& vec) const {
    float data[4];
    data[0] = vec._x;
    data[1] = vec._y;
    data[2] = vec._z;
    data[3] = vec._w;
    glUniform4fv(
        location, /*count=*/1, data);
}
