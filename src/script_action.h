#pragma once


#include "component.h"
#include "enums/ScriptActionType.h"
#include "components/orbitable.h"
#include "beat_time_event.h"

class ScriptAction {
public:
    virtual ~ScriptAction() {}
    virtual ScriptActionType Type() const = 0;

    // Expected to be run during ConnectComponents() in components.
    // TODO: Is this the right place to run it?
    void Init(GameManager& g) { 
        InitImpl(g);
        _init = true;
    }

    void Execute(GameManager& g) {
        assert(_init);
        ExecuteImpl(g);
    }
    
    virtual void DrawImGui() {}

    virtual void Save(serial::Ptree pt) const {}
    virtual void Load(serial::Ptree pt) {}

    static void SaveActions(serial::Ptree pt, std::vector<std::unique_ptr<ScriptAction>> const& actions);
    static void LoadActions(serial::Ptree pt, std::vector<std::unique_ptr<ScriptAction>>& actions);

protected:
    virtual void ExecuteImpl(GameManager& g) const = 0;
    virtual void InitImpl(GameManager& g) {};
    bool _init = false;
};

class ScriptActionDestroyAllPlanets : public ScriptAction {
public:
    virtual ScriptActionType Type() const override { return ScriptActionType::DestroyAllPlanets; }
    virtual void ExecuteImpl(GameManager& g) const override;
};

class ScriptActionActivateEntity : public ScriptAction {
public:
    virtual ScriptActionType Type() const override { return ScriptActionType::ActivateEntity; }
    virtual void InitImpl(GameManager& g) override;
    virtual void ExecuteImpl(GameManager& g) const override;

    virtual void DrawImGui() override;

    void Save(serial::Ptree pt) const override;
    void Load(serial::Ptree pt) override;

    // Serialize
    std::string _entityName;

    // Non-serialize
    EntityId _entityId;
};

class ScriptActionAudioEvent : public ScriptAction {
    virtual ScriptActionType Type() const override { return ScriptActionType::AudioEvent; }
    virtual void ExecuteImpl(GameManager& g) const override;

    virtual void DrawImGui() override;

    void Save(serial::Ptree pt) const override;
    void Load(serial::Ptree pt) override;

    // serialize
    double _denom = 0.25;
    BeatTimeEvent _event;
    std::string _recorderName;
};


class ScriptActionStartWaypointFollow : public ScriptAction {
    virtual ScriptActionType Type() const override { return ScriptActionType::StartWaypointFollow; }

    virtual void InitImpl(GameManager& g) override;
    virtual void ExecuteImpl(GameManager& g) const override;

    virtual void DrawImGui() override;

    void Save(serial::Ptree pt) const override;
    void Load(serial::Ptree pt) override;

    // serialize
    std::string _entityName;

    // non-serialize
    EntityId _entityId;
};

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType);
void DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions);