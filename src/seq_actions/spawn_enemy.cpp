#include "spawn_enemy.h"

#include <map>
#include <fstream>

#include "string_util.h"
#include "rng.h"
#include "audio.h"
#include "midi_util.h"
#include "sound_bank.h"
#include "color_presets.h"
#include "seq_actions/camera_control.h"

void SpawnEnemySeqAction::Execute(GameManager& g) {
    assert(!_done);
    TypingEnemyEntity* pEnemy = static_cast<TypingEnemyEntity*>(g._neEntityManager->AddEntity(ne::EntityType::TypingEnemy));
    // UGLY
    ne::EntityId id = pEnemy->_id;
    *pEnemy = std::move(_enemy);
    pEnemy->_id = id;
    pEnemy->Init(g);
    _done = true;
}

bool SpawnEnemySeqAction::_sNoteTablesLoaded = false;

namespace {

std::map<std::string, std::vector<std::array<int,4>>> gNoteTables;

void LoadNoteTables() {
    {
        std::vector<std::array<int,4>>& table = gNoteTables["bass1"];
        table.push_back({GetMidiNote("G2"),-1,-1,-1});
        table.push_back({GetMidiNote("B2-"),-1,-1,-1});
        table.push_back({GetMidiNote("C3"),-1,-1,-1});
        table.push_back({GetMidiNote("E3-"),-1,-1,-1});
        table.push_back({GetMidiNote("G3"),-1,-1,-1});
        table.push_back({GetMidiNote("B3-"),-1,-1,-1});
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bass2"];
        table.push_back({GetMidiNote("C2"),-1,-1,-1});
        table.push_back({GetMidiNote("C3"),-1,-1,-1});
    }
    
    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassDGA"];
        table.push_back({GetMidiNote("D2"),-1,-1,-1});
        table.push_back({GetMidiNote("G2"),-1,-1,-1});
        table.push_back({GetMidiNote("A2"),-1,-1,-1});
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassGA"];
        table.push_back({GetMidiNote("G2"),-1,-1,-1});
        table.push_back({GetMidiNote("A2"),-1,-1,-1});
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassGB-"];
        table.push_back({GetMidiNote("G2"),-1,-1,-1});
        table.push_back({GetMidiNote("B2-"),-1,-1,-1});
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassDGB-"];
        table.push_back({GetMidiNote("D2"),-1,-1,-1});
        table.push_back({GetMidiNote("G2"),-1,-1,-1});
        table.push_back({GetMidiNote("B2-"),-1,-1,-1});
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassSoloA"];
        table = {
            {GetMidiNote("C3"),-1,-1,-1},
            {GetMidiNote("G3"),-1,-1,-1},
            {GetMidiNote("F3"),-1,-1,-1},
            {GetMidiNote("E3-"),-1,-1,-1}
        };
    }
    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassSoloB"];
        table = {
            {GetMidiNote("G3"),-1,-1,-1},
            {GetMidiNote("B3-"),-1,-1,-1},
            {GetMidiNote("C4"),-1,-1,-1},
            {GetMidiNote("C3"),-1,-1,-1}
        };
    }
    {
        std::vector<std::array<int,4>>& table = gNoteTables["chordSeq1"];
        table = {
            {GetMidiNote("G3"), GetMidiNote("A3-"), GetMidiNote("C4"),-1},
            {GetMidiNote("A3-"), GetMidiNote("A3"), GetMidiNote("D4-"),-1},
            {GetMidiNote("A3"), GetMidiNote("B3-"), GetMidiNote("D4"),-1},
        };
    }
    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassSeq2"];
        table = {
            {GetMidiNote("F2"), -1, -1, -1},
            {GetMidiNote("F2"), -1, -1, -1},
            {GetMidiNote("F2"), -1, -1, -1},
            {GetMidiNote("A2-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassPair1"];
        table = {
            {GetMidiNote("F2"), -1, -1, -1},
            {GetMidiNote("F2+"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["bassPair2"];
        table = {
            {GetMidiNote("G1+"), -1, -1, -1},
            {GetMidiNote("A1"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["chordChrom1"];
        table = {
            {GetMidiNote("A3"), GetMidiNote("B3-"), GetMidiNote("D4"),-1},
            {GetMidiNote("A3-"), GetMidiNote("A3"), GetMidiNote("D4-"),-1},
            {GetMidiNote("A3"), GetMidiNote("B3-"), GetMidiNote("D4"),-1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo1"];
        table = {
            {GetMidiNote("C3"), -1, -1 ,-1},
            {GetMidiNote("D3"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2"];
        table = {
            {GetMidiNote("E3-"), -1, -1, -1},
            {GetMidiNote("F3"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo3"];
        table = {
            {GetMidiNote("G3"), -1, -1, -1},
            {GetMidiNote("B3-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo4"];
        table = {
            {GetMidiNote("C4"), -1, -1, -1},
            {GetMidiNote("D4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo5"];
        table = {
            {GetMidiNote("E4-"), -1, -1, -1},
            {GetMidiNote("F4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo6"];
        table = {
            {GetMidiNote("G4"), -1, -1, -1},
            {GetMidiNote("B4-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo6"];
        table = {
            {GetMidiNote("C5"), -1, -1, -1},
            {GetMidiNote("B4-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo7"];
        table = {
            {GetMidiNote("G4"), -1, -1, -1},
            {GetMidiNote("F4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo8"];
        table = {
            {GetMidiNote("E4-"), -1, -1, -1},
            {GetMidiNote("D4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo9"];
        table = {
            {GetMidiNote("C4"), -1, -1, -1},
            {GetMidiNote("B3-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo10"];
        table = {
            {GetMidiNote("G3"), -1, -1, -1},
            {GetMidiNote("F3"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo11"];
        table = {
            {GetMidiNote("E3-"), -1, -1, -1},
            {GetMidiNote("D3"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo12"];
        table = {
            {GetMidiNote("C3"), -1, -1, -1},
            {GetMidiNote("B2-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_1"];
        table = {
            {GetMidiNote("C4"), -1, -1, -1},
            {GetMidiNote("E4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_2"];
        table = {
            {GetMidiNote("F4"), -1, -1, -1},
            {GetMidiNote("B4-"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_3"];
        table = {
            {GetMidiNote("A4"), -1, -1, -1},
            {GetMidiNote("G4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_4"];
        table = {
            {GetMidiNote("F4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_5"];
        table = {
            {GetMidiNote("D5"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_6"];
        table = {
            {GetMidiNote("B4-"), -1, -1, -1},
            {GetMidiNote("A4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_7"];
        table = {
            {GetMidiNote("F5"), -1, -1, -1},
            {GetMidiNote("E5"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_8"];
        table = {
            {GetMidiNote("D5"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_9"];
        table = {
            {GetMidiNote("E5"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_10"];
        table = {
            {GetMidiNote("D5"), -1, -1, -1},
            {GetMidiNote("C5"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_11"];
        table = {
            {GetMidiNote("B4-"), -1, -1, -1},
            {GetMidiNote("C5"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_12"];
        table = {
            {GetMidiNote("D5"), -1, -1, -1},
            {GetMidiNote("A4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_13"];
        table = {
            {GetMidiNote("C4"), -1, -1, -1},
            {GetMidiNote("D4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_14"];
        table = {
            {GetMidiNote("E4-"), -1, -1, -1},
            {GetMidiNote("F4"), -1, -1, -1},
        };
    }

    {
        std::vector<std::array<int,4>>& table = gNoteTables["rookieLeadSolo2_15"];
        table = {
            {GetMidiNote("E4-"), -1, -1, -1},
        };
    }

    SpawnEnemySeqAction::_sNoteTablesLoaded = true;
}

// TODO!!!!!! are we going to consider beatTimeOffset?
void LoadParamEnemy(TypingEnemyEntity& enemy, std::istream& lineStream) {
    
    audio::SynthParamType paramType;
    std::vector<float> values;
    std::vector<float> times;
    int channel = 0;
    float fadeTime = 0.f;
    
    std::string token, key, value;
    while (!lineStream.eof()) {
        {
            lineStream >> token;
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("LoadParamEnemy: Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }

        if (key == "param") {
            paramType = audio::StringToSynthParamType(value.c_str());
        } else if (key == "v") {
            values.push_back(std::stof(value));
        } else if (key == "t") {
            times.push_back(std::stof(value));
        } else if (key == "fade_t") {
            fadeTime = std::stof(value);
        } else if (key == "channel") {
            channel = std::stoi(value);
        } else {
            printf("LoadParamEnemy: unrecognized key \"%s\"\n", key.c_str());
        }        
    }

    if (values.size() != times.size()) {
        printf("LoadParamEnemy: there should be equal # of v's and t's (v:%lu, t:%lu)\n", values.size(), times.size());
        values.clear();
        times.clear();
    }
    
    enemy._hitBehavior = TypingEnemyEntity::HitBehavior::AllActions;
    
    for (int i = 0; i < values.size(); ++i) {
        auto pA = std::make_unique<BeatTimeEventSeqAction>();
        pA->_b_e._beatTime = times[i];
        pA->_b_e._e.type = audio::EventType::SynthParam;
        pA->_b_e._e.channel = channel;
        pA->_b_e._e.param = paramType;
        pA->_b_e._e.paramChangeTime = (long)(fadeTime * SAMPLE_RATE);
        pA->_b_e._e.newParamValue = values[i];
        enemy._hitActions.push_back(std::move(pA));
    }
}

void LoadSeqEnemy(GameManager& g, SeqAction::LoadInputs const& loadInputs, TypingEnemyEntity& enemy, std::istream& lineStream) {
    ne::Entity* seqEntity = nullptr;
    std::array<int, 4> midiNotes = {-1, -1, -1, -1};
    int currentNoteIx = 0;
    std::vector<std::array<int,4>>* noteTable = nullptr;
    float velocity = 1.f;
    bool velOnly = false;
    bool saveSeqChange = loadInputs._defaultEnemiesSave;
    std::string token, key, value;
    while (!lineStream.eof()) {
        {
            lineStream >> token;
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("LoadSeqEnemy: Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }

        if (key == "seq") {
            seqEntity = g._neEntityManager->FindEntityByName(value);
            if (seqEntity == nullptr) {
                printf("LoadSeqEnemy: can't find seq entity \"%s\"\n", value.c_str());
            }
        } else if (key == "note") {
            if (currentNoteIx >= midiNotes.size()) {
                printf("LoadSeqEnemy: too many notes!\n");
            } else {
                midiNotes[currentNoteIx] = GetMidiNote(value.c_str());
                ++currentNoteIx;
            }
        } else if (key == "rand_note") {
            auto result = gNoteTables.find(value);
            if (result == gNoteTables.end()) {
                printf("ERROR: no note table named %s\n", value.c_str());
            } else {
                auto const& randNoteTable = result->second;
                int randNoteIx = rng::GetInt(0, randNoteTable.size()-1);
                midiNotes = randNoteTable.at(randNoteIx);
            }
        } else if (key == "note_list") {
            auto result = gNoteTables.find(value);
            if (result == gNoteTables.end()) {
                printf("ERROR: no note table named %s\n", value.c_str());
            } else {
                noteTable = &(result->second);
            }
        } else if (key == "note_vel") {
            velocity = std::stof(value);
        } else if (key == "vel_only") {
            velOnly = true;
        } else if (key == "save") {
            saveSeqChange = true;
        } else if (key == "all_actions") {
            enemy._hitBehavior = TypingEnemyEntity::HitBehavior::AllActions;
        } else {
            printf("LoadSeqEnemy: unrecognized key \"%s\"\n", key.c_str());
        }
    }

    if (seqEntity == nullptr) {
        printf("LoadSeqEnemy: ERROR: could not find seq entity.\n");
        return;
    }

    if (noteTable) {
        for (std::array<int,4> const& n : *noteTable) {
            auto pAction = std::make_unique<ChangeStepSequencerSeqAction>();
            pAction->_seqId = seqEntity->_id;
            pAction->_midiNotes = n;
            pAction->_velocity = velocity;
            pAction->_velOnly = velOnly;
            pAction->_temporary = !saveSeqChange;
            enemy._hitActions.push_back(std::move(pAction));
        }
    } else {
        auto pAction = std::make_unique<ChangeStepSequencerSeqAction>();
        pAction->_seqId = seqEntity->_id;
        pAction->_midiNotes = midiNotes;
        pAction->_velocity = velocity;
        pAction->_velOnly = velOnly;
        pAction->_temporary = !saveSeqChange;
        enemy._hitActions.push_back(std::move(pAction));
    }
}

// TODO: make this support chords
void LoadNoteEnemy(TypingEnemyEntity& enemy, std::istream& lineStream) {
    int midiNote = -1;
    std::vector<std::array<int, 4>>* noteTable = nullptr;
    float velocity = 1.f;
    int channel = 0;
    double noteLength = 0.25;
    
    std::string token, key, value;
    while (!lineStream.eof()) {
        {
            lineStream >> token;
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("LoadNoteEnemy: Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }

        if (key == "note") {
            midiNote = GetMidiNote(value.c_str());
        } else if (key == "rand_note") {
            auto result = gNoteTables.find(value);
            if (result == gNoteTables.end()) {
                printf("ERROR: no note table named %s\n", value.c_str());
            } else {
                auto const& randNoteTable = result->second;
                int randNoteIx = rng::GetInt(0, randNoteTable.size()-1);
                // HOWDYYYYY
                midiNote = randNoteTable.at(randNoteIx)[0];
            }
        } else if (key == "note_list") {
            auto result = gNoteTables.find(value);
            if (result == gNoteTables.end()) {
                printf("ERROR: no note table named %s\n", value.c_str());
            } else {
                noteTable = &(result->second);
            }
        } else if (key == "note_vel") {
            velocity = std::stof(value);
        } else if (key == "channel") {
            channel = std::stoi(value);
        } else if (key == "length") {
            noteLength = std::stod(value);
        } else {
            printf("LoadNoteEnemy: unrecognized key \"%s\"\n", key.c_str());
        }
    }

    if (noteTable) {
        for (std::array<int, 4> const& n : *noteTable) {
            auto pAction = std::make_unique<NoteOnOffSeqAction>();
            pAction->_channel = channel;
            // HOWDYYYYY
            pAction->_midiNote = n[0];
            pAction->_noteLength = noteLength;
            pAction->_velocity = velocity;
            enemy._hitActions.push_back(std::move(pAction));
        }
    } else {
        auto pAction = std::make_unique<NoteOnOffSeqAction>();
        pAction->_channel = channel;
        pAction->_midiNote = midiNote;
        pAction->_noteLength = noteLength;
        pAction->_velocity = velocity;
        enemy._hitActions.push_back(std::move(pAction));
    }
}

void LoadPcmEnemy(GameManager& g, TypingEnemyEntity& enemy, std::istream& lineStream) {
    float velocity = 1.f;
    int soundIx = -1;
    
    std::string token, key, value;
    while (!lineStream.eof()) {
        {
            lineStream >> token;
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("LoadNoteEnemy: Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }

        if (key == "sound") {
            soundIx = g._soundBank->GetSoundIx(value.c_str());
            if (soundIx < 0) {
                printf("ERROR: no sound with name \"%s\"\n", value.c_str());
            }
        } else if (key == "vel") {
            velocity = std::stof(value);
        } else {
            printf("LoadPcmEnemy: unrecognized key \"%s\"\n", key.c_str());
        }
    }

    if (soundIx >= 0) {
        auto pAction = std::make_unique<BeatTimeEventSeqAction>();
        pAction->_b_e._beatTime = 0.0;
        pAction->_b_e._e.type = audio::EventType::PlayPcm;
        pAction->_b_e._e.pcmSoundIx = soundIx;
        pAction->_b_e._e.pcmVelocity = velocity;
        pAction->_b_e._e.loop = false;
        enemy._hitActions.push_back(std::move(pAction));
    } else {
        printf("LoadPcmEnemy: no valid sound index!\n");
    }   
}

void LoadActionEnemy(GameManager& g, TypingEnemyEntity& enemy, std::istream& lineStream) {
    std::string seqFilename;
    lineStream >> seqFilename;
    std::ifstream seqFile(seqFilename);
    if (!seqFile.is_open()) {
        printf("LoadActionEnemy ERROR: could not open file \"%s\"\n", seqFilename.c_str());
        return;
    }
    {
        std::vector<BeatTimeAction> actions;
        SeqAction::LoadActions(g, seqFile, actions);
        // UGLY
        enemy._hitActions.reserve(actions.size());
        for (BeatTimeAction& action : actions) {
            enemy._hitActions.push_back(std::move(action._pAction));
        }
    }    
}

} // namespace

void SpawnEnemySeqAction::Load(GameManager& g, LoadInputs const& loadInputs, std::istream& lineStream) {
    if (!_sNoteTablesLoaded) {
        LoadNoteTables();
    }

    _enemy._modelName = "cube";
    _enemy._modelColor.Set(1.f, 0.f, 0.f, 1.f);

    _enemy._transform.Scale(0.2f, 0.2f, 0.2f);
    
    // Read in common properties until we read a type:blah one. After that, all
    // properties are assumed to be specific to that enemy "type".
    std::string token, key, value;
    while (!lineStream.eof()) {
        {
            lineStream >> token;
            if (token.empty()) {
                continue;
            }
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("SpawnEnemySeqAction::Load: Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }

        if (key == "type") {
            if (value == "param") {
                LoadParamEnemy(_enemy, lineStream);
            } else if (value == "note") {
                LoadNoteEnemy(_enemy, lineStream);
            } else if (value == "seq") {
                LoadSeqEnemy(g, loadInputs, _enemy, lineStream);
            } else if (value == "pcm") {
                LoadPcmEnemy(g, _enemy, lineStream);
            } else if (value == "action") {
                LoadActionEnemy(g, _enemy, lineStream);
            }
            break;
        } else if (key == "name") {
            _enemy._name = value;
        } else if (key == "color") {
            ColorPreset preset = StringToColorPreset(value.c_str());
            _enemy._modelColor = ToColor4(preset);
        } else if (key == "on") {
            _enemy._activeBeatTime = std::stod(value) + loadInputs._beatTimeOffset;
        } else if (key == "off") {
            _enemy._inactiveBeatTime = std::stod(value) + loadInputs._beatTimeOffset;
        } else if (key == "text") {
            assert(value.length() > 0);
            _enemy._text = value;
        } else if (key == "rand_text_len") {
            int numChars = std::stoi(value);
            _enemy._text = string_util::GenerateRandomText(numChars);
        } else if (key == "typekill") {
            _enemy._destroyAfterTyped = std::stoi(value) != 0;
        } else if (key == "section") {
            _enemy._sectionId = loadInputs._sectionId;
            // initialize to "inactive" by setting their start beat time to be
            // way in the future.
            _enemy._activeBeatTime = 99999.0;
        } else if (key == "flow_section") {
            _enemy._flowSectionId = loadInputs._sectionId;
        } else if (key == "x") {
            _enemy._transform._m03 = std::stof(value);
        } else if (key == "z") {
            _enemy._transform._m23 = std::stof(value);
        } else if (key == "vx") {
            _enemy._velocity._x = std::stof(value);
        } else if (key == "vz") {
            _enemy._velocity._z = std::stof(value);
        } else if (key == "rand_pos") {
            _enemy._transform._m03 = rng::GetFloat(-8.f, 8.f);
            _enemy._transform._m23 = rng::GetFloat(-5.f, 5.f);                
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
            float constexpr kSpeed = 2.5f;
            float constexpr kHorizLimit = 7.f;
            float constexpr kZMin = -10.f;
            float constexpr kZMax = 5.5f;
            if (gHorizontal) {                    
                bool leftSide = rng::GetInt(0, 1) == 1;
                    
                _enemy._transform._m03 = (leftSide ? -1 : 1) * kHorizLimit;
                _enemy._transform._m23 = rng::GetFloat(kZMin, kZMax);
                _enemy._velocity.Set(kSpeed * (leftSide ? 1.f : -1.f), 0.f, 0.f);
            } else {
                bool topSide = rng::GetInt(0, 1) == 1;

                _enemy._transform._m03 = rng::GetFloat(-kHorizLimit, kHorizLimit);
                _enemy._transform._m23 = topSide ? kZMin : kZMax;
                _enemy._velocity.Set(0.f, 0.f, kSpeed * (topSide ? 1.f : -1.f));
            }
            gHorizontal = !gHorizontal;
        } else if (key == "camera_target") {
            ne::BaseEntity* targetEntity = g._neEntityManager->FindEntityByName(value);
            if (targetEntity == nullptr) {
                printf("spawn_enemy:camera_target: could not find entity named \"%s\"\n", value.c_str());
            } else {
                auto pAction = std::make_unique<CameraControlSeqAction>();
                pAction->_targetEntityId = targetEntity->_id;
                pAction->_desiredTargetToCameraOffset.Set(0.f, 5.f, 0.f);
                _enemy._hitActions.push_back(std::move(pAction));
            }           
        } else if (key == "camera_offset") {
            assert(false);  // UNSUPPORTED
            // auto pAction = std::make_unique<CameraControlSeqAction>();
            // if (string_util::EqualsIgnoreCase(value, "top")) {
            //     pAction->_desiredTargetToCameraOffset.Set(0.f, 5.f, 3.f);
            // } else if (string_util::EqualsIgnoreCase(value, "bottom")) {
            //     pAction->_desiredTargetToCameraOffset.Set(0.f, 5.f, -3.f);
            // } else if (string_util::EqualsIgnoreCase(value, "left")) {
            //     pAction->_desiredTargetToCameraOffset.Set(3.f, 5.f, 0.f);
            // } else if (string_util::EqualsIgnoreCase(value, "right")) {
            //     pAction->_desiredTargetToCameraOffset.Set(-3.f, 5.f, 0.f);
            // } else if (string_util::EqualsIgnoreCase(value, "center")) {
            //     pAction->_desiredTargetToCameraOffset.Set(0.f, 5.f, 0.f);
            // } else {
            //     printf("SpawnEnemy:camera_offset: unrecognized direction \"%s\"\n", value.c_str());
            // }
            // _enemy._hitActions.push_back(std::move(pAction));
        } else {
            printf("SpawnEnemySeqAction: unknown common key \"%s\"\n", key.c_str());
        }
    }

    if (_enemy._sectionId >= 0) {
        _enemy._sectionLocalPos = _enemy._transform.GetPos();
    }
}
