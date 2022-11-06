#include "entities/player.h"

#include <array>
#include <fstream>
#include <sstream>

#include "game_manager.h"
#include "input_manager.h"
#include "entities/enemy.h"
#include "renderer.h"

namespace {
int constexpr kNumLanes = 8;
float constexpr kLaneWidth = 2.2f;
float constexpr kLaneBoundaryWidth = 0.25f;

// Draw them in 2 sets of 4.
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
    assert(octaveNum < 8);
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

ne::Entity* MakeNoteEnemy(GameManager& g, char const* noteName, int laneIx, double activeTime, double despawnTime, float z) {
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
    Vec3 p(leftMostLaneX + kLaneWidth * laneIx, 0.f, z);
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
    std::string _noteName;
    int _laneIx;
    float _z;
};

namespace {

// bool StartsWith(std::string const& str, std::string const& prefix) {
//     if (str.length() < prefix.length()) {
//         return false;
//     }
//     for (int i = 0; i < prefix.length(); ++i) {
//         if (str[i] != prefix[i]) {
//             return false;
//         }
//     }
//     return true;
// }

void LoadSpawnsFromFile(char const* fileName, std::vector<Spawn>* spawns) {
    std::ifstream inFile(fileName);
    double beatTimeOffset = 0.0;
    std::string line;
    std::string token;
    while (!inFile.eof()) {
        std::getline(inFile, line);
        // If it's only whitespace, just skip.
        bool onlySpaces = true;
        for (char c : line) {
            if (!isspace(c)) {
                onlySpaces = false;
                break;
            }
        }
        if (onlySpaces) {
            continue;
        }

        std::stringstream lineStream(line);
        lineStream >> token;
        if (token == "offset") {
            lineStream >> beatTimeOffset;
            continue;
        }     

        spawns->emplace_back();
        Spawn& spawn = spawns->back();        
        lineStream >> spawn._spawnBeatTime;
        lineStream >> spawn._despawnBeatTime;
        lineStream >> spawn._noteName;
        lineStream >> spawn._laneIx;
        lineStream >> spawn._z;
        spawn._spawnBeatTime += beatTimeOffset;
        spawn._despawnBeatTime += beatTimeOffset;
    }
}
}

void PlayerEntity::Init(GameManager& g) {
    std::vector<Spawn> spawns;
    LoadSpawnsFromFile("data/spawns_tech.txt", &spawns);
    for (Spawn const& spawn : spawns) {
        MakeNoteEnemy(g, spawn._noteName.c_str(), spawn._laneIx, spawn._spawnBeatTime, spawn._despawnBeatTime, spawn._z);
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
    struct KeyInfo {
        InputManager::Key _key;
        EnemyEntity* _nearest = nullptr;
        float _dist = 0.f;
    };
    std::array<KeyInfo, kNumSimulKeys> keysShot;
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
        bool alreadyFound = false;
        float enemyDist = -enemy->_transform._m23;
        for (int i = 0; i < numKeysShot; ++i) {
            if (keysShot[i]._key == enemyKey) {
                alreadyFound = true;
                if (enemyDist < keysShot[i]._dist) {
                    keysShot[i]._nearest = enemy;
                    keysShot[i]._dist = enemyDist;
                }
            }
        }
        if (!alreadyFound) {
            keysShot[numKeysShot]._key = enemyKey;
            keysShot[numKeysShot]._nearest = enemy;
            keysShot[numKeysShot]._dist = enemyDist;
            ++numKeysShot;
        }
        if (numKeysShot >= kNumSimulKeys) {
            break;
        }
    }

    for (int i = 0; i < numKeysShot; ++i) {
        keysShot[i]._nearest->OnShot(g);
    }

    if (!g._editMode) {
        ScriptUpdate(g);
    }
}