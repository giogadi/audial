#include <synth_imgui.h>

#include <imgui.h>

void RequestSynthParamChange(
    int synthIx, audio::SynthParamType const& param,
    double value, audio::Context& audioContext) {
    audio::Event e;
    e.channel = synthIx;
    e.timeInTicks = 0;
    e.type = audio::EventType::SynthParam;
    e.param = param;
    e.newParamValue = value;
    e.paramChangeTime = 0;
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
                e.timeInTicks = 0;
                audioContext.AddEvent(e);
            }
        }

        // Saving
        char saveFilenameBuffer[256];
        strcpy(saveFilenameBuffer, synthGuiState._saveFilename.c_str());
        ImGui::InputText("Save filename", saveFilenameBuffer, 256);
        synthGuiState._saveFilename = saveFilenameBuffer;
        if (ImGui::Button("Save")) {
            serial::SaveToFile(synthGuiState._saveFilename.c_str(), "patches", synthGuiState._synthPatches);            
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            serial::LoadFromFile(synthGuiState._saveFilename.c_str(), synthGuiState._synthPatches);
            for (int synthIx = 0; synthIx < synthGuiState._synthPatches._patches.size(); ++synthIx) {
                for (int paramIx = 0; paramIx < (int) audio::SynthParamType::Count; ++paramIx) {
                    audio::SynthParamType param = (audio::SynthParamType) paramIx;
                    float v = synthGuiState._synthPatches._patches[synthIx].Get(param);
                    RequestSynthParamChange(synthIx, param, v, audioContext);
                }
            }
        }
    }
            
    // TODO: consider caching this.
    std::vector<char const*> listNames;
    listNames.reserve(synthGuiState._synthPatches._names.size());
    for (auto const& s : synthGuiState._synthPatches._names) {
        listNames.push_back(s.c_str());
    }
    ImGui::ListBox("Synth list", &synthGuiState._currentSynthIx, listNames.data(), /*numItems=*/listNames.size());

    if (synthGuiState._currentSynthIx >= 0) {
        std::string& patchName = synthGuiState._synthPatches._names[synthGuiState._currentSynthIx];
        synth::Patch& patch = synthGuiState._synthPatches._patches[synthGuiState._currentSynthIx];

        char nameBuffer[128];
        strcpy(nameBuffer, patchName.c_str());
        ImGui::InputText("Name", nameBuffer, 128);
        patchName = nameBuffer;

        audio::SynthParamType changedParam = patch.ImGui();
        if (changedParam != audio::SynthParamType::Count) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, changedParam, patch.Get(changedParam), audioContext);
        }
    }
    ImGui::End();
}
