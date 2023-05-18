#include "int_variable.h"

#include <sstream>

#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

void IntVariableEntity::InitDerived(GameManager& g) {
    _currentValue = _initialValue;

    for (auto const& pAction : _actions) {
        pAction->Init(g);
    }   

    _g = &g;
}

void IntVariableEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("initial_value", _initialValue);
    SeqAction::SaveActionsInChildNode(pt, "actions", _actions);
}

void IntVariableEntity::LoadDerived(serial::Ptree pt) {
    _initialValue = pt.GetInt("initial_value");
    SeqAction::LoadActionsFromChildNode(pt, "actions", _actions);
}

ne::Entity::ImGuiResult IntVariableEntity::ImGuiDerived(GameManager& g) {
    ImGui::InputInt("Initial value", &_initialValue);
    SeqAction::ImGui("Actions on zero", _actions);
    return ne::Entity::ImGuiResult::Done;
}

void IntVariableEntity::AddToVariable(int amount) {
    bool wasZero = _currentValue == 0;
    _currentValue += amount;

    if (!wasZero && _currentValue == 0) {
        for (auto const& pAction : _actions) {
            pAction->Execute(*_g);
        }
    }
}
