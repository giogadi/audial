#pragma once


#include "component.h"
#include "enums/ScriptActionType.h"
#include "components/orbitable.h"
#include "beat_time_event.h"

class ScriptAction {
public:
    virtual ~ScriptAction() {}
    virtual ScriptActionType Type() const = 0;
    virtual void Execute(GameManager& g) const = 0;
    virtual void DrawImGui() {}
    virtual void Save(ptree& pt) const {}
    virtual void Load(ptree const& pt) {}

    virtual void Save(serial::Ptree pt) const {}

    static void SaveActions(ptree& pt, std::vector<std::unique_ptr<ScriptAction>> const& actions);
    static void SaveActions(serial::Ptree pt, std::vector<std::unique_ptr<ScriptAction>> const& actions);
    static void LoadActions(ptree const& pt, std::vector<std::unique_ptr<ScriptAction>>& actions);
};

class ScriptActionDestroyAllPlanets : public ScriptAction {
public:
    virtual ScriptActionType Type() const override { return ScriptActionType::DestroyAllPlanets; }
    virtual void Execute(GameManager& g) const override;
};

class ScriptActionActivateEntity : public ScriptAction {
public:
    virtual ScriptActionType Type() const override { return ScriptActionType::ActivateEntity; }
    virtual void Execute(GameManager& g) const override;

    virtual void DrawImGui() override;

    void Save(ptree& pt) const override;
    void Load(ptree const& pt) override;

    void Save(serial::Ptree pt) const override;

    std::string _entityName;
};

// TODO: need to think about how to specify denom + beattimeoffset etc. not
// doing that currently. Look at EventsOnHit::PlayEventsOnNextDenom.
class ScriptActionAudioEvent : public ScriptAction {
    virtual ScriptActionType Type() const override { return ScriptActionType::AudioEvent; }
    virtual void Execute(GameManager& g) const override;

    virtual void DrawImGui() override;

    void Save(ptree& pt) const override;
    void Load(ptree const& pt) override;

    void Save(serial::Ptree pt) const override;

    // serialize
    double _denom = 0.25;
    BeatTimeEvent _event;
};


class ScriptActionStartWaypointFollow : public ScriptAction {
    virtual ScriptActionType Type() const override { return ScriptActionType::StartWaypointFollow; }
    virtual void Execute(GameManager& g) const override;

    virtual void DrawImGui() override;

    void Save(ptree& pt) const override;
    void Load(ptree const& pt) override;

    void Save(serial::Ptree pt) const override;

    // serialize
    std::string _entityName;
};

std::unique_ptr<ScriptAction> MakeScriptActionOfType(ScriptActionType actionType);
void DrawScriptActionListImGui(std::vector<std::unique_ptr<ScriptAction>>& actions);