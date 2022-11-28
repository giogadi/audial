#include "lane_note_player.h"

#include <array>
#include <fstream>
#include <sstream>
#include <map>

#include "game_manager.h"
#include "input_manager.h"
#include "renderer.h"
#include "constants.h"
#include "color_presets.h"
#include "sound_bank.h"

namespace {

// Draw them in 2 sets of 4.
void DrawLanes(GameManager& g) {
    float startX = -0.5f * kNumLanes * kLaneWidth;
    Vec4 color(0.f, 0.f, 0.f, 1.f);
    for (int i = 0; i < kNumLanes+1; ++i) {
        Mat4 transform;
        transform.SetTranslation(Vec3(startX, 0.f, -2.f));
        transform.Scale(kLaneBoundaryWidth, kLaneBoundaryWidth, 20.f);
        if (i == kNumLanes / 2) {
            g._scene->DrawCube(transform, Vec4(1.f, 1.f, 1.f, 1.f));
        } else {
            g._scene->DrawCube(transform, color);
        }        
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
    int _laneIx = 0;
    float _z = 0.f;
    LaneNoteEnemyEntity::Behavior _behavior = LaneNoteEnemyEntity::Behavior::None;
    LaneNoteEnemyEntity::OnHitBehavior _onHitBehavior = LaneNoteEnemyEntity::OnHitBehavior::Default;
    int _hp = 1;
    int _numHpBars = 1;
    float _constVelX = 0.f;
    float _constVelZ = 0.f;
    std::string _laneNoteTableName;
    std::string _phaseTableName;
    std::string _deathTableName;
    std::string _pcmName;
    Vec4 _color = ToColor4(ColorPreset::Orange);
    float _velocity = 1.f;
};

ne::Entity* MakeNoteEnemy(
    GameManager& g, Spawn const& spawnInfo, std::map<std::string, LaneNoteEnemyEntity::LaneNotesTable> const& laneNotesTableMap) {
    LaneNoteEnemyEntity* enemy = (LaneNoteEnemyEntity*) g._neEntityManager->AddEntity(ne::EntityType::LaneNoteEnemy);
    enemy->_modelName = "cube";
    enemy->_modelColor = spawnInfo._color;
    enemy->_eventStartDenom = 0.25;
    enemy->_behavior = spawnInfo._behavior;
    enemy->_initialHp = spawnInfo._hp;
    enemy->_constVelX = spawnInfo._constVelX;
    enemy->_constVelZ = spawnInfo._constVelZ;
    enemy->_onHitBehavior = spawnInfo._onHitBehavior;
    enemy->_numHpBars = spawnInfo._numHpBars;

    if (!spawnInfo._pcmName.empty()) {
        enemy->_laneNoteBehavior = LaneNoteEnemyEntity::LaneNoteBehavior::Events;
        int soundIx = g._soundBank->GetSoundIx(spawnInfo._pcmName.c_str());
        if (soundIx < 0) {
            printf("WARNING: unrecognized sound \"%s\".", spawnInfo._pcmName.c_str());
        } else {
            enemy->_events.emplace_back();
            BeatTimeEvent& b_e = enemy->_events.back();
            b_e._e.type = audio::EventType::PlayPcm;
            b_e._e.pcmSoundIx = soundIx;
            b_e._beatTime = 0.0;
            b_e._e.pcmVelocity = spawnInfo._velocity;
        }
    }

    {
        if (!spawnInfo._laneNoteTableName.empty()) {
            auto result = laneNotesTableMap.find(spawnInfo._laneNoteTableName);
            if (result == laneNotesTableMap.end()) {
                printf("WARNING: lane-note-table enemy refers to non-existent table \"%s\"!\n", spawnInfo._laneNoteTableName.c_str());
            } else {
                enemy->_laneNotesTable = &(result->second);
            }
        }        

        if (!spawnInfo._phaseTableName.empty()) {
            auto result = laneNotesTableMap.find(spawnInfo._phaseTableName);
            if (result == laneNotesTableMap.end()) {
                printf("WARNING: lane-note-table enemy refers to non-existent table \"%s\"!\n", spawnInfo._phaseTableName.c_str());
            } else {
                enemy->_phaseNotesTable = &(result->second);
            }
        }

        if (!spawnInfo._deathTableName.empty()) {
            auto result = laneNotesTableMap.find(spawnInfo._deathTableName);
            if (result == laneNotesTableMap.end()) {
                printf("WARNING: lane-note-table enemy refers to non-existent table \"%s\"!\n", spawnInfo._deathTableName.c_str());
            } else {
                enemy->_deathNotesTable = &(result->second);
            }
        }
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

void LoadLaneNoteTablesFromFile(
    char const* fileName, std::map<std::string, LaneNoteEnemyEntity::LaneNotesTable>& laneNotesTables) {
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

        LaneNoteEnemyEntity::LaneNotesTable& table = laneNotesTables[tableName];

        // After table name, on same line, we do some key,value pairs.
        std::string token;
        while (!lineSs.eof()) {
            std::string key, value;
            {
                lineSs >> token;
                std::size_t delimIx = token.find_first_of(':');
                if (delimIx == std::string::npos) {
                    printf("Token missing \":\" - \"%s\"\n", token.c_str());
                    continue;
                }

                key = token.substr(0, delimIx);
                value = token.substr(delimIx+1);
            }
            if (key == "ch") {
                table._channel = std::stoi(value);
            } else if (key == "note_length") {
                table._noteLength = std::stod(value);
            } else {
                printf("Unrecognized key \"%s\".\n", key.c_str());
            }
        }
        
        for (int laneIx = 0; laneIx < kNumLanes; ++laneIx) {
            table._table[laneIx].fill(-1);
            std::getline(inFile, line);
            ++lineNumber;
            std::stringstream lineSs(line);
            for (int noteIx = 0; noteIx < LaneNoteEnemyEntity::kMaxNumNotesPerLane && !lineSs.eof(); ++noteIx) {
                std::string noteName;
                lineSs >> noteName;
                if (IsOnlyWhitespace(noteName)) {
                    continue;
                }
                int midiNote = GetMidiNote(noteName.c_str());
                table._table[laneIx][noteIx] = midiNote;
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
            } else if (key == "lane") {
                spawn._laneIx = std::stoi(value);
            } else if (key == "z") {
                spawn._z = std::stof(value);
            } else if (key == "beh") {
                if (value == "zig") {
                    spawn._behavior = LaneNoteEnemyEntity::Behavior::Zigging;
                } else if (value == "const_vel") {
                    spawn._behavior = LaneNoteEnemyEntity::Behavior::ConstVel;
                } else if (value == "move_on_phase") {
                    spawn._behavior = LaneNoteEnemyEntity::Behavior::MoveOnPhase;
                } else if (value == "down") {
                    spawn._behavior = LaneNoteEnemyEntity::Behavior::ConstVel;
                } else {
                    printf("Unrecognized behavior \"%s\".", value.c_str());
                }
            } else if (key == "hp") {
                spawn._hp = std::stoi(value);
            } else if (key == "xvel") {
                spawn._constVelX = std::stof(value);
            } else if (key == "zvel") {
                spawn._constVelZ = std::stof(value);
            } else if (key == "downv") {
                spawn._constVelZ = std::stof(value);
            }else if (key == "hit_beh") {
                if (value == "multiphase") {
                    spawn._onHitBehavior = LaneNoteEnemyEntity::OnHitBehavior::MultiPhase;
                }
            } else if (key == "num_phases") {
                // multiphase only
                spawn._numHpBars = std::stoi(value);
            } else if (key == "table") {
                spawn._laneNoteTableName = value;
            } else if (key == "phase_table") {
                spawn._phaseTableName = value;
            } else if (key == "death_table") {
                spawn._deathTableName = value;
            } else if (key == "color") {
                spawn._color = ToColor4(StringToColorPreset(value.c_str()));
            } else if (key == "pcm") {
                spawn._pcmName = value;
            } else if (key == "v") {
                spawn._velocity = std::stof(value);
            } else {
                printf("Unrecognized key \"%s\".\n", key.c_str());
            }
        }

        spawns->push_back(spawn);
    }
}
}

void LaneNotePlayerEntity::Init(GameManager& g) {
    LoadLaneNoteTablesFromFile("data/lane_note_tables.txt", _laneNotesTables);
    
    std::vector<Spawn> spawns;
    LoadSpawnsFromFile(_spawnsFilename.c_str(), &spawns);
    for (Spawn const& spawn : spawns) {
        MakeNoteEnemy(g, spawn, _laneNotesTables);
    }
}

void LaneNotePlayerEntity::Update(GameManager& g, float dt) {
    DrawLanes(g);

    // HOWDY DEBUG TEXT
    g._scene->DrawText("HOWDY", 200, 200);

    // assume only a few keys can be pressed at once.
    int constexpr kNumSimulKeys = 4;
    struct KeyInfo {
        InputManager::Key _key;
        LaneNoteEnemyEntity* _nearest = nullptr;
        float _dist = 0.f;
    };
    std::array<KeyInfo, kNumSimulKeys> keysShot;
    int numKeysShot = 0;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::LaneNoteEnemy);
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        LaneNoteEnemyEntity* enemy = (LaneNoteEnemyEntity*) enemyIter.GetEntity();
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

void LaneNotePlayerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("spawns_filename", _spawnsFilename.c_str());
}

void LaneNotePlayerEntity::LoadDerived(serial::Ptree pt) {
    _spawnsFilename = pt.GetString("spawns_filename");
}
