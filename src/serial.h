#pragma once

#include <string>

namespace serial {

struct NameTreePair;

class Ptree {
public:
    Ptree() {}

    Ptree AddChild(char const* name);

    // Crashes if it fails.
    Ptree GetChild(char const* name);

    // Returns invalid tree if it fails.
    Ptree TryGetChild(char const* name);
    
    void PutString(char const* name, char const* v);
    void PutInt(char const* name, int v);
    void PutLong(char const* name, long v);
    void PutBool(char const* name, bool v);
    void PutFloat(char const* name, float v);
    void PutDouble(char const* name, double v);

    bool TryGetString(char const* name, std::string* v);
    bool TryGetInt(char const* name, int* v);
    bool TryGetLong(char const* name, long* v);
    bool TryGetBool(char const* name, bool* v);
    bool TryGetFloat(char const* name, float* v);
    bool TryGetDouble(char const* name, double* v);

    std::string GetString(char const* name);
    int GetInt(char const* name);
    long GetLong(char const* name);
    bool GetBool(char const* name);
    float GetFloat(char const* name);
    double GetDouble(char const* name);

    void PutStringValue(char const* value);
    void PutIntValue(int value);
    void PutBoolValue(bool value);
    void PutFloatValue(float value);
    void PutDoubleValue(double value);
    
    
    std::string GetStringValue();
    int GetIntValue();
    bool GetBoolValue();
    float GetFloatValue();
    double GetDoubleValue();

    // Returns pointer to allocated memory. Up to user to delete.
    NameTreePair* GetChildren(int* numChildren);

    Ptree(Ptree const& other);
    Ptree& operator=(Ptree const& other);
    static Ptree MakeNew();
    void DeleteData();

    bool IsValid() { return _internal != nullptr; }

    bool WriteToFile(char const* filename);
    bool LoadFromFile(char const* filename);

private:
    void* _internal = nullptr;
};

struct NameTreePair {    
    char const* _name = nullptr;
    Ptree _pt;
};

template <typename T>
inline void SaveInNewChildOf(Ptree pt, char const* childName, T const& v) {
    Ptree childPt = pt.AddChild(childName);
    v.Save(childPt);
}

template <typename T>
inline bool LoadFromChildOf(Ptree pt, char const* childName, T& v) {
    Ptree childPt = pt.TryGetChild(childName);
    if (childPt.IsValid()) {
        v.Load(childPt);
        return true;
    } else {
        return false;
    }
}

template <typename E>
inline void PutEnum(Ptree pt, char const* name, E const v) {
    char const* vStr = EnumToString(v);
    pt.PutString(name, vStr);
}

template <typename E>
inline bool TryGetEnum(Ptree pt, char const* name, E& v) {
    std::string vStr;
    bool found = pt.TryGetString(name, &vStr);
    if (!found) {
        return false;
    }
    StringToEnum(vStr.c_str(), v);
    return true;
}

}  // namespace serial
