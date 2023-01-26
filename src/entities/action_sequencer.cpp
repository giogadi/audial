#include "action_sequencer.h"

#include <fstream>
#include <sstream>
#include <algorithm>

#include "entities/param_automator.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "seq_actions/spawn_enemy.h"
#include "entities/typing_player.h"
#include "entities/flow_player.h"
#include "string_util.h"

void ActionSequencerEntity::Init(GameManager& g) {
    _currentIx = 0;
    _actions.clear();
    
    std::ifstream inFile(_seqFilename);
    if (!inFile.is_open()) {
        printf("Action Sequencer %s could not find seq file %s\n", _name.c_str(), _seqFilename.c_str());
        return;
    }
    double startBeatTime = 0.0;
    SeqAction::LoadInputs loadInputs;
    loadInputs._beatTimeOffset = 0.0;
    std::string line;
    std::string token;
    int nextSectionId = 0;    
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
            lineStream >> loadInputs._beatTimeOffset;
            continue;
        } else if (token == "start") {
            lineStream >> startBeatTime;
            continue;
        } else if (token == "start_save") {
            loadInputs._defaultEnemiesSave = true;
            continue;
        } else if (token == "stop_save") {
            loadInputs._defaultEnemiesSave = false;
            continue;
        } else if (token == "section") {
            // lineStream >> loadInputs._sectionId;
            loadInputs._sectionId = nextSectionId++;
            int sectionNumBeats;
            lineStream >> sectionNumBeats;
            int numEntities = 0;
            ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::TypingPlayer, &numEntities);
            assert(numEntities == 1);
            TypingPlayerEntity* player = static_cast<TypingPlayerEntity*>(eIter.GetEntity());
            assert(player != nullptr);
            player->SetSectionLength(loadInputs._sectionId, sectionNumBeats);
            continue;
        } else if (token == "flow_section") {
            loadInputs._sectionId = nextSectionId++;
            std::string sectionDirStr;
            lineStream >> sectionDirStr;
            string_util::ToLower(sectionDirStr);
            int numEntities = 0;
            ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::FlowPlayer, &numEntities);
            assert(numEntities == 1);
            FlowPlayerEntity* player = static_cast<FlowPlayerEntity*>(eIter.GetEntity());
            assert(player != nullptr);
            FlowPlayerEntity::SectionDir sectionDir = FlowPlayerEntity::SectionDir::Up;
            if (sectionDirStr == "center") {
                sectionDir = FlowPlayerEntity::SectionDir::Center;
            } else if (sectionDirStr == "up") {
                sectionDir = FlowPlayerEntity::SectionDir::Up;
            } else if (sectionDirStr == "down") {
                sectionDir = FlowPlayerEntity::SectionDir::Down;
            } else if (sectionDirStr == "left") {
                sectionDir = FlowPlayerEntity::SectionDir::Left;
            } else if (sectionDirStr == "right") {
                sectionDir = FlowPlayerEntity::SectionDir::Right;
            } else {
                printf("ERROR: unrecognized flow_section direction \"%s\"\n", sectionDirStr.c_str());
            }
            player->_sectionDirs[loadInputs._sectionId] = sectionDir;
            continue;
        }

        double inputTime = std::stod(token);
        double beatTime = -1.0;
        if (inputTime < 0) {
            beatTime = inputTime;
        } else {
            double beatTimeAbs = inputTime + loadInputs._beatTimeOffset;
            if (beatTimeAbs < startBeatTime) {
                continue;
            }
            beatTime = beatTimeAbs - startBeatTime;
        }       

        lineStream >> token;

        // TODO: get these from the SeqActionType enum
        std::unique_ptr<SeqAction> pAction;
        if (token == "automate") {
            pAction = std::make_unique<SpawnAutomatorSeqAction>();
        } else if (token == "remove_entity") {
            pAction = std::make_unique<RemoveEntitySeqAction>();
        } else if (token == "change_step") {
            pAction = std::make_unique<ChangeStepSequencerSeqAction>();
        } else if (token == "set_all_steps") {
            pAction = std::make_unique<SetAllStepsSeqAction>();
        } else if (token == "set_step_seq") {
            pAction = std::make_unique<SetStepSequenceSeqAction>();
        } else if (token == "note_on_off") {
            pAction = std::make_unique<NoteOnOffSeqAction>();
        } else if (token == "e") {
            pAction = std::make_unique<SpawnEnemySeqAction>();
        } else if (token == "b_e") {
            pAction = std::make_unique<BeatTimeEventSeqAction>();
        } else {
            printf("ERROR: Unrecognized action type \"%s\".\n", token.c_str());
            continue;
        }

        assert(pAction != nullptr);

        pAction->Load(g, loadInputs, lineStream);

        _actions.emplace_back();
        BeatTimeAction& newBta = _actions.back();
        newBta._pAction = std::move(pAction);
        newBta._beatTime = beatTime;       
    }

    std::stable_sort(_actions.begin(), _actions.end(),
                     [](BeatTimeAction const& lhs, BeatTimeAction const& rhs) {
                         return lhs._beatTime < rhs._beatTime;
                     });

    // Immediately execute any actions that are < 0
    for (int n = _actions.size(); _currentIx < n; ++_currentIx) {
        BeatTimeAction const& bta = _actions[_currentIx];
        if (bta._beatTime < 0) {
            bta._pAction->Execute(g);
        } else {
            break;
        }
    }
}

void ActionSequencerEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    }
    if (_actions.empty()) {
        return;
    }

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    for (int n = _actions.size(); _currentIx < n; ++_currentIx) {
        BeatTimeAction const& bta = _actions[_currentIx];
        if (beatTime < bta._beatTime) {
            break;
        }
        bta._pAction->Execute(g);
    }
}

void ActionSequencerEntity::LoadDerived(serial::Ptree pt) {
    _seqFilename = pt.GetString("seq_filename");
}

void ActionSequencerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("seq_filename", _seqFilename.c_str());
}
