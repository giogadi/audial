#include "action_sequencer.h"

#include <fstream>
#include <sstream>

#include "entities/param_automator.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "seq_actions/spawn_enemy.h"

void ActionSequencerEntity::Init(GameManager& g) {
    _currentIx = 0;
    _actions.clear();
    
    std::ifstream inFile(_seqFilename);
    if (!inFile.is_open()) {
        printf("Action Sequencer %s could not find seq file %s\n", _name.c_str(), _seqFilename.c_str());
        return;
    }
    SeqAction::LoadInputs loadInputs;
    loadInputs._beatTimeOffset = 0.0;
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
            lineStream >> loadInputs._beatTimeOffset;
            continue;
        }

        double beatTime = std::stod(token) + loadInputs._beatTimeOffset;

        lineStream >> token;

        std::unique_ptr<SeqAction> pAction;
        if (token == "automate") {
            pAction = std::make_unique<SpawnAutomatorSeqAction>();
        } else if (token == "remove_entity") {
            pAction = std::make_unique<RemoveEntitySeqAction>();
        } else if (token == "change_step") {
            pAction = std::make_unique<ChangeStepSequencerSeqAction>();
        } else if (token == "set_seq_save") {
            pAction = std::make_unique<SetStepSequencerSaveSeqAction>();
        } else if (token == "note_on_off") {
            pAction = std::make_unique<NoteOnOffSeqAction>();
        } else if (token == "e") {
            pAction = std::make_unique<SpawnEnemySeqAction>();
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
