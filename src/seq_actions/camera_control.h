#pragma once

#include "seq_action.h"

struct CameraControlSeqAction : public SeqAction {    
    Vec3 _desiredTargetToCameraOffset;
    
    virtual void Execute(GameManager& g) override;
    virtual void Load(GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;
};
