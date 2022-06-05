#pragma once

#include <optional>
#include <string>

class Entity;
class GameManager;
class InputManager;
class EntityManager;

class EntityEditingContext {
public:
    int _selectedEntityIx = -1;
    int _selectedComponentIx = 0;
    std::string _saveFilename;

    void Update(float dt, bool editMode, GameManager const& g, int windowWidth, int windowHeight);    
    void DrawEntitiesWindow(EntityManager& entities, GameManager& g);
    static void DrawEntityImGui(Entity& entity, GameManager& g, int* selectedComponentIx, bool connectComponents);

private:
    void UpdateSelectedPositionFromInput(float dt, InputManager const& input, EntityManager& entities);
};