#include "action_sequencer.h"

#include <fstream>
#include <algorithm>

#include "entities/param_automator.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "string_util.h"
#include "imgui_util.h"

void ActionSequencerEntity::LoadDerived(serial::Ptree pt) {
    if (pt.TryGetString("seq_filename", &_seqFilename)) {
        std::ifstream inFile(_seqFilename);
        if (!inFile.is_open()) {
            printf("Action Sequencer %s could not find seq file %s\n", _name.c_str(), _seqFilename.c_str());
            return;
        }
        _actionsString << inFile.rdbuf();
    } else {
        printf("ActionSequencerEntity::LoadDerived: ERROR I DID NOT FIND seq_filename!\n");
    }
}

void ActionSequencerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("seq_filename", _seqFilename.c_str());
        
}

ne::Entity::ImGuiResult ActionSequencerEntity::ImGuiDerived(GameManager& g) {
    bool needsInit = imgui_util::InputText<256>("Filename", &_seqFilename, /*trueOnReturnOnly=*/true);
    if (needsInit) {
        return ImGuiResult::NeedsInit;
    }
    else {
        return ImGuiResult::Done;
    }
}

void ActionSequencerEntity::InitDerived(GameManager& g) {
    _currentIx = 0;
    _actions.clear();   

    SeqAction::LoadAndInitActions(g, _actionsString, _actions);

    // Set all beat times to be relative to when this sequencer got init.
    // TODO: maybe allow interpreting times as absolute instead of only relative to init
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double const startBeatTime = g._beatClock->GetNextDownBeatTime(beatTime);
    for (BeatTimeAction& bta : _actions) {
        if (bta._beatTime >= 0) {
            bta._beatTime += startBeatTime;
        }
    }

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
