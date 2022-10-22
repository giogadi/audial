#pragma once

#include "component.h"
#include "script_action.h"

// Ugh, kinda a waste of empty Update() for the whole entity's life until it
// gets destroyed. whatever.
class OnDestroyEventComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::OnDestroyEvent; }
    OnDestroyEventComponent() {}
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override {
        _g = &g;
        for (auto& action : _actions) {
            action->Init(id, g);
        }
        return true;
    }

    virtual void Destroy() override {
        for (auto const& action : _actions) {
            action->Execute(*_g);
        }
    }

    virtual void EditDestroy() override {}

    virtual bool DrawImGui() override {
        return DrawScriptActionListImGui(_actions);
    }

    virtual void Save(serial::Ptree pt) const override {
        serial::Ptree actionsPt = pt.AddChild("script_actions");
        ScriptAction::SaveActions(actionsPt, _actions);
    }

    virtual void Load(serial::Ptree pt) override {
        ScriptAction::LoadActions(pt.GetChild("script_actions"), _actions);
    }

    // Serialized
    std::vector<std::unique_ptr<ScriptAction>> _actions;

    // Non-serialized
    GameManager* _g;
};