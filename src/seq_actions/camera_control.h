#pragma once

#include "seq_action.h"

struct CameraControlSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::CameraControl; }
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
    
    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void InitDerived(GameManager& g) override;
};
