#include "action_sequencer.h"

#include <fstream>
#include <algorithm>

#include "entities/param_automator.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "string_util.h"
#include "imgui_util.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"

void ActionSequencerEntity::LoadDerived(serial::Ptree pt) {
    _useFile = true;
    pt.TryGetBool("use_file", &_useFile);
    if (_useFile) {
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
    } else {
        serial::LoadVectorFromChildNode(pt, "actions", _actions);
    }
}

void ActionSequencerEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutBool("use_file", _useFile);
    if (_useFile) {
        pt.PutString("seq_filename", _seqFilename.c_str());
    } else {
        serial::SaveVectorInChildNode(pt, "actions", "action", _actions);
    }
        
}

ne::Entity::ImGuiResult ActionSequencerEntity::ImGuiDerived(GameManager& g) {
    ImGui::Checkbox("Use file", &_useFile);
    if (_useFile) {
        bool needsInit = imgui_util::InputText<256>("Filename", &_seqFilename, /*trueOnReturnOnly=*/true);
        if (needsInit) {
            return ImGuiResult::NeedsInit;
        }
        else {
            return ImGuiResult::Done;
        }
    } else {
        imgui_util::InputVectorOptions options;
        options.treePerItem = true;
        options.removeOnSameLine = false;
        bool changed = imgui_util::InputVector(_actions, options);
        return changed ? ImGuiResult::NeedsInit : ImGuiResult::Done;
    }
}

void ActionSequencerEntity::InitDerived(GameManager& g) {
    if (_useFile) {
        // rewind stringstream
        _actionsString.clear();
        _actionsString.seekg(0);
        
        _currentIx = 0;
        _actions.clear();   

        SeqAction::LoadAndInitActions(g, _actionsString, _actions);
    } else {
        for (BeatTimeAction& action : _actions) {
            action._pAction->Init(g);
        }
    }

    // Set all beat times to be relative to when this sequencer got init.
    // TODO: maybe allow interpreting times as absolute instead of only relative to init
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    _startTime = g._beatClock->GetNextDownBeatTime(beatTime);  

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
        if (beatTime < _startTime + bta._beatTime) {
            break;
        }
        bta._pAction->Execute(g);
    }
}
