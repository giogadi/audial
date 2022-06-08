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

    virtual bool DrawImGui() override {
        DrawScriptActionListImGui(_actions);
        return false;  // no reconnect
    }

    // Serialized
    std::vector<std::unique_ptr<ScriptAction>> _actions;

    // Non-serialized
    GameManager* _g;
};