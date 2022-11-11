#include "player.h"

#include <array>
#include <fstream>
#include <sstream>
#include <map>

#include "game_manager.h"
#include "input_manager.h"
#include "entities/enemy.h"
#include "renderer.h"
#include "constants.h"

namespace {

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

struct Spawn {
    double _spawnBeatTime = 0.0;
    double _despawnBeatTime = -1.0;
    // NOTE: if laneNoteBehavior != None, this noteName is actually the ROOT
    // NOTE for the left-most lane.
    std::string _noteName = "C2";
    int _channel = 0;
    int _laneIx = 0;
    float _z = 0.f;
    EnemyEntity::Behavior _behavior = EnemyEntity::Behavior::None;
    EnemyEntity::LaneNoteBehavior _laneNoteBehavior =
    EnemyEntity::LaneNoteBehavior::None;
    EnemyEntity::OnHitBehavior _onHitBehavior = EnemyEntity::OnHitBehavior::Default;
    int _hp = 1;
    float _downSpeed = 2.f;
    double _noteLength = 0.25;
    std::vector<int> _laneNoteOffsets;
};

ne::Entity* MakeNoteEnemy(GameManager& g, Spawn const& spawnInfo) {
    EnemyEntity* enemy = (EnemyEntity*) g._neEntityManager->AddEntity(ne::EntityType::Enemy);
    enemy->_modelName = "cube";
    enemy->_modelColor.Set(0.3f, 0.3f, 0.3f, 1.f);
    enemy->_eventStartDenom = 0.25;
    enemy->_behavior = spawnInfo._behavior;
    enemy->_hp = spawnInfo._hp;
    enemy->_downSpeed = spawnInfo._downSpeed;
    enemy->_laneNoteBehavior = spawnInfo._laneNoteBehavior;
    enemy->_onHitBehavior = spawnInfo._onHitBehavior;

    int midiNote = GetMidiNote(spawnInfo._noteName.c_str());
    enemy->_laneRootNote = midiNote;
    enemy->_laneNoteOffsets = spawnInfo._laneNoteOffsets;

    // MAYBE TODO: for non-none lane-note-behaviors, maybe we should initialize this to the correct lane-corrected note?
    if (enemy->_laneNoteBehavior == EnemyEntity::LaneNoteBehavior::None) {
        BeatTimeEvent b_e;
        b_e._e.type = audio::EventType::NoteOn;
        b_e._e.channel = spawnInfo._channel;
        b_e._e.midiNote = midiNote;
        b_e._beatTime = 0.0;
        enemy->_events.push_back(b_e);
        b_e._e.type = audio::EventType::NoteOff;
        b_e._beatTime = spawnInfo._noteLength;
        enemy->_events.push_back(b_e);
    } else if (enemy->_laneNoteBehavior == EnemyEntity::LaneNoteBehavior::Minor) {
        for (int const laneNoteOffset : enemy->_laneNoteOffsets) {
            (void) laneNoteOffset;
            BeatTimeEvent b_e;
            b_e._e.type = audio::EventType::NoteOn;
            b_e._e.channel = spawnInfo._channel;
            b_e._e.midiNote = midiNote;
            b_e._beatTime = 0.0;
            enemy->_events.push_back(b_e);
            b_e._e.type = audio::EventType::NoteOff;
            b_e._beatTime = spawnInfo._noteLength;
            enemy->_events.push_back(b_e);
        }
    }

    // HOWDY HACK!!!!!
    if (spawnInfo._onHitBehavior == EnemyEntity::OnHitBehavior::MultiPhase) {
        BeatTimeEvent b_e;
        b_e._e.type = audio::EventType::NoteOn;
        b_e._e.channel = 1;
        b_e._e.midiNote = midiNote;
        b_e._beatTime = 0.0;
        enemy->_events.push_back(b_e);
        b_e._e.type = audio::EventType::NoteOff;
        b_e._beatTime = 2.f;
        enemy->_events.push_back(b_e);
    }

    switch (spawnInfo._laneIx) {
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
    Vec3 p(leftMostLaneX + kLaneWidth * spawnInfo._laneIx, 0.f, spawnInfo._z);
    enemy->_transform.SetTranslation(p);

    enemy->_activeBeatTime = spawnInfo._spawnBeatTime;
    enemy->_inactiveBeatTime = spawnInfo._despawnBeatTime;

    enemy->Init(g);

    return enemy;
}
}

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
    bool IsOnlyWhitespace(std::string const& str) {
        for (char c : str) {
            if (!isspace(c)) {
                return false;
            }
        }
        return true;
    }

int constexpr kMaxNumNotesPerLane = 4;
typedef std::array<std::array<int, kMaxNumNotesPerLane>, kNumLanes> LaneNotesTable;

void LoadLaneNoteTablesFromFile(
    char const* fileName, std::map<std::string, LaneNotesTable>& laneNotesTables) {
    std::ifstream inFile(fileName);
    std::string line;
    std::string tableName;
    int lineNumber = 0;
    while (!inFile.eof()) {
        std::getline(inFile, line);
        ++lineNumber;
        std::string tableName;
        // skip commented-out lines
        if (line[0] == '#') {
            continue;
        }
        std::stringstream lineSs(line);
        lineSs >> tableName;
        // If it's empty, skip.
        if (tableName.empty()) {
            continue;
        }
        // If it's only whitespace, just skip.
        if (IsOnlyWhitespace(tableName)) {
            continue;
        }

        LaneNotesTable& table = laneNotesTables[tableName];
        for (int laneIx = 0; laneIx < kNumLanes; ++laneIx) {
            table[laneIx].fill(-1);
            std::getline(inFile, line);
            ++lineNumber;
            std::stringstream lineSs(line);
            for (int noteIx = 0; noteIx < kMaxNumNotesPerLane && !lineSs.eof(); ++noteIx) {
                std::string noteName;
                lineSs >> noteName;
                if (IsOnlyWhitespace(noteName)) {
                    continue;
                }
                int midiNote = GetMidiNote(noteName.c_str());
                table[laneIx][noteIx] = midiNote;
            }
            if (!lineSs.eof()) {
                printf("WARNING: Already read 4 notes but there was still data left in line %d!\n", lineNumber);
            }
        }
    }
}

void LoadSpawnsFromFile(char const* fileName, std::vector<Spawn>* spawns) {
    std::ifstream inFile(fileName);
    double beatTimeOffset = 0.0;
    std::string line;
    std::string token;
    while (!inFile.eof()) {
        std::getline(inFile, line);
        // If it's empty, skip.
        if (line.empty()) {
            continue;
        }
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

        // skip commented-out lines
        if (line[0] == '#') {
            continue;
        }

        std::stringstream lineStream(line);
        lineStream >> token;
        if (token == "offset") {
            lineStream >> beatTimeOffset;
            continue;
        }

        Spawn spawn;

        while (!lineStream.eof()) {
            std::string key, value;
            {
                lineStream >> token;
                std::size_t delimIx = token.find_first_of(':');
                if (delimIx == std::string::npos) {
                    printf("Token missing \":\" - \"%s\"\n", token.c_str());
                    continue;
                }

                key = token.substr(0, delimIx);
                value = token.substr(delimIx+1);
            }
            // If value is empty, just skip to the next thing.
            if (value.empty()) {
                continue;
            }

            if (key == "on") {
                spawn._spawnBeatTime = std::stod(value) + beatTimeOffset;
            } else if (key == "off") {
                spawn._despawnBeatTime = std::stod(value) + beatTimeOffset;
            } else if (key == "ch") {
                spawn._channel = std::stoi(value);
            } else if (key == "note") {
                spawn._noteName = value;
            } else if (key == "note_len") {
                spawn._noteLength = std::stof(value);
            } else if (key == "lane") {
                spawn._laneIx = std::stoi(value);
            } else if (key == "z") {
                spawn._z = std::stof(value);
            } else if (key == "beh") {
                if (value == "zig") {
                    spawn._behavior = EnemyEntity::Behavior::Zigging;
                } else if (value == "down") {
                    spawn._behavior = EnemyEntity::Behavior::Down;
                }
            } else if (key == "hp") {
                spawn._hp = std::stoi(value);
            } else if (key == "downv") {
                spawn._downSpeed = std::stof(value);
            } else if (key == "note_beh") {
                if (value == "minor") {
                    spawn._laneNoteBehavior = EnemyEntity::LaneNoteBehavior::Minor;
                }
            } else if (key == "hit_beh") {
                if (value == "multiphase") {
                    spawn._onHitBehavior = EnemyEntity::OnHitBehavior::MultiPhase;
                }
            } else if (key == "lane_note") {
                spawn._laneNoteOffsets.push_back(std::stoi(value));
            } else {
                printf("Unrecognized key \"%s\".\n", key.c_str());
            }
        }

        spawns->push_back(spawn);
    }
}
}

void PlayerEntity::Init(GameManager& g) {
    std::map<std::string, LaneNotesTable> laneNotesTables;
    LoadLaneNoteTablesFromFile("data/lane_note_tables.txt", laneNotesTables);
    
    std::vector<Spawn> spawns;
    LoadSpawnsFromFile("data/spawns_tech.txt", &spawns);
    for (Spawn const& spawn : spawns) {
        MakeNoteEnemy(g, spawn);
    }
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
}
