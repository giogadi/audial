#pragma once

class SceneManager;
class InputManager;
namespace audio {
    class Context;
}
class EntityManager;
class CollisionManager;
class ModelManager;
class BeatClock;
class TransformManager;

struct GameManager {
    SceneManager* _sceneManager = nullptr;
    InputManager* _inputManager = nullptr;
    audio::Context* _audioContext = nullptr;
    EntityManager* _entityManager = nullptr;
    CollisionManager* _collisionManager = nullptr;
    ModelManager* _modelManager = nullptr;
    BeatClock* _beatClock = nullptr;
    TransformManager* _transformManager = nullptr;
};