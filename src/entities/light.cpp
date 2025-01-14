#include "entities/light.h"

#include "game_manager.h"
#include "renderer.h"
#include "imgui_util.h"
#include "serial.h"

void LightEntity::DebugPrint() {
    ne::Entity::DebugPrint();
}
void LightEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "color", _color);
    pt.PutFloat("ambient", _ambient);
    pt.PutFloat("diffuse", _diffuse);
    pt.PutFloat("specular", _specular);
    pt.PutFloat("range", _range);
    pt.PutBool("is_directional", _isDirectional);
    pt.PutBool("shadows", _shadows);
    pt.PutFloat("zn", _zn);
    pt.PutFloat("zf", _zf);
    pt.PutFloat("w", _width);
}
void LightEntity::LoadDerived(serial::Ptree pt) {
    _color = Vec3();
    _ambient = 0.f;
    _diffuse = 0.f;
    _specular = 0.f;
    _range = 32.f;
    if (pt.GetVersion() >= 5) {
        serial::LoadFromChildOf(pt, "color", _color);
        _ambient = pt.GetFloat("ambient");
        _diffuse = pt.GetFloat("diffuse");
        pt.TryGetFloat("specular", &_specular);
    } else {
        // Assume color is white
        _color.Set(1.f, 1.f, 1.f);

        Vec3 ambVec;
        serial::LoadFromChildOf(pt, "ambient", ambVec);
        _ambient = (ambVec._x + ambVec._y + ambVec._z) / 3.f;

        Vec3 difVec;
        serial::LoadFromChildOf(pt, "diffuse", difVec);
        _diffuse = (difVec._x + difVec._y + difVec._z) / 3.f;
    }
    pt.TryGetFloat("range", &_range);
    _isDirectional = false;
    pt.TryGetBool("is_directional", &_isDirectional);
    _shadows = false;
    pt.TryGetBool("shadows", &_shadows);
    pt.TryGetFloat("zn", &_zn);
    pt.TryGetFloat("zf", &_zf);
    pt.TryGetFloat("w", &_width);

}
ne::Entity::ImGuiResult LightEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Directional", &_isDirectional);
    ImGui::Checkbox("Shadows", &_shadows);
    imgui_util::ColorEdit3("Color", &_color);
    ImGui::SliderFloat("Diffuse", &_diffuse, 0.f, 1.f);
    ImGui::SliderFloat("Ambient", &_ambient, 0.f, 1.f);
    ImGui::SliderFloat("Specular", &_specular, 0.f, 1.f);
    ImGui::SliderFloat("Range (point only)", &_range, 0.1f, 100.f);
    ImGui::InputFloat("zn", &_zn);
    ImGui::InputFloat("zf", &_zf);
    ImGui::InputFloat("w", &_width);
    return ImGuiResult::Done;
}
void LightEntity::InitDerived(GameManager& g) {
    
}
void LightEntity::UpdateDerived(GameManager& g, float dt) {
    renderer::Light* l = g._scene->DrawLight();
    l->_isDirectional = _isDirectional;
    l->_p = _transform.Pos();
    l->_dir = _transform.GetZAxis();
    l->_color = _color;
    l->_ambient = _ambient;
    l->_diffuse = _diffuse;   
    l->_specular = _specular;
    l->_range = _range;
    l->_shadows = _shadows;
    l->_zn = _zn;
    l->_zf = _zf;
    l->_width = _width;
}
void LightEntity::Draw(GameManager &g, float dt) {
    if (_model != nullptr) {
        Mat4 const& mat = _transform.Mat4Scale();
        renderer::ModelInstance* m = g._scene->DrawTexturedMesh(_model, _textureId);
        m->_transform = mat;
        m->_color = _modelColor;
        m->_lightFactor = 0.f;
    }
}
void LightEntity::Destroy(GameManager& g) {
}
