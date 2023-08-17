#include "int_variable.h"

#include <sstream>

#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"
#include "renderer.h"
#include "math_util.h"

void IntVariableEntity::InitDerived(GameManager& g) {
    _currentValue = _initialValue;
    _beatTimeOfLastAdd = -1.0;

    for (auto const& pAction : _actions) {
        pAction->Init(g);
    }   

    _g = &g;
}

void IntVariableEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("initial_value", _initialValue);
    pt.PutBool("draw_counter", _drawCounter);
    SeqAction::SaveActionsInChildNode(pt, "actions", _actions);
}

void IntVariableEntity::LoadDerived(serial::Ptree pt) {
    _initialValue = pt.GetInt("initial_value");
    _drawCounter = false;
    pt.TryGetBool("draw_counter", &_drawCounter);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _actions);
}

ne::Entity::ImGuiResult IntVariableEntity::ImGuiDerived(GameManager& g) {
    ImGui::InputInt("Initial value", &_initialValue);
    ImGui::Checkbox("Draw", &_drawCounter);
    SeqAction::ImGui("Actions on zero", _actions);
    return ne::Entity::ImGuiResult::Done;
}

void IntVariableEntity::Draw(GameManager& g, float dt) {
    if (_drawCounter) {
        DrawCounter(g);
    }
}

void IntVariableEntity::AddToVariable(int amount) {
    bool wasZero = _currentValue == 0;
    _currentValue += amount;
    _beatTimeOfLastAdd = _g->_beatClock->GetBeatTimeFromEpoch();

    if (!wasZero && _currentValue == 0) {
        for (auto const& pAction : _actions) {
            pAction->Execute(*_g);
        }
    }
}

void IntVariableEntity::DrawCounter(GameManager& g) {
    int numSteps = _initialValue;
    float cellWidth = 0.25f;
    float spaceBetweenCells = 0.25f;
    float totalWidth = numSteps * cellWidth + (spaceBetweenCells * (numSteps - 1));
    Transform t;
    t.SetScale(Vec3(cellWidth, cellWidth, cellWidth));
    Vec3 pos = _transform.Pos();
    pos._x -= 0.5f * totalWidth;
    pos._x += 0.5f * cellWidth;
    int numOn = _initialValue - _currentValue;
    Vec4 constexpr kNoHitColor = Vec4(0.245098054f, 0.933390915f, 1.f, 1.f);
    Vec4 constexpr kHitColor = Vec4(0.933390915f, 0.245098054f, 1.f, 1.f);

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double beatTimeSinceLastAdd = std::numeric_limits<double>::max();
    double constexpr kAnimTime = 0.5f;
    float constexpr kNewlyActiveExtraScale = 2.f;
    float newlyActiveScaleFactor = 1.f;
    float alpha = _modelColor._w;
    if (_currentValue == 0) {
        alpha = 0.f;
    }
    if (_beatTimeOfLastAdd >= 0.0) {
        beatTimeSinceLastAdd = beatTime - _beatTimeOfLastAdd;
        if (beatTimeSinceLastAdd <= kAnimTime) {
            float factor = math_util::Clamp(beatTimeSinceLastAdd / kAnimTime, 0.0, 1.0);
            float scaleFactor = math_util::Triangle(factor);
            newlyActiveScaleFactor += scaleFactor * kNewlyActiveExtraScale;

            if (_currentValue == 0) {
                alpha = _modelColor._w + factor * (0.f - _modelColor._w);
            }
        } else {
            _beatTimeOfLastAdd = -1.0;
        }
    }
    for (int i = 0; i < numSteps; ++i) {
        Vec4 color;
        if (i < numOn) {
            color = kHitColor;
            t.SetScale(Vec3(cellWidth, cellWidth, cellWidth) * newlyActiveScaleFactor);
        } else {
            color = kNoHitColor;
            t.SetScale(Vec3(cellWidth, cellWidth, cellWidth));
        }
        color._w = alpha;

        t.SetTranslation(pos);
        renderer::ColorModelInstance& instance = g._scene->DrawCube(t.Mat4Scale(), color);
        instance._topLayer = true;
        pos._x += cellWidth + spaceBetweenCells;
        
    }
}
