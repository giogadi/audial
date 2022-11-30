#include "typing_player.h"

#include <cctype>
#include <string_view>
#include <fstream>
#include <sstream>

#include "input_manager.h"
#include "game_manager.h"
#include "entities/typing_enemy.h"

namespace {
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

struct SpawnInfo {
    std::string _text;
    float _x = 0.f;
    float _z = 0.f;
    double _activeBeatTime = -1.0;
    double _inactiveBeatTime = -1.0;
    int _channel = 0;
    int _midiNote = -1;
    double _noteLength = 0.0;
};

void SpawnEnemy(GameManager& g, SpawnInfo const& spawn) {
    TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->AddEntity(ne::EntityType::TypingEnemy));

    enemy->_modelName = "cube";
    enemy->_modelColor.Set(1.f, 0.f, 0.f, 1.f);

    enemy->_text = spawn._text;
    enemy->_transform._m03 = spawn._x;
    enemy->_transform._m23 = spawn._z;
    enemy->_activeBeatTime = spawn._activeBeatTime;
    enemy->_inactiveBeatTime = spawn._inactiveBeatTime;

    {
        enemy->_hitEvents.emplace_back();
        BeatTimeEvent& b_e = enemy->_hitEvents.back();
        b_e._beatTime = 0.0;
        b_e._e.type = audio::EventType::NoteOn;
        b_e._e.channel = spawn._channel;
        b_e._e.midiNote = spawn._midiNote;
    }
    {
        enemy->_hitEvents.emplace_back();
        BeatTimeEvent& b_e = enemy->_hitEvents.back();
        b_e._beatTime = spawn._noteLength;
        b_e._e.type = audio::EventType::NoteOff;
        b_e._e.channel = spawn._channel;
        b_e._e.midiNote = spawn._midiNote;        
    }

    // BLAH
    enemy->_deathEvents = enemy->_hitEvents;
}

void SpawnEnemiesFromFile(std::string_view filename, GameManager& g) {
    std::ifstream inFile(filename);
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

        SpawnInfo spawn;

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
                spawn._activeBeatTime = std::stod(value) + beatTimeOffset;
            } else if (key == "off") {
                spawn._inactiveBeatTime = std::stod(value) + beatTimeOffset;
            } else if (key == "text") {
                spawn._text = value;
            }else if (key == "x") {
                spawn._x = std::stof(value);
            } else if (key == "z") {
                spawn._z = std::stof(value);
            } else if (key == "note") {
                int midiNote = GetMidiNote(value.c_str());
                spawn._midiNote = midiNote;
            } else if (key == "channel") {
                spawn._channel = std::stoi(value);
            } else if (key == "length") {
                spawn._noteLength = std::stod(value);
            } else {
                printf("Unrecognized key \"%s\".\n", key.c_str());
            }
        }
        
        SpawnEnemy(g, spawn);
    }
}
}

void TypingPlayerEntity::Init(GameManager& g) {
    if (!_spawnFilename.empty()) {
        SpawnEnemiesFromFile(_spawnFilename, g);
    }
}

void TypingPlayerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("spawn_filename", _spawnFilename.c_str());
}

void TypingPlayerEntity::LoadDerived(serial::Ptree pt) {
    pt.TryGetString("spawn_filename", &_spawnFilename);
}

void TypingPlayerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }
    
    TypingEnemyEntity* nearest = nullptr;
    float nearestDist2 = -1.f;
    ne::EntityManager::Iterator enemyIter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy);
    Vec3 const& playerPos = _transform.GetPos();
    for (; !enemyIter.Finished(); enemyIter.Next()) {
        TypingEnemyEntity* enemy = (TypingEnemyEntity*) enemyIter.GetEntity();
        if (!enemy->IsActive(g)) {
            continue;
        }
        if (enemy->_numHits >= enemy->_text.length()) {
            continue;
        }
        char nextChar = std::tolower(enemy->_text[enemy->_numHits]);
        int charIx = nextChar - 'a';
        if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
            printf("WARNING: char \'%c\' not in InputManager!\n", nextChar);
            continue;
        }
        InputManager::Key nextKey = static_cast<InputManager::Key>(charIx);
        if (g._inputManager->IsKeyPressedThisFrame(nextKey)) {
            if (nearest == nullptr) {
                nearest = enemy;
            } else {
                Vec3 dp = playerPos - enemy->_transform.GetPos();
                float dist2 = dp.Length2();
                if (dist2 < nearestDist2) {
                    nearest = enemy;
                    nearestDist2 = dist2;
                }
            }
        }
    }

    if (nearest != nullptr) {
        nearest->OnHit(g);
    }
}
