#pragma once

#include "seq_action.h"
#include "editor_id.h"

struct AddMotionSeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::AddMotion; }

    struct Props {
        EditorId editorId;
        Vec3 v;
        float time = 0.f;
    };
    Props _p;
    struct State {
        ne::EntityId entityId;
    };
    State _s;
    virtual void InitDerived(GameManager& g) override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual bool ImGui() override;

    virtual void ExecuteDerived(GameManager& g) override;
};