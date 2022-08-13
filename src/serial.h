#pragma once

#include "boost/property_tree/ptree.hpp"

using boost::property_tree::ptree;

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

    std::string GetStringValue();

    // Returns pointer to allocated memory. Up to user to delete.
    NameTreePair* GetChildren(int* numChildren);

    Ptree(Ptree const& other);
    Ptree& operator=(Ptree const& other);
    static Ptree MakeNew();

    bool IsValid() { return _internal != nullptr; }

    ~Ptree();

    bool WriteToFile(char const* filename);
    bool LoadFromFile(char const* filename);

private:    
    ptree* _internal = nullptr;
    bool _owned = false;
};

struct NameTreePair {    
    char const* _name;
    Ptree _pt;
};

template <typename T>
inline void SaveInNewChildOf(ptree& pt, char const* childName, T const& v) {
    ptree childPt;
    v.Save(childPt);
    pt.add_child(childName, childPt);
}

template <typename T>
inline void SaveInNewChildOf(Ptree pt, char const* childName, T const& v) {
    Ptree childPt = pt.AddChild(childName);
    v.Save(childPt);
}

}