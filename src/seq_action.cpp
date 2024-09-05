#include "seq_action.h"

#include <sstream>

#include "imgui/imgui.h"

#include "audio.h"
#include "actions.h"
#include "seq_actions/camera_control.h"
#include "seq_actions/change_patch.h"
#include "seq_actions/vfx_pulse.h"
#include "seq_actions/set_enemy_hittable.h"
#include "seq_actions/add_motion.h"
#include "imgui_util.h"
#include "serial_enum.h"

void SeqAction::Save(serial::Ptree pt) const {
    pt.PutBool("one_time", _oneTime);
    pt.PutString("action_type", SeqActionTypeToString(Type()));
    SaveDerived(pt);
}

void SeqAction::SaveActionsInChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>> const& actions) {
    serial::Ptree vecPt = pt.AddChild(childName);
    for (auto const& action : actions) {
        serial::Ptree childPt = vecPt.AddChild("seq_action");
        action->Save(childPt);
    }
}

std::unique_ptr<SeqAction> SeqAction::New(SeqActionType actionType) {
    switch (actionType) {
        case SeqActionType::SpawnAutomator: return std::make_unique<SpawnAutomatorSeqAction>();
        case SeqActionType::RemoveEntity: return std::make_unique<RemoveEntitySeqAction>();
        case SeqActionType::ChangeStepSequencer: return std::make_unique<ChangeStepSequencerSeqAction>();
        case SeqActionType::SetAllSteps: return std::make_unique<SetAllStepsSeqAction>();
        case SeqActionType::SetStepSequence: return std::make_unique<SetStepSequenceSeqAction>();
        case SeqActionType::SetStepSequencerMute: return std::make_unique<SetStepSequencerMuteSeqAction>();
        case SeqActionType::NoteOnOff: return std::make_unique<NoteOnOffSeqAction>();
        case SeqActionType::BeatTimeEvent: return std::make_unique<BeatTimeEventSeqAction>();
        case SeqActionType::WaypointControl: return std::make_unique<WaypointControlSeqAction>();
        case SeqActionType::PlayerSetKillZone: return std::make_unique<PlayerSetKillZoneSeqAction>();
        case SeqActionType::PlayerSetSpawnPoint: return std::make_unique<PlayerSetSpawnPointSeqAction>();
        case SeqActionType::SetNewFlowSection: return std::make_unique<SetNewFlowSectionSeqAction>();
        case SeqActionType::AddToIntVariable: return std::make_unique<AddToIntVariableSeqAction>();
        case SeqActionType::CameraControl: return std::make_unique<CameraControlSeqAction>();
        case SeqActionType::SetEntityActive: return std::make_unique<SetEntityActiveSeqAction>();
        case SeqActionType::ChangeStepSeqMaxVoices: return std::make_unique<ChangeStepSeqMaxVoicesSeqAction>();
        case SeqActionType::ChangePatch: return std::make_unique<ChangePatchSeqAction>();
        case SeqActionType::VfxPulse: return std::make_unique<VfxPulseSeqAction>();
        case SeqActionType::Trigger: return std::make_unique<TriggerSeqAction>();
        case SeqActionType::SetEnemyHittable: return std::make_unique<SetEnemyHittableSeqAction>();
        case SeqActionType::Respawn: return std::make_unique<RespawnSeqAction>();
        case SeqActionType::AddMotion: return std::make_unique<AddMotionSeqAction>();
        case SeqActionType::Count: break;
    }
    assert(false);
    return nullptr;
}

std::unique_ptr<SeqAction> SeqAction::Load(serial::Ptree pt) {
    SeqActionType actionType;
    bool found = TryGetEnum(pt, "action_type", actionType);
    if (!found) {
        printf("Could not find action_type!\n");
        return nullptr;
    }
    std::unique_ptr<SeqAction> pAction = SeqAction::New(actionType);
    pt.TryGetBool("one_time", &(pAction->_oneTime));
    pAction->LoadDerived(pt);
    return pAction;
}

bool SeqAction::LoadActionsFromChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>>& actions) {
    actions.clear();
    serial::Ptree actionsPt = pt.TryGetChild(childName);
    if (!actionsPt.IsValid()) {
        return false;
    }
    int numChildren;
    serial::NameTreePair* children = actionsPt.GetChildren(&numChildren);
    actions.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        actions.push_back(Load(children[i]._pt));
    }
    delete[] children;
    return true;
}

namespace {
    std::vector<std::unique_ptr<SeqAction>> sClipboardActions;
}

bool SeqAction::ImGui(char const* label, std::vector<std::unique_ptr<SeqAction>>& actions) {
    bool changed = false;
    if (ImGui::TreeNode(label)) {
        if (ImGui::Button("Clear Actions")) {
            actions.clear();
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy actions")) {
            sClipboardActions.clear();
            sClipboardActions.reserve(actions.size());
            for (auto const& pAction : actions) {
                auto clone = SeqAction::Clone(*pAction);
                sClipboardActions.push_back(std::move(clone));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Paste actions")) {
            actions.reserve(sClipboardActions.size());
            for (auto const& pAction : sClipboardActions) {
                auto clone = SeqAction::Clone(*pAction);
                actions.push_back(std::move(clone));
            }
            changed = true;
        }
        
        static SeqActionType actionType = SeqActionType::SpawnAutomator;
        SeqActionTypeImGui("Action type", &actionType);
        if (ImGui::Button("Add")) {
            actions.push_back(SeqAction::New(actionType));
            changed = true;
        }        
        int deleteIx = -1;
        std::pair<int, int> moveFromToIndices = std::make_pair(-1, -1);
        char actionId[8];
        for (int i = 0, n = actions.size(); i < n; ++i) {
            sprintf(actionId, "%d", i);
            bool selected = ImGui::TreeNode(actionId, "%s", SeqActionTypeToString(actions[i]->Type()));
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("SeqActionDND", &i, sizeof(int));
                ImGui::Text("Move action to");
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("SeqActionDND")) {
                    assert(payload->DataSize == sizeof(int));
                    int payloadIx = *(static_cast<int const*>(payload->Data));
                    moveFromToIndices = std::make_pair(payloadIx, i);
                }
                ImGui::EndDragDropTarget();
            }

            if (selected) {
                bool thisChanged = ImGui::Checkbox("One time", &actions[i]->_oneTime);
                changed = thisChanged || changed;

                thisChanged = actions[i]->ImGui();
                changed = thisChanged || changed;
                if (ImGui::Button("Remove")) {
                    deleteIx = i;
                    changed = true;
                }
                ImGui::TreePop();
            }
        }
        if (moveFromToIndices.first >= 0 && moveFromToIndices.first != moveFromToIndices.second) {
            std::unique_ptr<SeqAction> movedAction = std::move(actions[moveFromToIndices.first]);
            actions.erase(actions.begin() + moveFromToIndices.first);
            actions.insert(actions.begin() + moveFromToIndices.second, std::move(movedAction));
        } else if (deleteIx >= 0) {
            actions.erase(actions.begin() + deleteIx);
        }
        
        ImGui::TreePop();
    }
    
    return changed;
}

std::unique_ptr<SeqAction> SeqAction::LoadAction(LoadInputs const& loadInputs, std::istream& input) {
    std::string token;
    input >> token;

    bool oneTime = false;
    if (token == "onetime") {
        oneTime = true;
        input >> token;
    }

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
    } else if (token == "step_seq_mute") {
        pAction = std::make_unique<SetStepSequencerMuteSeqAction>();
    } else if (token == "note_on_off") {
        pAction = std::make_unique<NoteOnOffSeqAction>();
    } else if (token == "b_e") {
        pAction = std::make_unique<BeatTimeEventSeqAction>();
    } else if (token == "camera") {
        pAction = std::make_unique<CameraControlSeqAction>();
    } else if (token == "waypoint") {
        pAction = std::make_unique<WaypointControlSeqAction>();
    } else if (token == "player_killzone") {
        pAction = std::make_unique<PlayerSetKillZoneSeqAction>();
    } else if (token == "set_new_section") {
        pAction = std::make_unique<SetNewFlowSectionSeqAction>();
    } else if (token == "set_player_spawn") {
        pAction = std::make_unique<PlayerSetSpawnPointSeqAction>();
    } else if (token == "add_int") {
        pAction = std::make_unique<AddToIntVariableSeqAction>();
    } else if (token == "change_step_seq_max_voices") {
        pAction = std::make_unique<ChangeStepSeqMaxVoicesSeqAction>();
    } else if (token == "change_patch") {
        pAction = std::make_unique<ChangePatchSeqAction>();
    } else if (token == "set_enemy_hittable") {
        pAction = std::make_unique<SetEnemyHittableSeqAction>();
    } else if (token == "trigger") {
        pAction = std::make_unique<TriggerSeqAction>();
    } else if (token == "respawn") {
        pAction = std::make_unique<RespawnSeqAction>();
    } else {
        printf("ERROR: Unrecognized action type \"%s\".\n", token.c_str());
    }

    if (pAction == nullptr) {
        return nullptr;
    }

    pAction->_oneTime = oneTime;

    pAction->LoadDerived(loadInputs, input);

    return pAction;
}

void SeqAction::LoadAndInitActions(GameManager& g, std::istream& input, std::vector<BeatTimeAction>& actions) {
    double startBeatTime = 0.0;
    SeqAction::LoadInputs loadInputs;
    loadInputs._beatTimeOffset = 0.0;
    loadInputs._sampleRate = g._audioContext->_sampleRate;
    std::string line;
    std::string token;
    int nextSectionId = 0;    
    while (!input.eof()) {
        std::getline(input, line);
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

        std::unique_ptr<SeqAction> pAction = SeqAction::LoadAction(loadInputs, lineStream);

        pAction->Init(g);

        actions.emplace_back();
        BeatTimeAction& newBta = actions.back();
        newBta._pAction = std::move(pAction);
        newBta._beatTime = beatTime;
    }

    std::stable_sort(actions.begin(), actions.end(),
                     [](BeatTimeAction const& lhs, BeatTimeAction const& rhs) {
                         return lhs._beatTime < rhs._beatTime;
                     });
}

std::unique_ptr<SeqAction> SeqAction::Clone(SeqAction const& action) {
    // Slow and simple
    serial::Ptree pt = serial::Ptree::MakeNew();
    action.Save(pt);
    std::unique_ptr<SeqAction> copy = SeqAction::Load(pt);
    pt.DeleteData();
    return copy;

    // Faster, more maintenance
    /*switch (action.Type()) {
    case SeqActionType::SpawnAutomator: return std::make_unique<SpawnAutomatorSeqAction>(static_cast<SpawnAutomatorSeqAction const&>(action));
    case SeqActionType::RemoveEntity: return std::make_unique<RemoveEntitySeqAction>(static_cast<RemoveEntitySeqAction const&>(action));
    case SeqActionType::ChangeStepSequencer: return std::make_unique<ChangeStepSequencerSeqAction>(static_cast<ChangeStepSequencerSeqAction const&>(action));
    case SeqActionType::SetAllSteps: return std::make_unique<SetAllStepsSeqAction>(static_cast<SetAllStepsSeqAction const&>(action));
    case SeqActionType::SetStepSequence: return std::make_unique<SetStepSequenceSeqAction>(static_cast<SetStepSequenceSeqAction const&>(action));
    case SeqActionType::SetStepSequencerMute: return std::make_unique<SetStepSequencerMuteSeqAction>(static_cast<SetStepSequencerMuteSeqAction const&>(action));
    case SeqActionType::NoteOnOff: return std::make_unique<NoteOnOffSeqAction>(static_cast<NoteOnOffSeqAction const&>(action));
    case SeqActionType::BeatTimeEvent: return std::make_unique<BeatTimeEventSeqAction>(static_cast<BeatTimeEventSeqAction const&>(action));
    case SeqActionType::WaypointControl: return std::make_unique<WaypointControlSeqAction>(static_cast<WaypointControlSeqAction const&>(action));
    case SeqActionType::PlayerSetKillZone: return std::make_unique<PlayerSetKillZoneSeqAction>(static_cast<PlayerSetKillZoneSeqAction const&>(action));
    case SeqActionType::PlayerSetSpawnPoint: return std::make_unique<PlayerSetSpawnPointSeqAction>(static_cast<PlayerSetSpawnPointSeqAction const&>(action));
    case SeqActionType::SetNewFlowSection: return std::make_unique<SetNewFlowSectionSeqAction>(static_cast<SetNewFlowSectionSeqAction const&>(action));
    case SeqActionType::AddToIntVariable: return std::make_unique<AddToIntVariableSeqAction>(static_cast<AddToIntVariableSeqAction const&>(action));
    case SeqActionType::CameraControl: return std::make_unique<CameraControlSeqAction>(static_cast<CameraControlSeqAction const&>(action));
    case SeqActionType::SpawnEnemy: {
        SpawnEnemySeqAction const& src = static_cast<SpawnEnemySeqAction const&>(action);
        src;
        std::unique_ptr<SpawnEnemySeqAction> copy = std::make_unique<SpawnEnemySeqAction>();       
        copy->_enemy.Load();
        return std::make_unique<SpawnEnemySeqAction>(static_cast<SpawnEnemySeqAction const&>(action));
    }
    case SeqActionType::SetEntityActive: return std::make_unique<SetEntityActiveSeqAction>(static_cast<SetEntityActiveSeqAction const&>(action));
    case SeqActionType::ChangeStepSeqMaxVoices: return std::make_unique<ChangeStepSeqMaxVoicesSeqAction>(static_cast<ChangeStepSeqMaxVoicesSeqAction const&>(action));
    case SeqActionType::ChangePatch: return std::make_unique<ChangePatchSeqAction>(static_cast<ChangePatchSeqAction const&>(action));
    case SeqActionType::VfxPulse: return std::make_unique<VfxPulseSeqAction>(static_cast<VfxPulseSeqAction const&>(action));
    case SeqActionType::Trigger: return std::make_unique<TriggerSeqAction>(static_cast<TriggerSeqAction const&>(action));
    case SeqActionType::SetEnemyHittable: return std::make_unique<SetEnemyHittableSeqAction>(static_cast<SetEnemyHittableSeqAction const&>(action));
    case SeqActionType::Respawn: return std::make_unique<RespawnSeqAction>(static_cast<RespawnSeqAction const&>(action));
    case SeqActionType::Count: break;
    }
    assert(false);
    return nullptr;*/
}
