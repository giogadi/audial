#include "change_patch.h"

#include "imgui/imgui.h"
#include "audio.h"
#include "imgui_util.h"
#include "synth_patch_bank.h"

extern GameManager gGameManager;

void ChangePatchSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    _p = Props();
    input >> _p._channelIx;
    input >> _p._blendBeatTime;
    input >> _p._patchBankName;
    _p._usePatchBank = true;
}

void ChangePatchSeqAction::LoadDerived(serial::Ptree pt) {
    _p = Props();
    _p._channelIx = pt.GetInt("channel");
    _p._blendBeatTime = pt.GetDouble("blend_beat_time");
    pt.TryGetBool("use_patch_bank", &_p._usePatchBank);
    if (_p._usePatchBank) {
        pt.TryGetString("patch_name", &_p._patchBankName);
    } else {
        serial::LoadFromChildOf(pt, "patch", _p._patch);
    }
}
void ChangePatchSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("channel", _p._channelIx);
    pt.PutDouble("blend_beat_time", _p._blendBeatTime);
    pt.PutBool("use_patch_bank", _p._usePatchBank);
    if (_p._usePatchBank) {
        pt.PutString("patch_name", _p._patchBankName.c_str());
    } else {
        serial::SaveInNewChildOf(pt, "patch", _p._patch);
    }    
}
bool ChangePatchSeqAction::ImGui() {
    ImGui::InputInt("Channel", &_p._channelIx);
    ImGui::InputDouble("Blend beat time", &_p._blendBeatTime);
    // TODO: Import/Apply need to get routed through synthGuiState so that it can update its current patch.
    /*if (ImGui::Button("Import current patch")) {
        auto const& synths = gGameManager._audioContext->_state.synths;
        if (_p._channelIx >= 0 && _p._channelIx < synths.size()) {
            synth::Patch const& currentPatch = synths[_p._channelIx].patch;
            _p._patch = currentPatch;
        }
    }    
    if (ImGui::Button("Apply current patch")) {
        audio::Event e;
        e.channel = _p._channelIx;
        e.timeInTicks = 0;
        e.type = audio::EventType::SynthParam;
        e.paramChangeTime = 0;
        for (int ii = 0; ii < (int)audio::SynthParamType::Count; ++ii) {
            auto type = (audio::SynthParamType) ii;
            e.param = type;
            e.newParamValue = _p._patch.Get(type);
            gGameManager._audioContext->AddEvent(e);
        }
    }*/
    ImGui::Checkbox("Use patch bank", &_p._usePatchBank);
    if (_p._usePatchBank) {
        ImGui::PushID("bank");
        if (ImGui::BeginCombo("Patch name", _p._patchBankName.c_str())) {
            for (int patchIx = 0, n = gGameManager._synthPatchBank->_names.size(); patchIx < n; ++patchIx) {
                ImGui::PushID(patchIx);
                std::string const& patchName = gGameManager._synthPatchBank->_names[patchIx];
                bool selected = patchName == _p._patchBankName;
                if (ImGui::Selectable(patchName.c_str(), selected)) {
                    _p._patchBankName = patchName;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    } else {
        _p._patch.ImGui();
    }
    
    return false;
}
void ChangePatchSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }    
    synth::Patch const* patchToUse = nullptr;
    if (_p._usePatchBank) {
        patchToUse = g._synthPatchBank->GetPatch(_p._patchBankName);
        if (patchToUse == nullptr) {
            printf("ChangePatchSeqAction: Failed to find patch \"%s\" in patch bank!\n", _p._patchBankName.c_str());
            return;
        }
    }
    else {
        patchToUse = &_p._patch;
    }
    double blendTimeSecs = g._beatClock->BeatTimeToSecs(_p._blendBeatTime);
    audio::Event e;
    e.type = audio::EventType::SynthParam;
    e.channel = _p._channelIx;
    e.paramChangeTimeSecs = blendTimeSecs;
    for (int paramIx = 0, n = (int)audio::SynthParamType::Count; paramIx < n; ++paramIx) {
        audio::SynthParamType paramType = (audio::SynthParamType) paramIx;
        float v = patchToUse->Get(paramType);
        e.param = paramType;
        e.newParamValue = v;
        g._audioContext->AddEvent(e);
    }
}
