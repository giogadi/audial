#include "action_sequencer.h"

#include <fstream>
#include <sstream>

#include "entities/param_automator.h"
#include "game_manager.h"

void SpawnAutomatorSeqAction::Load(std::istream& input) {
    std::string paramName;
    input >> paramName;
    if (paramName == "seq_velocity") {
        _synth = false;
        input >> _seqEntityName;
    } else {
        _synth = true;
        _synthParam = audio::StringToSynthParamType(paramName.c_str());
        input >> _channel;
    }
    input >> _startValue;
    input >> _endValue;
    input >> _desiredAutomateTime;
}

void SpawnAutomatorSeqAction::Execute(GameManager& g) {
    ParamAutomatorEntity* e = static_cast<ParamAutomatorEntity*>(g._neEntityManager->AddEntity(ne::EntityType::ParamAutomator));
    e->_startValue = _startValue;
    e->_endValue = _endValue;
    e->_desiredAutomateTime = _desiredAutomateTime;
    e->_synth = _synth;
    e->_synthParam = _synthParam;
    e->_channel = _channel;
    e->_seqEntityName = _seqEntityName;
    e->Init(g);
}

void RemoveEntitySeqAction::Load(std::istream& input) {
    input >> _entityName;
}

void RemoveEntitySeqAction::Execute(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_entityName);
    if (e) {
        g._neEntityManager->TagForDestroy(e->_id);
    } else {
        printf("RemoveEntitySeqAction could not find entity with name \"%s\"\n", _entityName.c_str());
    }
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
            } else if (actionType == "remove_entity") {
                pAction = std::make_unique<RemoveEntitySeqAction>();
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