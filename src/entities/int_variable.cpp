#include "int_variable.h"

#include <sstream>

#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

void IntVariableEntity::InitDerived(GameManager& g) {
    _currentValue = _initialValue;

    _actions.clear();
    _actions.reserve(_actionStrings.size());    
    SeqAction::LoadInputs loadInputs;  // default
    for (std::string const& actionStr : _actionStrings) {
        std::stringstream ss(actionStr);
        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, ss);
        if (pAction != nullptr) {
            pAction->Init(g);
            _actions.push_back(std::move(pAction));   
        }
    }

    _g = &g;
}

void IntVariableEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("initial_value", _initialValue);
    serial::SaveVectorInChildNode(pt, "actions", "action", _actionStrings);   
}

void IntVariableEntity::LoadDerived(serial::Ptree pt) {
    _initialValue = pt.GetInt("initial_value");
    serial::LoadVectorFromChildNode(pt, "actions", _actionStrings);
}

ne::Entity::ImGuiResult IntVariableEntity::ImGuiDerived(GameManager& g) {
    ImGui::InputInt("Initial value", &_initialValue);
    if (ImGui::CollapsingHeader("Actions")) {
        imgui_util::InputVector(_actionStrings);       
    }
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
