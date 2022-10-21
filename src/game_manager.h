#pragma once

namespace renderer {
    class Scene;
}
class InputManager;
namespace audio {
    struct Context;
}
class EntityManager;
class CollisionManager;
class BeatClock;
class SoundBank;

struct GameManager {
    renderer::Scene* _scene = nullptr;
    InputManager* _inputManager = nullptr;
    audio::Context* _audioContext = nullptr;
    EntityManager* _entityManager = nullptr;
    CollisionManager* _collisionManager = nullptr;
    BeatClock* _beatClock = nullptr;
    SoundBank* _soundBank = nullptr;

    bool _editMode = false;

    int _windowWidth = -1;
    int _windowHeight = -1;
};