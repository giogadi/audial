#pragma once

#include "new_entity.h"

struct ParamAutomatorEntity : public ne::Entity {
    // serialized
    float _startVelocity = 0.f;
    float _endVelocity = 1.f;
    double _desiredAutomateTime = 1.0;
    std::string _seqEntityName;

    // non-serialized
    ne::EntityId _seqId;
    double _automateStartTime = 0.0;

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void SaveDerived(serial::Ptree pt) const override; */
    /* virtual void LoadDerived(serial::Ptree pt) override; */
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) override; */
};
