#pragma once

#include "game_manager.h"
#include "new_entity.h"

struct Editor {
    void Init(GameManager* g);
    void Update();
    void DrawWindow();

    GameManager* _g = nullptr;
    bool _visible = false;
    ne::EntityId _selectedEntityId;
    int _selectedEntityTypeIx = 0;
    std::string _saveFilename;
    std::vector<ne::EntityId> _entityIds;
};