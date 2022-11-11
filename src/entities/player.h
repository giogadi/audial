#pragma once

#include <map>

#include "new_entity.h"
#include "entities/enemy.h"

struct PlayerEntity : public ne::Entity {
    std::map<std::string, EnemyEntity::LaneNotesTable> _laneNotesTables;
    
    virtual void Init(GameManager& g);
    virtual void Update(GameManager& g, float dt);
    // virtual void Destroy(GameManager& g) {}
    // virtual void OnEditPick(GameManager& g) {}
    // virtual void DebugPrint();

protected:
    virtual void SaveDerived(serial::Ptree pt) const {};
    virtual void LoadDerived(serial::Ptree pt) {};
    virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; }
};
