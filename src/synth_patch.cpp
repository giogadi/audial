#include "synth_patch.h"

#include "imgui/imgui.h"

namespace synth {

namespace {

synth::Waveform FloatToWaveform(float f) {
    if (f < 0.5f) {
        return Waveform::Square;
    } else if (f < 1.5f) {
        return Waveform::Saw;
    } else {
        return Waveform::Noise;
    }
}

float WaveformToFloat(synth::Waveform w) {
    switch (w) {
        case Waveform::Square: return 0.f;
        case Waveform::Saw: return 1.f;
        case Waveform::Noise: return 2.f;
        default:
            assert(false);
    }
    return 0.f;
}

bool FloatToIsFm(float f) {
    return f >= 0.5f;
}

float IsFmToFloat(bool isFm) {
    if (isFm) {
        return 1.f;
    } else {
        return 0.f;
    }
}

bool IsFmParam(audio::SynthParamType const paramType) {
    switch (paramType) {
        case audio::SynthParamType::FM:
        case audio::SynthParamType::FMOsc2Level:
        case audio::SynthParamType::FMOsc2Ratio:
            return true;
        case audio::SynthParamType::Mono:        
        case audio::SynthParamType::Osc1Waveform:
        case audio::SynthParamType::Osc2Waveform:
        case audio::SynthParamType::Cutoff:
        case audio::SynthParamType::Peak:
        case audio::SynthParamType::HpfCutoff:
        case audio::SynthParamType::HpfPeak:
        case audio::SynthParamType::Portamento:
        case audio::SynthParamType::PitchLFOFreq:
        case audio::SynthParamType::CutoffLFOFreq:
        case audio::SynthParamType::AmpEnvAttack:
        case audio::SynthParamType::AmpEnvDecay:
        case audio::SynthParamType::AmpEnvRelease:
        case audio::SynthParamType::CutoffEnvGain:
        case audio::SynthParamType::CutoffEnvAttack:
        case audio::SynthParamType::CutoffEnvDecay:
        case audio::SynthParamType::Gain:
        case audio::SynthParamType::Detune:
        case audio::SynthParamType::OscFader:
        case audio::SynthParamType::PitchLFOGain:
        case audio::SynthParamType::CutoffLFOGain:
        case audio::SynthParamType::AmpEnvSustain:
        case audio::SynthParamType::CutoffEnvSustain:
        case audio::SynthParamType::CutoffEnvRelease:
        case audio::SynthParamType::PitchEnvGain:
        case audio::SynthParamType::PitchEnvAttack:
        case audio::SynthParamType::PitchEnvDecay:
        case audio::SynthParamType::PitchEnvSustain:
        case audio::SynthParamType::PitchEnvRelease:
        case audio::SynthParamType::DelayGain:
        case audio::SynthParamType::DelayTime:
        case audio::SynthParamType::DelayFeedback:
        case audio::SynthParamType::Count:
            return false;
    }
    return false;
}

}

void ADSREnvSpec::Save(serial::Ptree pt) const {
    pt.PutFloat("attack_time", attackTime);
    pt.PutFloat("decay_time", decayTime);
    pt.PutFloat("sustain_level", sustainLevel);
    pt.PutFloat("release_time", releaseTime);
    pt.PutFloat("min_value", minValue);
}
void ADSREnvSpec::Load(serial::Ptree pt) {
    attackTime = pt.GetFloat("attack_time");
    decayTime = pt.GetFloat("decay_time");
    sustainLevel = pt.GetFloat("sustain_level");
    releaseTime = pt.GetFloat("release_time");
    minValue = pt.GetFloat("min_value");
}

Waveform Patch::GetOsc1Waveform() const {
    float osc1AsFloat = Get(audio::SynthParamType::Osc1Waveform);
    return FloatToWaveform(osc1AsFloat);
}

Waveform Patch::GetOsc2Waveform() const {
    float osc2AsFloat = Get(audio::SynthParamType::Osc2Waveform);
    return FloatToWaveform(osc2AsFloat);
}

bool Patch::GetIsFm() const {
    float isFmAsFloat = Get(audio::SynthParamType::FM);
    return FloatToIsFm(isFmAsFloat);
}

ADSREnvSpec Patch::GetAmpEnvSpec() const {
    ADSREnvSpec spec;
    spec.attackTime = Get(audio::SynthParamType::AmpEnvAttack);
    spec.decayTime = Get(audio::SynthParamType::AmpEnvDecay);
    spec.sustainLevel = Get(audio::SynthParamType::AmpEnvSustain);
    spec.releaseTime = Get(audio::SynthParamType::AmpEnvRelease);
    return spec;
}

ADSREnvSpec Patch::GetCutoffEnvSpec() const {
    ADSREnvSpec spec;
    spec.attackTime = Get(audio::SynthParamType::CutoffEnvAttack);
    spec.decayTime = Get(audio::SynthParamType::CutoffEnvDecay);
    spec.sustainLevel = Get(audio::SynthParamType::CutoffEnvSustain);
    spec.releaseTime = Get(audio::SynthParamType::CutoffEnvRelease);
    return spec;
}

ADSREnvSpec Patch::GetPitchEnvSpec() const {
    ADSREnvSpec spec;
    spec.attackTime = Get(audio::SynthParamType::PitchEnvAttack);
    spec.decayTime = Get(audio::SynthParamType::PitchEnvDecay);
    spec.sustainLevel = Get(audio::SynthParamType::PitchEnvSustain);
    spec.releaseTime = Get(audio::SynthParamType::PitchEnvRelease);
    return spec;
}

void Patch::Save(serial::Ptree pt) const {
    for (int i = 0, n = (int) audio::SynthParamType::Count; i < n; ++i) {
        audio::SynthParamType paramType = (audio::SynthParamType) i;
        char const* paramName = SynthParamTypeToString(paramType);
        switch (paramType) {
            case audio::SynthParamType::Osc1Waveform: // fall through
            case audio::SynthParamType::Osc2Waveform: {
                Waveform w = FloatToWaveform(_data[i]);
                serial::PutEnum(pt, paramName, w);
                break;
            }
            default:
                pt.PutFloat(paramName, _data[i]);
                break;
        }
    }
}

void Patch::Load(serial::Ptree pt) {
    int version;
    if (pt.TryGetInt("version", &version)) {
        // old-style patch.
        Get(audio::SynthParamType::Gain) = pt.GetFloat("gain_factor");
        if (version >= 4) {
            Get(audio::SynthParamType::Mono) = pt.GetBool("mono") ? 1.f : 0.f;
        }
        if (version >= 1) {
            Waveform w = Waveform::Square;
            serial::TryGetEnum(pt, "osc1_waveform", w);
            Get(audio::SynthParamType::Osc1Waveform) = WaveformToFloat(w);

            serial::TryGetEnum(pt, "osc2_waveform", w);
            Get(audio::SynthParamType::Osc2Waveform) = WaveformToFloat(w);

            Get(audio::SynthParamType::Detune) = pt.GetFloat("detune");
            Get(audio::SynthParamType::OscFader) = pt.GetFloat("osc_fader");
        }
        Get(audio::SynthParamType::Cutoff) = pt.GetFloat("cutoff_freq");
        Get(audio::SynthParamType::Peak) = pt.GetFloat("cutoff_k");
        if (version >= 3) {
            Get(audio::SynthParamType::HpfCutoff) = pt.GetFloat("hpf_cutoff_freq");
            Get(audio::SynthParamType::HpfPeak) = pt.GetFloat("hpf_peak");
        }
        Get(audio::SynthParamType::PitchLFOGain) = pt.GetFloat("pitch_lfo_gain");
        Get(audio::SynthParamType::PitchLFOFreq) = pt.GetFloat("pitch_lfo_freq");
        Get(audio::SynthParamType::CutoffLFOGain) = pt.GetFloat("cutoff_lfo_gain");
        Get(audio::SynthParamType::CutoffLFOFreq) = pt.GetFloat("cutoff_lfo_freq");
        
        ADSREnvSpec envSpec;
        envSpec.Load(pt.GetChild("amp_env_spec"));
        Get(audio::SynthParamType::AmpEnvAttack) = envSpec.attackTime;
        Get(audio::SynthParamType::AmpEnvDecay) = envSpec.decayTime;
        Get(audio::SynthParamType::AmpEnvSustain) = envSpec.sustainLevel;
        Get(audio::SynthParamType::AmpEnvRelease) = envSpec.releaseTime;
        
        Get(audio::SynthParamType::CutoffEnvGain) = pt.GetFloat("cutoff_env_gain"); 
        envSpec.Load(pt.GetChild("cutoff_env_spec"));
        Get(audio::SynthParamType::CutoffEnvAttack) = envSpec.attackTime;
        Get(audio::SynthParamType::CutoffEnvDecay) = envSpec.decayTime;
        Get(audio::SynthParamType::CutoffEnvSustain) = envSpec.sustainLevel;
        Get(audio::SynthParamType::CutoffEnvRelease) = envSpec.releaseTime;
        if (version >= 2) {
            Get(audio::SynthParamType::PitchEnvGain) = pt.GetFloat("pitch_env_gain");
            envSpec.Load(pt.GetChild("pitch_env_spec"));
            Get(audio::SynthParamType::PitchEnvAttack) = envSpec.attackTime;
            Get(audio::SynthParamType::PitchEnvDecay) = envSpec.decayTime;
            Get(audio::SynthParamType::PitchEnvSustain) = envSpec.sustainLevel;
            Get(audio::SynthParamType::PitchEnvRelease) = envSpec.releaseTime;
        }
    } else {
        for (int i = 0, n = (int) audio::SynthParamType::Count; i < n; ++i) {
            _data[i] = 0.f;
            audio::SynthParamType paramType = (audio::SynthParamType) i;
            char const* paramName = SynthParamTypeToString(paramType);
            switch (paramType) {
                case audio::SynthParamType::Osc1Waveform: // fall through
                case audio::SynthParamType::Osc2Waveform: {
                    Waveform w = Waveform::Square;
                    if (!serial::TryGetEnum(pt, paramName, w)) {
                        printf("Patch::Load: unrecognized waveform\n");
                    }
                    float f = WaveformToFloat(w);
                    _data[i] = f; 
                    break;
                }
                default:
                    if (!pt.TryGetFloat(paramName, &_data[i])) {
                        if (IsFmParam(paramType) && !GetIsFm()) {
                            break;
                        }
                        printf("Note: patch had no param \"%s\"\n", paramName);
                    }
                    break;
            }
        }
    }  
}

namespace {
Patch gClipboardPatch;
}

Patch::ImGuiResult Patch::ImGui() {
    ImGuiResult result;
    
    if (ImGui::Button("Copy")) {
        gClipboardPatch = *this;
    }
    ImGui::SameLine();
    if (ImGui::Button("Paste")) {
        *this = gClipboardPatch;
        result._allChanged = true;
    }
    audio::SynthParamType changedParam = audio::SynthParamType::Count;
    for (int i = 0, n = (int) audio::SynthParamType::Count; i < n; ++i) {
        audio::SynthParamType paramType = (audio::SynthParamType) i;
        char const* paramName = SynthParamTypeToString(paramType);
        bool changed = false;
        switch (paramType) {
            case audio::SynthParamType::Mono: {
                bool isMono = _data[i] > 0.f;
                changed = ImGui::Checkbox(paramName, &isMono);
                if (changed) {
                    _data[i] = isMono ? 1.f : 0.f;
                }
                break;
            }
            case audio::SynthParamType::FM: {
                bool isFm = _data[i] > 0.f;
                changed = ImGui::Checkbox(paramName, &isFm);
                if (changed) {
                    _data[i] = isFm ? 1.f : 0.f;
                }
                break;
            }
            case audio::SynthParamType::Osc1Waveform: // fallthrough
            case audio::SynthParamType::Osc2Waveform: {
                Waveform w = FloatToWaveform(_data[i]);
                changed = WaveformImGui(paramName, &w);
                if (changed) {
                    _data[i] = WaveformToFloat(w);
                }
                break;
            }
            case audio::SynthParamType::Cutoff: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 24000.f);
                break;
            }
            case audio::SynthParamType::Peak: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 10.f);
                break;
            }
            case audio::SynthParamType::HpfCutoff: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 44100.f);
                break;
            }
            case audio::SynthParamType::HpfPeak: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 3.99f);
                break;
            }
            case audio::SynthParamType::Portamento: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 1.f);
                break;
            }
            case audio::SynthParamType::PitchLFOFreq: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 100.f);
                break;
            }
            case audio::SynthParamType::CutoffLFOFreq: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 100.f);
                break;
            }
            case audio::SynthParamType::AmpEnvAttack: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 3.f);
                break;
            }
            case audio::SynthParamType::AmpEnvDecay: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 5.f);
                break;
            }
            case audio::SynthParamType::AmpEnvRelease: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 5.f);
                break;
            }
            case audio::SynthParamType::CutoffEnvGain: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 44100.f);
                break;
            }
            case audio::SynthParamType::CutoffEnvAttack: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 3.f);
                break;
            }
            case audio::SynthParamType::CutoffEnvDecay: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 5.f);
                break;
            }
            case audio::SynthParamType::Gain: // fallthrough
            case audio::SynthParamType::Detune:
            case audio::SynthParamType::OscFader:
            case audio::SynthParamType::PitchLFOGain:
            case audio::SynthParamType::CutoffLFOGain:
            case audio::SynthParamType::AmpEnvSustain:
            case audio::SynthParamType::CutoffEnvSustain:
            case audio::SynthParamType::CutoffEnvRelease:
            case audio::SynthParamType::PitchEnvGain:
            case audio::SynthParamType::PitchEnvAttack:
            case audio::SynthParamType::PitchEnvDecay:
            case audio::SynthParamType::PitchEnvSustain:
            case audio::SynthParamType::PitchEnvRelease: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 1.f);
                break;
            }
            case audio::SynthParamType::FMOsc2Level: {
                changed = ImGui::InputFloat(paramName, &_data[i]);
                break;
            }
            case audio::SynthParamType::FMOsc2Ratio: {
                changed = ImGui::InputFloat(paramName, &_data[i]);
                break;
            }
            case audio::SynthParamType::DelayGain: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 1.f);
                break;
            }
            case audio::SynthParamType::DelayTime: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 0.5f);
                break;
            }
            case audio::SynthParamType::DelayFeedback: {
                changed = ImGui::SliderFloat(paramName, &_data[i], 0.f, 1.f);
                break;
            }
            case audio::SynthParamType::Count:
                assert(false);
                break;
        }
        if (changed) {
            changedParam = paramType;
        }
    }

    result._changedParam = changedParam;
    return result;
}

}
