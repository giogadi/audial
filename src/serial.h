#pragma once

#include <string>

namespace serial {

struct NameTreePair;

class Ptree {
public:
    Ptree() {}

    int GetVersion() const { return _version; }

    Ptree AddChild(char const* name);

    // Crashes if it fails.
    Ptree GetChild(char const* name);

    // Returns invalid tree if it fails.
    Ptree TryGetChild(char const* name);
    
    void PutString(char const* name, char const* v);
    void PutChar(char const* name, char v);
    void PutInt(char const* name, int v);
    void PutInt64(char const* name, int64_t v);
    void PutBool(char const* name, bool v);
    void PutFloat(char const* name, float v);
    void PutDouble(char const* name, double v);

    bool TryGetString(char const* name, std::string* v);
    bool TryGetChar(char const* name, char* v);
    bool TryGetInt(char const* name, int* v);
    bool TryGetInt64(char const* name, int64_t* v);
    bool TryGetBool(char const* name, bool* v);
    bool TryGetFloat(char const* name, float* v);
    bool TryGetDouble(char const* name, double* v);

    std::string GetString(char const* name);
    int GetInt(char const* name);
    int64_t GetInt64(char const* name);
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
    int64_t GetInt64Value();
    bool GetBoolValue();
    float GetFloatValue();
    double GetDoubleValue();

    // Returns pointer to allocated memory. Up to user to delete[].
    NameTreePair* GetChildren(int* numChildren);

    Ptree(Ptree const& other);
    Ptree& operator=(Ptree const& other);
    static Ptree MakeNew();
    void DeleteData();

    bool IsValid() { return _internal != nullptr; }

    bool WriteToFile(char const* filename);
    bool LoadFromFile(char const* filename);

private:
    int _version = 0;
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

template <typename T>
inline bool SaveToFile(char const* filename, char const* rootName, T const& v) {
    serial::Ptree pt = serial::Ptree::MakeNew();
    serial::Ptree root = pt.AddChild(rootName);
    v.Save(root);
    bool success = pt.WriteToFile(filename);
    pt.DeleteData();
    if (!success) {
        printf("Failed to write pt to \"%s\"\n", filename);
    }
    return success;
}

template <typename T>
inline bool LoadFromFile(char const* filename, T& v) {
    serial::Ptree pt = serial::Ptree::MakeNew();
    if (!pt.LoadFromFile(filename)) {
        pt.DeleteData();
        printf("Failed to load pt from file \"%s\"\n", filename);
        return false;
    }
    // Assume a single root node. Inside that is what we load from.
    int numChildren = 0;
    serial::NameTreePair* children = pt.GetChildren(&numChildren);
    if (numChildren != 1) {
        printf("serial::LoadFromFile: expected exactly one root, but found %d\n", numChildren);
        delete[] children;
        pt.DeleteData();
        return false;
    }
    v.Load(children->_pt);
    delete[] children;
    pt.DeleteData();
    return true;
}

}  // namespace serial
