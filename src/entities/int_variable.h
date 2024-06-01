#pragma once

#include "new_entity.h"

struct SeqAction;

namespace synth {
    struct Patch;
}

struct IntVariableEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::IntVariable; }
    static ne::EntityType StaticType() { return ne::EntityType::IntVariable; }
    
    // serialized
    int _initialValue = -1;

    bool _drawCounter = false;
    bool _drawHorizontal = false;
    float _cellWidth = 0.25f;
    float _cellSpacing = 0.25f;

    int _synthPatchChannel = -1;
    std::string _startPatchName;
    std::string _endPatchName;
    double _blendTime = 0.0;

    // non-serialized
    int _currentValue = -1;
    std::vector<std::unique_ptr<SeqAction>> _actions;
    GameManager* _g = nullptr;
    double _beatTimeOfLastAdd = -1.0;
    synth::Patch* _startPatch = nullptr;
    synth::Patch* _endPatch = nullptr;


    void AddToVariable(int amount);
    void Reset();

    void DrawCounter(GameManager& g);

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    virtual void Draw(GameManager& g, float dt) override;

    IntVariableEntity() = default;
    IntVariableEntity(IntVariableEntity const&) = delete;
    IntVariableEntity(IntVariableEntity&&) = default;
    IntVariableEntity& operator=(IntVariableEntity&&) = default;
};
