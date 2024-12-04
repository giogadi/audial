#pragma once

struct Editor;
namespace renderer {
    class Scene;
}
class InputManager;
namespace audio {
    struct Context;
}
namespace synth {
    struct PatchBank;
}
namespace ne {
    struct EntityManager;
}
class CollisionManager;
class BeatClock;
class SoundBank;
struct ParticleMgr;
struct MotionManager;
struct TypingEnemyMgr;
struct OmniSequencer;

#include "viewport.h"

struct GameManager {
    Editor* _editor = nullptr;
    renderer::Scene* _scene = nullptr;
    InputManager* _inputManager = nullptr;
    audio::Context* _audioContext = nullptr;
    ne::EntityManager* _neEntityManager = nullptr;
    BeatClock* _beatClock = nullptr;
    SoundBank* _soundBank = nullptr;
    synth::PatchBank* _synthPatchBank = nullptr;
    ParticleMgr* _particleMgr = nullptr;
    MotionManager* _motionManager = nullptr;
    TypingEnemyMgr *_typingEnemyMgr = nullptr;
    OmniSequencer *_omniSequencer = nullptr;

    bool _editMode = false;

    int _windowWidth = -1;
    int _windowHeight = -1;
    int _fbWidth = -1;
    int _fbHeight = -1;
    float _aspectRatio = 0.f;
    ViewportInfo _viewportInfo;
};
