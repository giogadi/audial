#pragma once

#include <optional>
#include <string>

#include "component.h"
#include "version_id.h"
#include "game_manager.h"

class GameManager;
class BoundMeshPNU;

class EntityEditingContext {
public:
    bool Init(GameManager& g);
    EntityId _selectedEntityId = EntityId::InvalidId();
    int _selectedComponentIx = 0;
    std::string _saveFilename;
    std::string _prefabFilename;

    void Update(float dt, bool editMode, GameManager& g, int windowWidth, int windowHeight);
    void DrawEntitiesWindow(EntityManager& entities, GameManager& g);
    static void DrawEntityImGui(EntityId id, GameManager& g, int* selectedComponentIx, bool connectComponents);

private:
    void UpdateSelectedPositionFromInput(float dt, GameManager& gameManager);

    BoundMeshPNU const* _axesModel = nullptr;
};