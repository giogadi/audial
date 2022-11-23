#pragma once

#include <map>

#include "new_entity.h"
#include "entities/lane_note_enemy.h"

struct LaneNotePlayerEntity : public ne::Entity {
    // serialized
    std::string _spawnsFilename;
    
    std::map<std::string, LaneNoteEnemyEntity::LaneNotesTable> _laneNotesTables;
    
    virtual void Init(GameManager& g);
    virtual void Update(GameManager& g, float dt);
    // virtual void Destroy(GameManager& g) {}
    // virtual void OnEditPick(GameManager& g) {}
    // virtual void DebugPrint();

protected:
    virtual void SaveDerived(serial::Ptree pt) const;
    virtual void LoadDerived(serial::Ptree pt);
    virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; }
};
