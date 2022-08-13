#pragma once

#include "boost/property_tree/ptree.hpp"

using boost::property_tree::ptree;

namespace serial {

class Ptree {
public:
    Ptree AddChild(char const* name);
    Ptree GetChild(char const* name);
    
    void PutString(char const* name, char const* v);
    void PutInt(char const* name, int v);
    void PutLong(char const* name, long v);
    void PutBool(char const* name, bool v);
    void PutFloat(char const* name, float v);
    void PutDouble(char const* name, double v);

    std::string GetString(char const* name);
    // int GetInt(char const* name);
    // bool GetBool(char const* name);
    // float GetFloat(char const* float);
    // double GetDouble(char const* double);

    Ptree(Ptree const& other);
    Ptree& operator=(Ptree const& other);
    static Ptree MakeNew();

    bool IsValid() { return _internal != nullptr; }

    ~Ptree();

    bool WriteToFile(char const* filename);

private:
    Ptree() {}
    ptree* _internal = nullptr;
    bool _owned = false;
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