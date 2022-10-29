#pragma once

#include "new_entity.h"
#include "beat_time_event.h"

struct EnemyEntity : public ne::Entity {
    // serialized
    std::vector<BeatTimeEvent> _events;
    double _eventStartDenom = 0.25;

    // virtual void Init(GameManager& g) override;
    // virtual void Update(GameManager& g, float dt) override;
    // virtual void Destroy(GameManager& g) override;    
    virtual void OnEditPick(GameManager& g) override;

    void SendEvents(GameManager& g);

protected:
    // Used by derived classes to save/load child-specific data.
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};