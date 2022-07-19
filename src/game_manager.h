#pragma once

namespace renderer {
    class Scene;
}
class InputManager;
namespace audio {
    class Context;
}
class EntityManager;
class CollisionManager;
class ModelManager;
class BeatClock;

struct GameManager {
    renderer::Scene* _scene = nullptr;
    InputManager* _inputManager = nullptr;
    audio::Context* _audioContext = nullptr;
    EntityManager* _entityManager = nullptr;
    CollisionManager* _collisionManager = nullptr;
    ModelManager* _modelManager = nullptr;
    BeatClock* _beatClock = nullptr;
};