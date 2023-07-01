#pragma once

#include "seq_action.h"

struct SetEnemyHittableSeqAction : public SeqAction {
    int _entityTag = -1;
    bool _hittable = false;
    
    virtual SeqActionType Type() const override { return SeqActionType::SetEnemyHittable; }
    
    
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;
    virtual void ExecuteDerived(GameManager& g) override;
};
