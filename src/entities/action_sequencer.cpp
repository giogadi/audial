#include "action_sequencer.h"

#include <fstream>
#include <sstream>

#include "entities/param_automator.h"
#include "game_manager.h"

// void AutomateParamSeqAction::Load(std::istream& input) {
//     input >> _lengthInFrames;
//     input >> _initialValue;

//     _currentValue = _initialValue;
// }

// SeqAction::Result AutomateParamSeqAction::Execute(GameManager& g) {
//     _currentValue += 0.01;

//     audio::Event e;
//     e.type = audio::EventType::SynthParam;
//     e.param = audio::SynthParamType::Gain;
//     e.newParamValue = _currentValue;
//     g._audioContext->AddEvent(e);
    
//     ++_currentIx;
//     if (_currentIx >= _lengthInFrames) {
//         return SeqAction::Result::Done;
//     } else {
//         return SeqAction::Result::Continue;
//     }
// }

void SpawnAutomatorSeqAction::Load(std::istream& input) {
    input >> _seqEntityName;
    input >> _startVelocity;
    input >> _endVelocity;
    input >> _desiredAutomateTime;
}

void SpawnAutomatorSeqAction::Execute(GameManager& g) {
    ParamAutomatorEntity* e = static_cast<ParamAutomatorEntity*>(g._neEntityManager->AddEntity(ne::EntityType::ParamAutomator));
    e->_startVelocity = _startVelocity;
    e->_endVelocity = _endVelocity;
    e->_desiredAutomateTime = _desiredAutomateTime;
    e->_seqEntityName = _seqEntityName;
    e->Init(g);
}

void ActionSequencerEntity::Init(GameManager& g) {
    _currentIx = 0;
    _actions.clear();
    
    std::ifstream inFile(_seqFilename);
    if (!inFile.is_open()) {
        printf("Action Sequencer %s could not find seq file %s\n", _name.c_str(), _seqFilename.c_str());
        return;
    }
    std::string line;
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
        while (!lineStream.eof()) {
            std::string actionType;
            lineStream >> actionType;
            std::unique_ptr<SeqAction> pAction;
            if (actionType == "automate") {
                pAction = std::make_unique<SpawnAutomatorSeqAction>();
            } else {
                printf("ERROR: Unrecognized action type \"%s\".\n", actionType.c_str());
                break;
            }

            assert(pAction != nullptr);

            double beatTime;
            lineStream >> beatTime;

            pAction->Load(lineStream);

            _actions.emplace_back();
            BeatTimeAction& newBta = _actions.back();
            newBta._pAction = std::move(pAction);
            newBta._beatTime = beatTime;
        }
    }

    std::stable_sort(_actions.begin(), _actions.end(),
                     [](BeatTimeAction const& lhs, BeatTimeAction const& rhs) {
                         return lhs._beatTime < rhs._beatTime;
                     });
}

void ActionSequencerEntity::Update(GameManager& g, float dt) {
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
