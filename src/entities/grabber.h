#pragma once

#include "new_entity.h"
#include "imgui_str.h"
#include "seq_action.h"
#include "mech_button.h"

struct GrabberEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::Grabber; }
    static ne::EntityType StaticType() { return ne::EntityType::Grabber; }
    
    struct Props {
        bool hasPlusRot = true;
        bool hasNegRot = true;
        double quantize = 0.0;
        float angleDeg = 0.f;
        float length = 1.f;
        float height = 0.f;
        std::vector<std::unique_ptr<SeqAction>> plusRotActions;
        std::vector<std::unique_ptr<SeqAction>> negRotActions;
    };
    Props _p;

    struct State {
        double actionBeatTime = -1.0;
        bool posAction = false;
        ne::EntityId resource;

        float angleRad = 0.f;
        double moveStartTime = -1.0;
        double moveEndTime = -1.0;
        float moveStartAngleRad = 0.0f;
        float moveEndAngleRad = 0.0f;
        Vec3 handToResOffset;
    };
    State _s;

    MechButton _plusRotKey;
    MechButton _negRotKey;

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void OnEditPick(GameManager& g) override; 

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override; 
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    GrabberEntity() = default;
    GrabberEntity(GrabberEntity const&) = delete;
    GrabberEntity(GrabberEntity&&) = default;
    GrabberEntity& operator=(GrabberEntity&&) = default;

};

