#pragma once

#include <optional>

class Entity;
class GameManager;
class InputManager;
class EntityManager;

class EntityEditingContext {
public:
    std::optional<int> _selectedEntityIx;
    std::weak_ptr<Entity> _selectedEntity;
    int _selectedComponentIx = 0;

    void Update(float dt, bool editMode, GameManager const& g, int windowWidth, int windowHeight);    
    void DrawEntitiesWindow(EntityManager& entities, GameManager& g);

private:
    void UpdateSelectedPositionFromInput(float dt, InputManager const& input);
};