#pragma once

#include <optional>

class Entity;
class TransformComponent;
class GameManager;
class InputManager;
class EntityManager;

class EntityEditingContext {
public:
    std::optional<int> _selectedEntityIx;
    Entity* _selectedEntity = nullptr;
    TransformComponent* _selectedTransform = nullptr;

    void Update(float dt, bool editMode, GameManager const& g, int windowWidth, int windowHeight);    
    void DrawEntitiesWindow(EntityManager& entities, GameManager& g);

private:
    void UpdateSelectedPositionFromInput(float dt, InputManager const& input);
};