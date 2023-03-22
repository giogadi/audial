#pragma once

#include "new_entity.h"

struct SeqAction;

struct IntVariableEntity : public ne::Entity {
    // serialized
    int _initialValue = -1;
    std::vector<std::string> _actionStrings;  // default behavior: action when current value hits zero

    // non-serialized
    int _currentValue = -1;
    std::vector<std::unique_ptr<SeqAction>> _actions;
    GameManager* _g = nullptr;

    void AddToVariable(int amount);

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    IntVariableEntity() = default;
    IntVariableEntity(IntVariableEntity const&) = delete;
    IntVariableEntity(IntVariableEntity&&) = default;
    IntVariableEntity& operator=(IntVariableEntity&&) = default;
};
