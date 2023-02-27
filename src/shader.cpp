#include "shader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include <glad/glad.h>

bool Shader::Init(const char* vertexPath, const char* fragmentPath)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();		
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();		
    }
    catch(std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        return false;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    
    // vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    };
    
    // similiar for Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    };
    
    // shader Program
    _id = glCreateProgram();
    glAttachShader(_id, vertex);
    glAttachShader(_id, fragment);
    glLinkProgram(_id);
    // print linking errors if any
    glGetProgramiv(_id, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(_id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

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
