#pragma once

#include "seq_action.h"
#include "properties/VfxPulseSeqAction.h"

struct VfxPulseSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::VfxPulse; }

    VfxPulseSeqActionProps _p;

    struct State {
        ne::EntityId _entityId;
    };
    State _s;
    
    virtual void ExecuteDerived(GameManager& g) override;   
    virtual void LoadDerived(serial::Ptree pt) override { _p.Load(pt); }
    virtual void SaveDerived(serial::Ptree pt) const override {
        _p.Save(pt);        
    }
    virtual bool ImGui() override { return _p.ImGui(); };
    virtual void InitDerived(GameManager& g) override;
};
