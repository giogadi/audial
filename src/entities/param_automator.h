#pragma once

#include "new_entity.h"
#include "enums/audio_SynthParamType.h"
#include "properties/SpawnAutomatorSeqAction.h"

struct ParamAutomatorEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::ParamAutomator; }
    static ne::EntityType StaticType() { return ne::EntityType::ParamAutomator; }
    
    SpawnAutomatorSeqActionProps _props;

    // non-serialized
    ne::EntityId _seqId;
    double _automateStartTime = 0.0;
    float _startValueAdjusted = 0.f;
    float _endValueAdjusted = 1.f;

    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
};
