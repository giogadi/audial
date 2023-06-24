#pragma once

#include "seq_action.h"
#include "properties/CameraControlSeqAction.h"

struct CameraControlSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::CameraControl; }
    CameraControlSeqActionProps _props;

    //non-serialized
    ne::EntityId _targetEntityId;    
    
    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;
    virtual void LoadDerived(serial::Ptree pt) override { _props.Load(pt); }
    virtual void SaveDerived(serial::Ptree pt) const override { _props.Save(pt); }
    virtual bool ImGui() override;
    virtual void InitDerived(GameManager& g) override;
};
