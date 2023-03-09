#pragma once

#include "matrix.h"

// TODO: Do we need a "glDeleteProgram" or something on destruction?

class Shader {
public:
    unsigned int GetId() const { return _id; }

    // reads and builds the shader
    // TODO: if returns false, will leak memory because it doesn't free the shaders. Just quit if this fails
    bool Init(const char* vertexPath, const char* fragmentPath, const char* geometryPath="");
    // use/activate the shader
    void Use() const;
    // utility uniform functions
    // TODO: cache the uniform locations.
    int SetBool(const char* name, bool value) const;  
    int SetInt(const char* name, int value) const;   
    int SetFloat(const char* name, float value) const;
    int SetMat3(const char* name, Mat3 const& mat) const;
    int SetMat4(const char* name, Mat4 const& mat) const;
    int SetVec3(const char* name, Vec3 const& vec) const;
    int SetVec4(const char* name, Vec4 const& vec) const;

    void SetBool(int location, bool value) const;
    void SetInt(int location, int value) const;   
    void SetFloat(int location, float value) const;
    void SetMat3(int location,  Mat3 const& mat) const;
    void SetMat4(int location, Mat4 const& mat) const;
    void SetVec3(int location, Vec3 const& vec) const;
    void SetVec4(int location, Vec4 const& vec) const;

    int GetUniformLocation(const char* name) const;
private:
    unsigned int _id = 0;
    
};
