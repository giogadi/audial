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
        return true;
    }

    virtual void Destroy() override {
        for (auto const& action : _actions) {
            action->Execute(*_g);
        }
    }

    virtual void EditDestroy() override {}

    virtual bool DrawImGui() override {
        DrawScriptActionListImGui(_actions);
        return false;  // no reconnect
    }

    virtual void Save(ptree& pt) const override {
        ScriptAction::SaveActions(pt.add_child("script_actions", ptree()), _actions);
    }
    virtual void Save(serial::Ptree pt) const override {
        serial::Ptree actionsPt = pt.AddChild("script_actions");
        ScriptAction::SaveActions(actionsPt, _actions);
    }
    virtual void Load(ptree const& pt) override {
        ScriptAction::LoadActions(pt.get_child("script_actions"), _actions);
    }

    // Serialized
    std::vector<std::unique_ptr<ScriptAction>> _actions;

    // Non-serialized
    GameManager* _g;
};