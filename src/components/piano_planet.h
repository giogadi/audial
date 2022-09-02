#pragma once

#include <vector>

#include "component.h"

class PianoPlanetComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::PianoPlanet; };
    virtual void Update(float dt) override;
    // virtual void Destroy() {};
    // virtual void EditDestroy() { Destroy(); };
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    // // If true, request that we try reconnecting the entity's components.
    virtual bool DrawImGui() override;
    // virtual void OnEditPick() {}
    // virtual void EditModeUpdate(float dt) {}

    virtual void Save(serial::Ptree pt) const override;
    virtual void Load(serial::Ptree pt) override;

    // Serialized
    std::vector<int> _midiNotes;

    // Non-serialized
    float _currentValue = 0.5f;  // [0,1]
    int _currentNote = -1;
    GameManager* _g = nullptr;
    EntityId _entityId;
};