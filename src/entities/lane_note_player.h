#pragma once

#include <map>

#include "new_entity.h"
#include "entities/lane_note_enemy.h"

struct LaneNotePlayerEntity : public ne::Entity {
    // serialized
    std::string _spawnsFilename;
    
    std::map<std::string, LaneNoteEnemyEntity::LaneNotesTable> _laneNotesTables;
    
    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    // virtual void Destroy(GameManager& g) {}
    // virtual void OnEditPick(GameManager& g) {}
    // virtual void DebugPrint();

protected:
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override { return ImGuiResult::Done; }
};
