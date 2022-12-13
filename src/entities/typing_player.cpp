#include "typing_player.h"

#include <cctype>
#include <string_view>
#include <fstream>
#include <sstream>
#include <map>

#include "input_manager.h"
#include "game_manager.h"
#include "entities/typing_enemy.h"
#include "audio.h"
#include "midi_util.h"
#include "rng.h"
#include "constants.h"

namespace {

struct SpawnInfo {
    std::string _text;
    int _randTextLength = -1;  // -1 means no random text
    float _x = 0.f;
    float _z = 0.f;
    double _activeBeatTime = -1.0;
    double _inactiveBeatTime = -1.0;
    int _channel = 0;
    int _midiNote = -1;
    float _noteVelocity = 1.f;
    double _noteLength = -1.0;
    // If "duck" = true, then we ignore midiNote, noteVelocity, noteLength, etc and just apply a quick ducking to the gain. This is a hack obviously.
    bool _duck = false;
    
    // If oscFaderValue >= 0, then this takes precedence over midi notes.
    // If _noteLength >= 0, add another event to revert to previous value.
    // double _oscFaderValue = -1.0;
    Vec3 _velocity;
    std::string _seqEntityName;
    // std::vector<BeatTimeEvent>* _eventSeq = nullptr;
    std::vector<int>* _noteTable = nullptr;
    enum class NoteTableType {
        Random, HitSeq        
    };
    NoteTableType _noteTableType = NoteTableType::Random;
};

std::map<std::string, std::vector<int>> gNoteTables;

void LoadNoteTables() {
    {
        std::vector<int>& table = gNoteTables["bass1"];
        table.push_back(GetMidiNote("G2"));
        table.push_back(GetMidiNote("B2-"));
        table.push_back(GetMidiNote("C3"));
        table.push_back(GetMidiNote("E3-"));
        table.push_back(GetMidiNote("G3"));
        table.push_back(GetMidiNote("B3-"));
    }

    {
        std::vector<int>& table = gNoteTables["bass2"];
        table.push_back(GetMidiNote("C2"));
        table.push_back(GetMidiNote("C3"));
    }
    
    {
        std::vector<int>& table = gNoteTables["bassDGA"];
        table.push_back(GetMidiNote("D2"));
        table.push_back(GetMidiNote("G2"));
        table.push_back(GetMidiNote("A2"));
    }

    {
        std::vector<int>& table = gNoteTables["bassGA"];
        table.push_back(GetMidiNote("G2"));
        table.push_back(GetMidiNote("A2"));
    }

    {
        std::vector<int>& table = gNoteTables["bassGB-"];
        table.push_back(GetMidiNote("G2"));
        table.push_back(GetMidiNote("B2-"));
    }

    {
        std::vector<int>& table = gNoteTables["bassDGB-"];
        table.push_back(GetMidiNote("D2"));
        table.push_back(GetMidiNote("G2"));
        table.push_back(GetMidiNote("B2-"));
    }

    {
        std::vector<int>& table = gNoteTables["bassSoloA"];
        table = {
            GetMidiNote("C3"),
            GetMidiNote("G3"),
            GetMidiNote("F3"),
            GetMidiNote("E3-")
        };
    }
    {
        std::vector<int>& table = gNoteTables["bassSoloB"];
        table = {
            GetMidiNote("G3"),
            GetMidiNote("B3-"),
            GetMidiNote("C4"),
            GetMidiNote("C3")
        };
    }
}

std::string GenerateRandomText(int numChars) {
    std::string text;
    text.reserve(numChars);
    for (int i = 0; i < numChars; ++i) {
        int randCharIx = rng::GetInt(0, 25);
        char randChar = 'a' + (char) randCharIx;
        text.push_back(randChar);
    }
    return text;
}

void SpawnEnemy(GameManager& g, SpawnInfo const& spawn) {
    TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->AddEntity(ne::EntityType::TypingEnemy));

    enemy->_modelName = "cube";
    enemy->_modelColor.Set(1.f, 0.f, 0.f, 1.f);

    enemy->_text = spawn._text;
    enemy->_transform._m03 = spawn._x;
    enemy->_transform._m23 = spawn._z;
    enemy->_activeBeatTime = spawn._activeBeatTime;
    enemy->_inactiveBeatTime = spawn._inactiveBeatTime;

    // enemy->_eventStartDenom = 0.25;
    double constexpr kEventStartDenom = 0.25;

    enemy->_velocity = spawn._velocity;

    ne::Entity* seqEntity = nullptr;
    if (!spawn._seqEntityName.empty()) {
        seqEntity = g._neEntityManager->FindEntityByName(spawn._seqEntityName);
    }    

    // if (spawn._oscFaderValue >= 0.0) {
    //     {
    //         enemy->_hitEvents.emplace_back();
    //         BeatTimeEvent& b_e = enemy->_hitEvents.back();
    //         b_e._beatTime = 0.0;
    //         b_e._e.type = audio::EventType::SynthParam;
    //         b_e._e.channel = spawn._channel;
    //         b_e._e.param = audio::SynthParamType::OscFader;
    //         b_e._e.newParamValue = spawn._oscFaderValue;
    //     }
    //     if (spawn._noteLength >= 0.0) {
    //         float currValue = g._audioContext->_state.synths[spawn._channel].patch.oscFader;
    //         enemy->_hitEvents.emplace_back();
    //         BeatTimeEvent& b_e = enemy->_hitEvents.back();
    //         b_e._beatTime = spawn._noteLength;
    //         b_e._e.type = audio::EventType::SynthParam;
    //         b_e._e.channel = spawn._channel;
    //         b_e._e.param = audio::SynthParamType::OscFader;
    //         b_e._e.newParamValue = currValue;
    //     }
    //     // BLAH
    //     enemy->_deathEvents = enemy->_hitEvents;
        
    // } else if (spawn._noteTable) {
    if (spawn._duck) {
        enemy->_hitBehavior = TypingEnemyEntity::HitBehavior::AllActions;
        
        auto pA = std::make_unique<BeatTimeEventSeqAction>();
        pA->_b_e._beatTime = 0.0;
        pA->_b_e._e.type = audio::EventType::SynthParam;
        pA->_b_e._e.channel = spawn._channel;
        pA->_b_e._e.param = audio::SynthParamType::Gain;
        double constexpr kChangeSeconds = 0.05;
        long constexpr kChangeTicks = (long)(kChangeSeconds * SAMPLE_RATE);
        pA->_b_e._e.paramChangeTime = kChangeTicks;
        pA->_b_e._e.newParamValue = 0.0f;
        enemy->_hitActions.push_back(std::move(pA));

        pA = std::make_unique<BeatTimeEventSeqAction>();
        pA->_b_e._beatTime = 0.25;
        pA->_b_e._e.type = audio::EventType::SynthParam;
        pA->_b_e._e.channel = spawn._channel;
        pA->_b_e._e.param = audio::SynthParamType::Gain;
        pA->_b_e._e.paramChangeTime = kChangeTicks;
        pA->_b_e._e.newParamValue = 0.677999973f;
        enemy->_hitActions.push_back(std::move(pA));
    } else if (spawn._noteTable) {
        
        switch (spawn._noteTableType) {
            case SpawnInfo::NoteTableType::Random: {
                int randNoteIx = rng::GetInt(0, spawn._noteTable->size()-1);
                int midiNote = spawn._noteTable->at(randNoteIx);
                std::unique_ptr<SeqAction> pAction;
                if (seqEntity) {
                    auto pA = std::make_unique<ChangeStepSequencerSeqAction>();
                    pA->_seqId = seqEntity->_id;
                    pA->_midiNote = midiNote;
                    pA->_velocity = 1.f;
                    pAction = std::move(pA);
                } else {
                    auto pA = std::make_unique<NoteOnOffSeqAction>();
                    pA->_channel = spawn._channel;
                    pA->_midiNote = midiNote;
                    pA->_noteLength = spawn._noteLength;
                    pA->_velocity = spawn._noteVelocity;
                    pA->_quantizeDenom = kEventStartDenom;
                    pAction = std::move(pA);
                }
                enemy->_hitActions.push_back(std::move(pAction));
                break;
            }
            case SpawnInfo::NoteTableType::HitSeq: {                
                for (int midiNote : *spawn._noteTable) {
                    std::unique_ptr<SeqAction> pAction;
                    if (seqEntity) {
                        auto pA = std::make_unique<ChangeStepSequencerSeqAction>();
                        pA->_seqId = seqEntity->_id;
                        pA->_midiNote = midiNote;
                        pA->_velocity = 1.f;  // TODO: was this actually 0 before????
                        pAction = std::move(pA);                        
                    } else {
                        auto pA = std::make_unique<NoteOnOffSeqAction>();
                        pA->_channel = spawn._channel;
                        pA->_midiNote = midiNote;
                        pA->_noteLength = spawn._noteLength;
                        pA->_velocity = spawn._noteVelocity;
                        pAction = std::move(pA);
                    }
                    enemy->_hitActions.push_back(std::move(pAction));
                }
                break;
            }
        }
    } else {
        // No note table, just use the one note.
        std::unique_ptr<SeqAction> pAction;           
        if (seqEntity) {
            auto pA = std::make_unique<ChangeStepSequencerSeqAction>();
            pA->_seqId = seqEntity->_id;
            pA->_midiNote = spawn._midiNote;
            pA->_velocity = 1.f;
            pAction = std::move(pA);
        } else {
            auto pA = std::make_unique<NoteOnOffSeqAction>();
            pA->_channel = spawn._channel;
            pA->_midiNote = spawn._midiNote;
            pA->_noteLength = spawn._noteLength;
            pA->_velocity = spawn._noteVelocity;
            pAction = std::move(pA);
        }
        enemy->_hitActions.push_back(std::move(pAction));
    }
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
            if (key == "on") {
                spawn._activeBeatTime = std::stod(value) + beatTimeOffset;
            } else if (key == "off") {
                spawn._inactiveBeatTime = std::stod(value) + beatTimeOffset;
            } else if (key == "text") {
                spawn._text = value;
            } else if (key == "rand_text_len") {
                int numChars = std::stoi(value);
                spawn._text = GenerateRandomText(numChars);
            } else if (key == "x") {
                spawn._x = std::stof(value);
            } else if (key == "z") {
                spawn._z = std::stof(value);
            } else if (key == "vx") {
                spawn._velocity._x = std::stof(value);
            } else if (key == "vz") {
                spawn._velocity._z = std::stof(value);
            } else if (key == "note") {
                int midiNote = GetMidiNote(value.c_str());
                spawn._midiNote = midiNote;
            } else if (key == "channel") {
                spawn._channel = std::stoi(value);
            } else if (key == "note_vel") {
                spawn._noteVelocity = std::stof(value);
            } else if (key == "length") {
                spawn._noteLength = std::stod(value);
            } /*else if (key == "osc_fader") {
                spawn._oscFaderValue = std::stod(value);
                }*/
            else if (key == "rand_pos") {
                spawn._x = rng::GetFloat(-8.f, 8.f);
                spawn._z = rng::GetFloat(-5.f, 5.f);                
            } else if (key == "rand_pv") {                
                // float constexpr kSpeed = 5.f;
                // float constexpr kSpawnRadius = 6.f;
                // float randAngle = rng::GetFloat(-kPi, kPi);                
                // spawn._x = cos(randAngle);
                // spawn._z = sin(randAngle);

                // spawn._velocity.Set(-spawn._x, 0.f, -spawn._z);
                
                // spawn._x *= kSpawnRadius;
                // spawn._z *= kSpawnRadius;
                // spawn._velocity = spawn._velocity * kSpeed;
                static bool gHorizontal = true;
                float constexpr kSpeed = 5.f;
                float constexpr kHorizLimit = 7.f;
                float constexpr kZMin = -10.f;
                float constexpr kZMax = 5.5f;
                if (gHorizontal) {                    
                    bool leftSide = rng::GetInt(0, 1) == 1;
                    
                    spawn._x = (leftSide ? -1 : 1) * kHorizLimit;
                    spawn._z = rng::GetFloat(kZMin, kZMax);
                    spawn._velocity.Set(kSpeed * (leftSide ? 1.f : -1.f), 0.f, 0.f);
                } else {
                    bool topSide = rng::GetInt(0, 1) == 1;

                    spawn._x = rng::GetFloat(-kHorizLimit, kHorizLimit);
                    spawn._z = topSide ? kZMin : kZMax;
                    spawn._velocity.Set(0.f, 0.f, kSpeed * (topSide ? 1.f : -1.f));
                }
                gHorizontal = !gHorizontal;
            } else if (key == "seq") {
                spawn._seqEntityName = value;
            } else if (key == "rand_note") {
                auto result = gNoteTables.find(value);
                if (result == gNoteTables.end()) {
                    printf("ERROR: no note table named %s\n", value.c_str());
                } else {
                    spawn._noteTable = &(result->second);
                    spawn._noteTableType = SpawnInfo::NoteTableType::Random;
                }
            } else if (key == "hit_seq") {
                auto result = gNoteTables.find(value);
                if (result == gNoteTables.end()) {
                    printf("ERROR: no note table named %s\n", value.c_str());
                } else {
                    spawn._noteTable = &(result->second);
                    spawn._noteTableType = SpawnInfo::NoteTableType::HitSeq;
                }
            } else if (key == "duck") {
                spawn._duck = true;
            } else {
                printf("Unrecognized key \"%s\".\n", key.c_str());
            }
        }
        
        SpawnEnemy(g, spawn);
    }
}
}

void TypingPlayerEntity::Init(GameManager& g) {
    LoadNoteTables();
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
    // Vec3 const& playerPos = _transform.GetPos();
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
            // Vec3 dp = playerPos - enemy->_transform.GetPos();
            // float dist2 = dp.Length2();
            float dist2 = -enemy->_transform.GetPos()._z;
            if (nearest == nullptr) {
                nearest = enemy;
                nearestDist2 = dist2;
            } else if (dist2 < nearestDist2) {
                nearest = enemy;
                nearestDist2 = dist2;
            }
        }
    }

    if (nearest != nullptr) {
        nearest->OnHit(g);
    }
}
