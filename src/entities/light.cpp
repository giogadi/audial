#include "entities/light.h"

#include "game_manager.h"
#include "renderer.h"
#include "imgui_util.h"
#include "serial.h"

void LightEntity::DebugPrint() {
    ne::Entity::DebugPrint();
    printf("ambient: %f %f %f\n", _ambient._x, _ambient._y, _ambient._z);
    printf("diffuse: %f %f %f\n", _diffuse._x, _diffuse._y, _diffuse._z);
}
void LightEntity::SaveDerived(serial::Ptree pt) const {
    serial::SaveInNewChildOf(pt, "ambient", _ambient);
    serial::SaveInNewChildOf(pt, "diffuse", _diffuse);
    pt.PutBool("is_directional", _isDirectional);
}
void LightEntity::LoadDerived(serial::Ptree pt) {
    _ambient.Load(pt.GetChild("ambient"));
    _diffuse.Load(pt.GetChild("diffuse"));
    _isDirectional = false;
    pt.TryGetBool("is_directional", &_isDirectional);
}
ne::Entity::ImGuiResult LightEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Directional", &_isDirectional);
    imgui_util::ColorEdit3("Diffuse", &_diffuse);
    imgui_util::ColorEdit3("Ambient", &_ambient);
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
    l->_ambient = _ambient;
    l->_diffuse = _diffuse;   
}
void LightEntity::Destroy(GameManager& g) {
}
