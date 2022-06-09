#pragma once

#include "cereal/cereal.hpp"

#include "component.h"
#include "enums/ScriptActionType.h"
#include "enums/ScriptActionType_cereal.h"
#include "components/orbitable.h"

class ScriptAction {
public:
    virtual ~ScriptAction() {}
    virtual ScriptActionType Type() const = 0;
    virtual void Execute(GameManager& g) const = 0;
    virtual void DrawImGui() const {}
};

class ScriptActionDestroyAllPlanets : public ScriptAction {
public:
    virtual ScriptActionType Type() const override { return ScriptActionType::DestroyAllPlanets; }
    virtual void Execute(GameManager& g) const override {
        g._entityManager->ForEveryActiveEntity([&g](EntityId id) {
            Entity* entity = g._entityManager->GetEntity(id);
            bool hasPlanet = !entity->FindComponentOfType<OrbitableComponent>().expired();
            if (hasPlanet) {
                g._entityManager->TagEntityForDestroy(id);
            }            
        });        
    }

    template<typename Archive>
    void serialize(Archive& ar) {}
};

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType);
void DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions);

template<typename Archive>
inline void SaveActions(Archive& ar, std::vector<std::unique_ptr<ScriptAction>> const& actions) {
    ar(cereal::make_nvp("num_actions", actions.size()));
    for (std::unique_ptr<ScriptAction> const& action : actions) {
        ar(cereal::make_nvp("action_type", action->Type()));
        switch (action->Type()) {
            case ScriptActionType::DestroyAllPlanets: {
                auto const* pAction = dynamic_cast<ScriptActionDestroyAllPlanets const*>(action.get());
                ar(cereal::make_nvp("action", *pAction));
                break;
            }
            case ScriptActionType::Count: {
                assert(false);
            }
        }
    }
}

template<typename Archive>
inline void LoadActions(Archive& ar, std::vector<std::unique_ptr<ScriptAction>>& actions) {
    int numActions;
    ar(cereal::make_nvp("num_actions", numActions));
    for (int i = 0; i < numActions; ++i) {
        ScriptActionType actionType;
        ar(cereal::make_nvp("action_type", actionType));
        std::unique_ptr<ScriptAction> pBaseAction = MakeScriptActionOfType(actionType);
        switch (actionType) {
            case ScriptActionType::DestroyAllPlanets: {
                auto* pAction = dynamic_cast<ScriptActionDestroyAllPlanets*>(pBaseAction.get());
                ar(cereal::make_nvp("action", *pAction));
                actions.push_back(std::move(pBaseAction));
                break;
            }
            case ScriptActionType::Count:
                assert(false);
        }
    }
}