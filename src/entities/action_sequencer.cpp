#include "action_sequencer.h"

#include <fstream>
#include <algorithm>

#include "entities/param_automator.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "string_util.h"
#include "imgui_util.h"

void ActionSequencerEntity::InitDerived(GameManager& g) {
    _currentIx = 0;
    _actions.clear();
    
    std::ifstream inFile(_seqFilename);
    if (!inFile.is_open()) {
        printf("Action Sequencer %s could not find seq file %s\n", _name.c_str(), _seqFilename.c_str());
        return;
    }

    SeqAction::LoadAndInitActions(g, inFile, _actions);

    // Immediately execute any actions that are < 0
    for (int n = _actions.size(); _currentIx < n; ++_currentIx) {
        BeatTimeAction const& bta = _actions[_currentIx];
        if (bta._beatTime < 0) {
            bta._pAction->ExecuteBase(g);
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
        bta._pAction->ExecuteBase(g);
    }
}

void ActionSequencerEntity::LoadDerived(serial::Ptree pt) {
    _seqFilename = pt.GetString("seq_filename");
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
