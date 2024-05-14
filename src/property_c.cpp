#include "property.h"

#include "imgui/imgui.h"

#include "imgui_util.h"

#include "matrix.h"

void LoadProperties(Property const* props, size_t numProperties, serial::Ptree pt, void* object) {
    for (int ii = 0; ii < numProperties; ++ii) {
        Property const* prop = &props[ii];
        serial::Ptree childPt = pt.TryGetChild(prop->name);
        if (childPt.IsValid()) {
            void* v = (char*)object + prop->offset;
            prop->spec->loadFn(v, childPt); 
        }
    }
}
void SaveProperties(Property const* props, size_t numProperties, serial::Ptree pt, void const* object) {
    for (int ii = 0; ii < numProperties; ++ii) {
        Property const* prop = &props[ii];
        serial::Ptree childPt = pt.AddChild(prop->name);
        if (!childPt.IsValid()) {
            printf("WTFFF CHILD PT NOT VALID AFTER ADD_CHILD?\n");
            continue;
        }
        void const* v = (char const*) object + prop->offset;
        prop->spec->saveFn(v, childPt);
    }
}



void PropertyIntLoad(void* vIn, serial::Ptree pt) {
    int* v = static_cast<int*>(vIn);
    *v = pt.GetIntValue(); 
}
void PropertyIntSave(void const* vIn, serial::Ptree pt) {
    int const* v = static_cast<int const*>(vIn);
    pt.PutIntValue(*v);
}
bool PropertyIntImGui(char const* name, void* vIn) {
    int* v = static_cast<int*>(vIn);
    return ImGui::InputInt(name, v);
}
PropertySpec const gPropertyIntSpec {
    .loadFn = &PropertyIntLoad,
    .saveFn = &PropertyIntSave,
    .imguiFn = &PropertyIntImGui
};

// Int64
void PropertyInt64Load(void* vIn, serial::Ptree pt) {
    int64_t* v = static_cast<int64_t*>(vIn);
    *v = pt.GetInt64Value(); 
}
void PropertyInt64Save(void const* vIn, serial::Ptree pt) {
    int64_t const* v = static_cast<int64_t const*>(vIn);
    pt.PutInt64Value(*v);
}
bool PropertyInt64ImGui(char const* name, void* vIn) {
    int64_t* v = static_cast<int64_t*>(vIn);
    return ImGui::InputScalar(name, ImGuiDataType_S64, v);
}
PropertySpec const gPropertyInt64Spec {
    .loadFn = &PropertyInt64Load,
    .saveFn = &PropertyInt64Save,
    .imguiFn = &PropertyInt64ImGui
};

// Float
void PropertyFloatLoad(void* vIn, serial::Ptree pt) {
    float* v = static_cast<float*>(vIn);
    *v = pt.GetFloatValue(); 
}
void PropertyFloatSave(void const* vIn, serial::Ptree pt) {
    float const* v = static_cast<float const*>(vIn);
    pt.PutFloatValue(*v);
}
bool PropertyFloatImGui(char const* name, void* vIn) {
    float* v = static_cast<float*>(vIn);
    return ImGui::InputFloat(name, v);
}
PropertySpec const gPropertyFloatSpec {
    .loadFn = &PropertyFloatLoad,
    .saveFn = &PropertyFloatSave,
    .imguiFn = &PropertyFloatImGui
};

// Double
void PropertyDoubleLoad(void* vIn, serial::Ptree pt) {
    double* v = static_cast<double*>(vIn);
    *v = pt.GetDoubleValue();
}
void PropertyDoubleSave(void const* vIn, serial::Ptree pt) {
    double const* v = static_cast<double const*>(vIn);
    pt.PutDoubleValue(*v);
}
bool PropertyDoubleImGui(char const* name, void* vIn) {
    double* v = static_cast<double*>(vIn);
    return ImGui::InputDouble(name, v);
}
PropertySpec const gPropertyDoubleSpec {
    .loadFn = &PropertyDoubleLoad,
    .saveFn = &PropertyDoubleSave,
    .imguiFn = &PropertyDoubleImGui
};

// Bool
void PropertyBoolLoad(void* vIn, serial::Ptree pt) {
    bool* v = static_cast<bool*>(vIn);
    *v = pt.GetBoolValue(); 
}
void PropertyBoolSave(void const* vIn, serial::Ptree pt) {
    bool const* v = static_cast<bool const*>(vIn);
    pt.PutBoolValue(*v);
}
bool PropertyBoolImGui(char const* name, void* vIn) {
    bool* v = static_cast<bool*>(vIn);
    return ImGui::Checkbox(name, v);
}
PropertySpec const gPropertyBoolSpec {
    .loadFn = &PropertyBoolLoad,
    .saveFn = &PropertyBoolSave,
    .imguiFn = &PropertyBoolImGui
};

// String
void PropertyStringLoad(void* vIn, serial::Ptree pt) {
    std::string* v = static_cast<std::string*>(vIn);
    *v = pt.GetStringValue(); 
}
void PropertyStringSave(void const* vIn, serial::Ptree pt) {
    std::string const* v = static_cast<std::string const*>(vIn);
    pt.PutStringValue(v->c_str());
}
bool PropertyString32ImGui(char const* name, void* vIn) {
    std::string* v = static_cast<std::string*>(vIn);
    return imgui_util::InputText<32>(name, v);
}
bool PropertyString128ImGui(char const* name, void* vIn) {
    std::string* v = static_cast<std::string*>(vIn);
    return imgui_util::InputText<128>(name, v);
}
PropertySpec const gPropertyString32Spec {
    .loadFn = &PropertyStringLoad,
    .saveFn = &PropertyStringSave,
    .imguiFn = &PropertyString32ImGui
};
PropertySpec const gPropertyString128Spec {
    .loadFn = &PropertyStringLoad,
    .saveFn = &PropertyStringSave,
    .imguiFn = &PropertyString128ImGui
};

// Vec3
void PropertyVec3Load(void* vIn, serial::Ptree pt) {
    Vec3* v = static_cast<Vec3*>(vIn);
    v->Load(pt);
}
void PropertyVec3Save(void const* vIn, serial::Ptree pt) {
    Vec3 const* v = static_cast<Vec3 const*>(vIn);
    v->Save(pt);
}
bool PropertyVec3ImGui(char const* name, void* vIn) {
    Vec3* v = static_cast<Vec3*>(vIn);
    return imgui_util::InputVec3(name, v);
}
PropertySpec const gPropertyVec3Spec {
    .loadFn = &PropertyVec3Load,
    .saveFn = &PropertyVec3Save,
    .imguiFn = &PropertyVec3ImGui
};

// Color4
void PropertyVec4Load(void* vIn, serial::Ptree pt) {
    Vec4* v = static_cast<Vec4*>(vIn);
    v->Load(pt);
}
void PropertyVec4Save(void const* vIn, serial::Ptree pt) {
    Vec4 const* v = static_cast<Vec4 const*>(vIn);
    v->Save(pt);
}
bool PropertyColor4ImGui(char const* name, void* vIn) {
    Vec4* v = static_cast<Vec4*>(vIn);
    return imgui_util::ColorEdit4(name, v);
}
PropertySpec const gPropertyColor4Spec {
    .loadFn = &PropertyVec4Load,
    .saveFn = &PropertyVec4Save,
    .imguiFn = &PropertyColor4ImGui
};
