#include <synth_imgui.h>

#include "imgui.h"

#include "imgui_util.h"
#include "math_util.h"

extern int gkScriptVersion;

void RequestSynthParamChange(
    int synthIx, audio::SynthParamType const& param,
    double value, audio::Context& audioContext) {
    audio::Event e;
    e.channel = synthIx;
    e.delaySecs = 0.0;
    e.type = audio::EventType::SynthParam;
    e.param = param;
    e.newParamValue = value;
    e.paramChangeTimeSecs = 0.0;
    audioContext.AddEvent(e);
}

void DrawSynthGuiAndUpdatePatch(SynthGuiState& synthGuiState, audio::Context& audioContext) {
    ImGui::Begin("Synth settings");
    {
        // kill sound
        if (ImGui::Button("All notes off")) {
            for (int i = 0; i < audioContext._state.synths.size(); ++i) {
                audio::Event e;
                e.type = audio::EventType::AllNotesOff;
                e.channel = i;
                e.delaySecs = 0;
                audioContext.AddEvent(e);
            }
        }

        // Saving
        char saveFilenameBuffer[256];
        strcpy(saveFilenameBuffer, synthGuiState._saveFilename.c_str());
        ImGui::InputText("Save filename", saveFilenameBuffer, 256);
        synthGuiState._saveFilename = saveFilenameBuffer;
        if (ImGui::Button("Save")) {
            serial::SaveToFile(gkScriptVersion, synthGuiState._saveFilename.c_str(), "patches", *synthGuiState._synthPatches);
            printf("Saved patches to \"%s\"\n", synthGuiState._saveFilename.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            serial::LoadFromFile(synthGuiState._saveFilename.c_str(), *synthGuiState._synthPatches);
            synthGuiState._currentPatchBankIx = 0;
            for (int synthIx = 0; synthIx < synthGuiState._synthPatches->_patches.size(); ++synthIx) {
                for (int paramIx = 0; paramIx < (int) audio::SynthParamType::Count; ++paramIx) {
                    audio::SynthParamType param = (audio::SynthParamType) paramIx;
                    float v = synthGuiState._synthPatches->_patches[synthIx].Get(param);
                    RequestSynthParamChange(synthIx, param, v, audioContext);
                }
            }
        }
    }

    bool synthSelectionChanged = false;
    if (ImGui::BeginListBox("Synths")) {
        char synthName[] = "X";
        for (int ii = 0; ii < audio::kNumSynths; ++ii) {
            synthName[0] = '0' + ii;
            bool selected = ii == synthGuiState._currentSynthIx;
            if (ImGui::Selectable(synthName, selected)) {
                synthGuiState._currentSynthIx = ii;
                synthSelectionChanged = true;
            }
        }
        ImGui::EndListBox();
    }

    if (synthGuiState._currentSynthIx < 0) {
        synthGuiState._currentSynthIx = 0;
        synthSelectionChanged = true;
    }

    if (synthSelectionChanged) {
        // Update patch param GUI to reflect newly selected synth.
        synthGuiState._currentPatch = audioContext._state.synths[synthGuiState._currentSynthIx].patch;
    }

    if (ImGui::Button("New patch")) {
        synthGuiState._synthPatches->_names.emplace_back("new_patch");
        synthGuiState._synthPatches->_patches.emplace_back();
        synthGuiState._currentPatchBankIx = synthGuiState._synthPatches->_names.size() - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete patch")) {
        if (synthGuiState._currentPatchBankIx >= 0 && synthGuiState._currentPatchBankIx < synthGuiState._synthPatches->_names.size()) {
            synthGuiState._synthPatches->_names.erase(synthGuiState._synthPatches->_names.begin() + synthGuiState._currentPatchBankIx);
            synthGuiState._synthPatches->_patches.erase(synthGuiState._synthPatches->_patches.begin() + synthGuiState._currentPatchBankIx);
            synthGuiState._currentPatchBankIx = math_util::Clamp(synthGuiState._currentPatchBankIx, 0, synthGuiState._synthPatches->_names.size() - 1);
        }        
    }
    if (ImGui::BeginCombo("Patch bank", synthGuiState._synthPatches->_names[synthGuiState._currentPatchBankIx].c_str())) {
        for (int ii = 0, n = synthGuiState._synthPatches->_names.size(); ii < n; ++ii) {
            std::string const& patchName = synthGuiState._synthPatches->_names[ii];
            bool selected = ii == synthGuiState._currentPatchBankIx;
            if (ImGui::Selectable(patchName.c_str(), selected)) {
                synthGuiState._currentPatchBankIx = ii;
            }
        }
        ImGui::EndCombo();
    }


    bool loadingNewPatch = false;
    if (ImGui::Button("Load from Patch")) {
        synthGuiState._currentPatch = synthGuiState._synthPatches->_patches[synthGuiState._currentPatchBankIx];
        loadingNewPatch = true;
    }
    if (ImGui::Button("Save to Patch")) {
        synthGuiState._synthPatches->_patches[synthGuiState._currentPatchBankIx] = synthGuiState._currentPatch;
    }

    {
        std::string& patchName = synthGuiState._synthPatches->_names[synthGuiState._currentPatchBankIx];
        imgui_util::InputText<32>("Patch name", &patchName);
    }    

    synth::Patch::ImGuiResult result = synthGuiState._currentPatch.ImGui();
    if (result._allChanged || loadingNewPatch) {
        for (int ii = 0; ii < (int)audio::SynthParamType::Count; ++ii) {
            auto type = (audio::SynthParamType)ii;
            RequestSynthParamChange(synthGuiState._currentSynthIx, (audio::SynthParamType)ii, synthGuiState._currentPatch.Get(type), audioContext);
        }
    }
    else if (result._changedParam != audio::SynthParamType::Count) {
        RequestSynthParamChange(synthGuiState._currentSynthIx, result._changedParam, synthGuiState._currentPatch.Get(result._changedParam), audioContext);
    }

    ImGui::End();
}
