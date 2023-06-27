#pragma once

#include "seq_action.h"
#include "synth_patch.h"

struct ChangePatchSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::ChangePatch; }

    struct Props {
        int _channelIx = 0;
        double _blendBeatTime = 0.0;
        bool _usePatchBank = false;
        std::string _patchBankName;
        synth::Patch _patch;  // used if usePatchBank is false
    };
    Props _p;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;

    virtual void ExecuteDerived(GameManager& g) override;
};
