#pragma once

class Shader {
public:
    unsigned int GetId() const { return _id; }

    // reads and builds the shader
    // TODO: if returns false, will leak memory because it doesn't free the shaders. Just quit if this fails
    bool Init(const char* vertexPath, const char* fragmentPath);
    // use/activate the shader
    void Use();
    // utility uniform functions
    // TODO: cache the uniform locations.
    void SetBool(const char* name, bool value) const;  
    void SetInt(const char* name, int value) const;   
    void SetFloat(const char* name, float value) const;
private:
    unsigned int _id = 0;
};