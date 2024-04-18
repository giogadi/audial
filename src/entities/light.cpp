#include "entities/light.h"

#include "game_manager.h"
#include "renderer.h"
#include "imgui_util.h"
#include "serial.h"

void LightEntity::DebugPrint() {
    ne::Entity::DebugPrint();
    // printf("ambient: %f %f %f\n", _ambient._x, _ambient._y, _ambient._z);
    // printf("diffuse: %f %f %f\n", _diffuse._x, _diffuse._y, _diffuse._z);
}
void LightEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "color", _color);
    pt.PutFloat("ambient", _ambient);
    pt.PutFloat("diffuse", _diffuse);
    pt.PutBool("is_directional", _isDirectional);
}
void LightEntity::LoadDerived(serial::Ptree pt) {
    if (pt.GetVersion() >= 5) {
        serial::LoadFromChildOf(pt, "color", _color);
        _ambient = pt.GetFloat("ambient");
        _diffuse = pt.GetFloat("diffuse");
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
    _isDirectional = false;
    pt.TryGetBool("is_directional", &_isDirectional);
}
ne::Entity::ImGuiResult LightEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Directional", &_isDirectional);
    imgui_util::ColorEdit3("Color", &_color);
    ImGui::SliderFloat("Diffuse", &_diffuse, 0.f, 1.f);
    ImGui::SliderFloat("Ambient", &_ambient, 0.f, 1.f);
    return ImGuiResult::Done;
}
void LightEntity::InitDerived(GameManager& g) {
    
}
void LightEntity::UpdateDerived(GameManager& g, float dt) {
    renderer::Light* l = g._scene->DrawLight();
    l->_isDirectional = _isDirectional;
    if (_isDirectional) {
        l->_p = _transform.GetZAxis();
    } else {
        l->_p = _transform.Pos();
    }
    l->_color = _color;
    l->_ambient = _ambient;
    l->_diffuse = _diffuse;   
}
void LightEntity::Destroy(GameManager& g) {
}
