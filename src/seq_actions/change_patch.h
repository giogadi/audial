#pragma once

#include "seq_action.h"
#include "synth_patch.h"

struct ChangePatchSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::ChangePatch; }

    int _channelIx = 0;
    double _blendBeatTime = 0.0;
    synth::Patch _patch;    

    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;

    virtual void ExecuteDerived(GameManager& g) override;
};
