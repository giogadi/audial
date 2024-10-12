#pragma once

#include "new_entity.h"
#include "serial.h"
#include "seq_action.h"

struct TypingSliderEntity : public ne::BaseEntity {
    virtual ne::EntityType Type() override { return ne::EntityType::TypingSlider; }
    static ne::EntityType StaticType() { return ne::EntityType::TypingSlider; }
    
    struct Props {
        std::string _text;
        std::vector<std::unique_ptr<SeqAction>> _actions;
    };
    Props _p;

    struct State {
        int _index = 0;
    };
    State _s;

    // Init() is intended to be called after Load(). Load should not touch anything
    // outside this class. Everything else should happen here.
    virtual void OnEditPick(GameManager& g) override; 

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override; 
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual ImGuiResult MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) override;

    virtual void UpdateDerived(GameManager& g, float dt) override; 

    virtual void Draw(GameManager& g, float dt) override;

    TypingSliderEntity() = default;
    TypingSliderEntity(TypingSliderEntity const&) = delete;
    TypingSliderEntity(TypingSliderEntity&&) = default;
    TypingSliderEntity& operator=(TypingSliderEntity&&) = default;

};

