#include "flow_wall.h"

#include <sstream>

#include "serial_vector_util.h"
#include "imgui_vector_util.h"
#include "math_util.h"
#include "renderer.h"

void FlowWallEntity::InitDerived(GameManager& g) {
    _wpFollower.Init(_transform.Pos());
    
    _hitActions.reserve(_hitActionStrings.size());
    SeqAction::LoadInputs loadInputs;  // default
    for (std::string const& actionStr : _hitActionStrings) {
        std::stringstream ss(actionStr);
        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, ss);
        if (pAction != nullptr) {
            pAction->InitBase(g);
            _hitActions.push_back(std::move(pAction));   
        }
    }

    _currentColor = _modelColor;

    _meshId = g._scene->LoadPolygon2d(_polygon);
}

void FlowWallEntity::Update(GameManager& g, float dt) {

    if (!g._editMode) {
        Vec3 newPos;
        bool updatePos = _wpFollower.Update(g, dt, &newPos);
        if (updatePos) {
            _transform.SetPos(newPos);
        }
    }

    // Maybe update color from getting hit
    // turn to FadeColor on hit, then fade back to regular color
    if (_timeOfLastHit >= 0.0) {
        double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
        double constexpr kFadeTime = 0.25;
        double fadeFactor = (beatTime - _timeOfLastHit) / kFadeTime;        
        fadeFactor = math_util::Clamp(fadeFactor, 0.0, 1.0);
        Vec4 constexpr kFadeColor(0.f, 0.f, 0.f, 1.f);
        if (fadeFactor == 1.0) {
            _timeOfLastHit = -1.0;
        }
        _currentColor = kFadeColor + fadeFactor * (_modelColor - kFadeColor);
    }

    if (renderer::ColorModelInstance* model = g._scene->DrawMesh(_meshId)) {
        model->_transform = _transform.Mat4Scale();
        model->_color = _currentColor;
    } else if (_model != nullptr) {
        g._scene->DrawMesh(_model, _transform.Mat4Scale(), _currentColor);
    }

}

void FlowWallEntity::SaveDerived(serial::Ptree pt) const {
    _wpFollower.Save(pt);
    serial::SaveVectorInChildNode(pt, "polygon", "point", _polygon);
    serial::SaveVectorInChildNode(pt, "hit_actions", "action", _hitActionStrings);   
}

void FlowWallEntity::LoadDerived(serial::Ptree pt) {
    _wpFollower.Load(pt);
    serial::LoadVectorFromChildNode(pt, "polygon", _polygon);
    serial::LoadVectorFromChildNode(pt, "hit_actions", _hitActionStrings);
}

FlowWallEntity::ImGuiResult FlowWallEntity::ImGuiDerived(GameManager& g)  {
    ImGuiResult result = ImGuiResult::Done;
    
    if (ImGui::CollapsingHeader("Waypoints")) {
        _wpFollower.ImGui();
    }

    if (ImGui::CollapsingHeader("Polygon")) {
        imgui_util::InputVector(_polygon);        
    }

    if (ImGui::CollapsingHeader("Hit actions")) {
        bool needsInit = imgui_util::InputVector(_hitActionStrings);
        if (needsInit) {
            result = ImGuiResult::NeedsInit;
        }
    }
    
    return result;
}

void FlowWallEntity::OnHit(GameManager& g) {
    _timeOfLastHit = g._beatClock->GetBeatTimeFromEpoch();
    for (auto const& pAction : _hitActions) {
        pAction->ExecuteBase(g);
    }

}

void FlowWallEntity::OnEditPick(GameManager& g) {
    OnHit(g);
}
