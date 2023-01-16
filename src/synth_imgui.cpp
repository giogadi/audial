#include <synth_imgui.h>

#include <imgui.h>
#include <audio_loader.h>

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

void RequestSynthParamChangeInt(int synthIx, audio::SynthParamType const& param,
				int value, audio::Context& audioContext) {
    audio::Event e;
    e.channel = synthIx;
    e.timeInTicks = 0;
    e.type = audio::EventType::SynthParam;
    e.param = param;
    e.newParamValueInt = value;
    audioContext.AddEvent(e);
}

void InitSynthGuiState(audio::Context const& audioContext, char const* saveFilename,
		       SynthGuiState& guiState) {
    guiState._synthPatches.clear();
    for (synth::StateData const& synthState : audioContext._state.synths) {
        guiState._synthPatches.push_back(synthState.patch);
    }
    assert(!guiState._synthPatches.empty());
    guiState._currentSynthIx = 0;
    guiState._saveFilename = std::string(saveFilename);
};
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
        strncpy_s(saveFilenameBuffer, synthGuiState._saveFilename.c_str(), synthGuiState._saveFilename.length());
        ImGui::InputText("Save filename", saveFilenameBuffer, 256);
        synthGuiState._saveFilename = saveFilenameBuffer;
        if (ImGui::Button("Save")) {
            if (SaveSynthPatches(synthGuiState._saveFilename.c_str(), synthGuiState._synthPatches)) {
                std::cout << "Saved synth patches to \"" << synthGuiState._saveFilename << "\"." << std::endl;
            }
        }
    }

    // TODO: consider caching this.
    std::vector<char const*> listNames;
    listNames.reserve(synthGuiState._synthPatches.size());
    for (auto const& s : synthGuiState._synthPatches) {
        listNames.push_back(s.name.c_str());
    }
    ImGui::ListBox("Synth list", &synthGuiState._currentSynthIx, listNames.data(), /*numItems=*/listNames.size());

    if (synthGuiState._currentSynthIx >= 0) {
        synth::Patch& patch = synthGuiState._synthPatches[synthGuiState._currentSynthIx];

        char nameBuffer[128];
        strncpy_s(nameBuffer, patch.name.c_str(), patch.name.length());
        ImGui::InputText("Name", nameBuffer, 128);
        patch.name = nameBuffer;

        bool changed = ImGui::SliderFloat("Gain", &patch.gainFactor, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Gain, patch.gainFactor, audioContext);
        }

        int const numWaveforms = static_cast<int>(synth::Waveform::Count);
        {
            int currentWaveIx = static_cast<int>(patch.osc1Waveform);
            changed = ImGui::Combo("Osc1 Wave", &currentWaveIx, synth::gWaveformStrings, numWaveforms);
            patch.osc1Waveform = static_cast<synth::Waveform>(currentWaveIx);
            if (changed) {
                RequestSynthParamChangeInt(synthGuiState._currentSynthIx, audio::SynthParamType::Osc1Waveform, currentWaveIx, audioContext);
            }
        }

        {
            int currentWaveIx = static_cast<int>(patch.osc2Waveform);
            changed = ImGui::Combo("Osc2 Wave", &currentWaveIx, synth::gWaveformStrings, numWaveforms);
            patch.osc2Waveform = static_cast<synth::Waveform>(currentWaveIx);
            if (changed) {
                RequestSynthParamChangeInt(synthGuiState._currentSynthIx, audio::SynthParamType::Osc2Waveform, currentWaveIx, audioContext);
            }
        }

        changed = ImGui::SliderFloat("Detune", &patch.detune, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Detune, patch.detune, audioContext);
        }

        changed = ImGui::SliderFloat("OscFader", &patch.oscFader, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::OscFader, patch.oscFader, audioContext);
        }

        changed = ImGui::SliderFloat("LPF Cutoff", &patch.cutoffFreq, 0.f, 44100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Cutoff, patch.cutoffFreq, audioContext);
        }

        changed = ImGui::SliderFloat("Peak", &patch.cutoffK, 0.f, 3.99f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Peak, patch.cutoffK, audioContext);
        }

        changed = ImGui::SliderFloat("HPF Cutoff", &patch.hpfCutoffFreq, 0.f, 44100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::HpfCutoff, patch.hpfCutoffFreq, audioContext);
        }

        changed = ImGui::SliderFloat("HPF Peak", &patch.hpfPeak, 0.f, 3.99f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::HpfPeak, patch.hpfPeak, audioContext);
        }

        changed = ImGui::SliderFloat("PitchLFOGain", &patch.pitchLFOGain, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchLFOGain, patch.pitchLFOGain, audioContext);
        }

        changed = ImGui::SliderFloat("PitchLFOFreq", &patch.pitchLFOFreq, 0.f, 100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchLFOFreq, patch.pitchLFOFreq, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffLFOGain", &patch.cutoffLFOGain, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffLFOGain, patch.cutoffLFOGain, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffLFOFreq", &patch.cutoffLFOFreq, 0.f, 100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffLFOFreq, patch.cutoffLFOFreq, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvAtk", &patch.ampEnvSpec.attackTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvAttack, patch.ampEnvSpec.attackTime, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvDecay", &patch.ampEnvSpec.decayTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvDecay, patch.ampEnvSpec.decayTime, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvSustain", &patch.ampEnvSpec.sustainLevel, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvSustain, patch.ampEnvSpec.sustainLevel, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvRelease", &patch.ampEnvSpec.releaseTime, 0.f, 5.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvRelease, patch.ampEnvSpec.releaseTime, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvGain", &patch.cutoffEnvGain, 0.f, 44100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvGain, patch.cutoffEnvGain, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvAtk", &patch.cutoffEnvSpec.attackTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvAttack, patch.cutoffEnvSpec.attackTime, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvDecay", &patch.cutoffEnvSpec.decayTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvDecay, patch.cutoffEnvSpec.decayTime, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvSustain", &patch.cutoffEnvSpec.sustainLevel, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvSustain, patch.cutoffEnvSpec.sustainLevel, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvRelease", &patch.cutoffEnvSpec.releaseTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvRelease, patch.cutoffEnvSpec.releaseTime, audioContext);
        }

	changed = ImGui::SliderFloat("PitchEnvGain", &patch.pitchEnvGain, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchEnvGain, patch.pitchEnvGain, audioContext);
        }

        changed = ImGui::SliderFloat("PitchEnvAtk", &patch.pitchEnvSpec.attackTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchEnvAttack, patch.pitchEnvSpec.attackTime, audioContext);
        }

        changed = ImGui::SliderFloat("PitchEnvDecay", &patch.pitchEnvSpec.decayTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchEnvDecay, patch.pitchEnvSpec.decayTime, audioContext);
        }

        changed = ImGui::SliderFloat("PitchEnvSustain", &patch.pitchEnvSpec.sustainLevel, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchEnvSustain, patch.pitchEnvSpec.sustainLevel, audioContext);
        }

        changed = ImGui::SliderFloat("PitchEnvRelease", &patch.pitchEnvSpec.releaseTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchEnvRelease, patch.pitchEnvSpec.releaseTime, audioContext);
        }
    }
    ImGui::End();
}
