#pragma once

#include "new_entity.h"

struct SeqAction;

struct IntVariableEntity : public ne::Entity {
    // serialized
    int _initialValue = -1;
    bool _drawCounter = false;
    bool _drawHorizontal = false;

    // non-serialized
    int _currentValue = -1;
    std::vector<std::unique_ptr<SeqAction>> _actions;
    GameManager* _g = nullptr;
    double _beatTimeOfLastAdd = -1.0;

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
