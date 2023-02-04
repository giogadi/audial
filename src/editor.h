#pragma once

#include <vector>
#include <set>

#include "game_manager.h"
#include "new_entity.h"

class BoundMeshPNU;

struct Editor {
    void Init(GameManager* g);
    void Update(float dt);
    void DrawWindow();
    void DrawMultiEnemyWindow();
    void HandleEntitySelectAndMove();

    GameManager* _g = nullptr;
    bool _visible = false;
    std::set<ne::EntityId> _selectedEntityIds;
    int _selectedEntityTypeIx = 0;
    BoundMeshPNU const* _axesMesh = nullptr;
    std::string _saveFilename;
    std::vector<ne::EntityId> _entityIds;
};
