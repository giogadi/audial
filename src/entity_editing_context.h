#pragma once

#include <optional>
#include <string>

#include "component.h"

class GameManager;
class InputManager;

class EntityEditingContext {
public:
    EntityId _selectedEntityId = EntityId::InvalidId();
    int _selectedComponentIx = 0;
    std::string _saveFilename;

    void Update(float dt, bool editMode, GameManager const& g, int windowWidth, int windowHeight);    
    void DrawEntitiesWindow(EntityManager& entities, GameManager& g);
    static void DrawEntityImGui(EntityId id, GameManager& g, int* selectedComponentIx, bool connectComponents);

private:
    void UpdateSelectedPositionFromInput(float dt, InputManager const& input, EntityManager& entities);
};