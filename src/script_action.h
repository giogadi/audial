#pragma once

#include "component.h"
#include "enums/ScriptActionType.h"
#include "components/orbitable.h"

class ScriptAction {
public:
    virtual ~ScriptAction() {}
    virtual ScriptActionType Type() const = 0;
    virtual void Execute(GameManager& g) const = 0;
    virtual void DrawImGui() const {}
};

class ScriptActionDestroyAllPlanets : public ScriptAction {
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
};

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType);
void DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions);