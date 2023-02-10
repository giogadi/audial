#pragma once

#include "seq_action.h"

struct CameraControlSeqAction : public SeqAction {
    // serialized
    std::string _targetEntityName;
    Vec3 _desiredTargetToCameraOffset;
    bool _setTarget = false;
    bool _setOffset = false;
    std::optional<float> _minX;
    std::optional<float> _minZ;
    std::optional<float> _maxX;
    std::optional<float> _maxZ;

    //non-serialized
    ne::EntityId _targetEntityId;    
    
    virtual void Execute(GameManager& g) override;
    virtual void Load(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void Init(GameManager& g) override;
};
