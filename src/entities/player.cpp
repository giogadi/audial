#include "entities/player.h"

#include <array>

#include "game_manager.h"
#include "input_manager.h"
#include "entities/enemy.h"
#include "renderer.h"

namespace {
int constexpr kNumLanes = 8;
float constexpr kLaneWidth = 2.2f;
float constexpr kLaneBoundaryWidth = 0.25f;

void DrawLanes(GameManager& g) {
    float startX = -0.5f * kNumLanes * kLaneWidth;
    Vec4 color(0.f, 0.f, 0.f, 1.f);
    for (int i = 0; i < kNumLanes+1; ++i) {
        Mat4 transform;
        transform.SetTranslation(Vec3(startX, 0.f, -2.f));
        transform.Scale(kLaneBoundaryWidth, kLaneBoundaryWidth, 20.f);
        g._scene->DrawCube(transform, color);
        startX += kLaneWidth;
    }
}

int GetMidiNote(char noteName, int octaveNum) {
    noteName = tolower(noteName);
    assert(noteName >= 'a' && noteName <= 'g');
    assert(octaveNum >= 0);
    // c0 is 12.
    int midiNum = 0;
    switch (noteName) {        
        case 'c': midiNum = 12; break;
        case 'd': midiNum = 14; break;
        case 'e': midiNum = 16; break;
        case 'f': midiNum = 17; break;
        case 'g': midiNum = 19; break;
        case 'a': midiNum = 21; break;
        case 'b': midiNum = 23; break;
    }
    midiNum += 12 * octaveNum;
    return midiNum;
}

int GetMidiNote(char const* noteName) {
    assert(noteName[0] != '\0' && noteName[1] != '\0');
    char octaveChar = noteName[1];
    int octaveNum = octaveChar - '0';
    int modifier = 0;
    if (noteName[2] == '+') {
        modifier = 1;
    } else if (noteName[2] == '-') {
        modifier = -1;
    }
    int note = GetMidiNote(noteName[0], octaveNum) + modifier;
    return note;
}

ne::Entity* MakeNoteEnemy(GameManager& g, char const* noteName, int laneIx, double activeTime, double despawnTime) {
    EnemyEntity* enemy = (EnemyEntity*) g._neEntityManager->AddEntity(ne::EntityType::Enemy);
    enemy->_modelName = "cube";
    enemy->_modelColor.Set(0.3f, 0.3f, 0.3f, 1.f);

    int midiNote = GetMidiNote(noteName);

    BeatTimeEvent b_e;
    b_e._e.type = audio::EventType::NoteOn;
    b_e._e.channel = 0;
    b_e._e.midiNote = midiNote;
    b_e._beatTime = 0.0;
    enemy->_events.push_back(b_e);
    b_e._e.type = audio::EventType::NoteOff;
    b_e._beatTime = 0.5;
    enemy->_events.push_back(b_e);

    switch (laneIx) {
        case 0: enemy->_shootButton = InputManager::Key::A; break;
        case 1: enemy->_shootButton = InputManager::Key::S; break;
        case 2: enemy->_shootButton = InputManager::Key::D; break;
        case 3: enemy->_shootButton = InputManager::Key::F; break;
        case 4: enemy->_shootButton = InputManager::Key::H; break;
        case 5: enemy->_shootButton = InputManager::Key::J; break;
        case 6: enemy->_shootButton = InputManager::Key::K; break;
        case 7: enemy->_shootButton = InputManager::Key::L; break;
        default: assert(false); break;
    }

    float leftMostLaneX = -0.5f * kNumLanes * kLaneWidth + 0.5f*kLaneWidth;
    Vec3 p(leftMostLaneX + kLaneWidth * laneIx, 0.f, -10.f);
    enemy->_transform.SetTranslation(p);

    enemy->_activeBeatTime = activeTime;
    enemy->_inactiveBeatTime = despawnTime;

    enemy->Init(g);

    return enemy;
}
}

struct Spawn {
    double _spawnBeatTime;
    double _despawnBeatTime;
    char const* _noteName;
    int _laneIx;
};

namespace {
std::vector<Spawn> const gSpawns = {
    {0., 4., "C1", 0},
    {0., 4., "G1", 4},
    {8., 12., "C1", 0},
    {8., 12., "G1", 4},
    {8., 12., "B1-", 6},
    {8., 12., "C2", 7},
    {16., 20., "C1", 0},
    {16., 20., "G1", 4},
    {24., 28., "C1", 0},
    {24., 28., "G1", 4},
    {24., 28., "B1-", 6},
    {24., 28., "C2", 7},
};
int gSpawnsIx = 0;
}

void PlayerEntity::Init(GameManager& g) {
    for (Spawn const& spawn : gSpawns) {
        MakeNoteEnemy(g, spawn._noteName, spawn._laneIx, spawn._spawnBeatTime, spawn._despawnBeatTime);
    }
}

void PlayerEntity::ScriptUpdate(GameManager& g) {
    // double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    // while (gSpawnsIx < gSpawns.size() && beatTime >= gSpawns[gSpawnsIx]._spawnBeatTime) {
    //     MakeNoteEnemy(g, gSpawns[gSpawnsIx]._noteName, gSpawns[gSpawnsIx]._laneIx,);
    //     ++gSpawnsIx;
    // }
}

void PlayerEntity::Update(GameManager& g, float dt) {
    DrawLanes(g);

    // assume only a few keys can be pressed at once.
    int constexpr kNumSimulKeys = 4;
    std::array<InputManager::Key, kNumSimulKeys> keysShot;
    int numKeysShot = 0;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::Enemy);
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        EnemyEntity* enemy = (EnemyEntity*) enemyIter.GetEntity();
        if (!enemy->IsActive(g)) {
            continue;
        }
        InputManager::Key enemyKey = enemy->_shootButton;
        if (!g._inputManager->IsKeyPressedThisFrame(enemyKey)) {
            continue;
        }
        auto keysEnd = keysShot.begin() + numKeysShot;
        auto findRes = std::find(keysShot.begin(), keysEnd, enemyKey);
        if (findRes != keysEnd) {
            // Already shot an enemy with this key, skip.
            continue;
        }
        keysShot[numKeysShot] = enemyKey;
        ++numKeysShot;
        enemy->OnShot(g);
        if (numKeysShot >= kNumSimulKeys) {
            break;
        }
    }

    if (!g._editMode) {
        ScriptUpdate(g);
    }
}