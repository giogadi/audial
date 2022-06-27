#pragma once


#include "component.h"
#include "enums/ScriptActionType.h"
#include "components/orbitable.h"

class ScriptAction {
public:
    virtual ~ScriptAction() {}
    virtual ScriptActionType Type() const = 0;
    virtual void Execute(GameManager& g) const = 0;
    virtual void DrawImGui() {}
    virtual void Save(ptree& pt) const {}
    virtual void Load(ptree const& pt) {}

    static void SaveActions(ptree& pt, std::vector<std::unique_ptr<ScriptAction>> const& actions);
    static void LoadActions(ptree const& pt, std::vector<std::unique_ptr<ScriptAction>>& actions);
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

class ScriptActionActivateEntity : public ScriptAction {
public:
    virtual ScriptActionType Type() const override { return ScriptActionType::ActivateEntity; }
    virtual void Execute(GameManager& g) const override {
        EntityId id = g._entityManager->FindInactiveEntityByName(_entityName.c_str());
        g._entityManager->ActivateEntity(id, g);
    }

    virtual void DrawImGui() override;

    void Save(ptree& pt) const override;
    void Load(ptree const& pt) override;

    std::string _entityName;
};

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType);
void DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions);