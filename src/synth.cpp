#include "synth.h"

#include <iostream>

#include "audio_util.h"
#include "constants.h"
#include "math_util.h"

namespace synth {
namespace {
float Polyblep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return t+t - t*t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t*t + t+t + 1.0f;
    }
    else {
        return 0.0f;
    }
}

float GenerateSquare(float const phase, float const phaseChange) {
    float v = 0.0f;
    if (phase < M_PI) {
        v = 1.0f;
    } else {
        v = -1.0f;
    }
    // polyblep
    float dt = phaseChange / (2*M_PI);
    float t = phase / (2*M_PI);
    v += Polyblep(t, dt);
    v -= Polyblep(fmod(t + 0.5f, 1.0f), dt);
    return v;
}

float GenerateSaw(float const phase, float const phaseChange) {
    float v = (phase / M_PI) - 1.0f;
    // polyblep
    float dt = phaseChange / (2*M_PI);
    float t = phase / (2*M_PI);
    v -= Polyblep(t, dt);
    return v;
}
}  // namespace

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

void Patch::Save(serial::Ptree pt) const {
    pt.PutInt("version", kVersion);
    pt.PutString("name", name.c_str());
    pt.PutFloat("gain_factor", gainFactor);
    pt.PutString("osc1_waveform", WaveformToString(osc1Waveform));
    pt.PutString("osc2_waveform", WaveformToString(osc2Waveform));
    pt.PutFloat("detune", detune);
    pt.PutFloat("osc_fader", oscFader);
    pt.PutFloat("cutoff_freq", cutoffFreq);
    pt.PutFloat("cutoff_k", cutoffK);
    pt.PutFloat("hpf_cutoff_freq", hpfCutoffFreq);
    pt.PutFloat("hpf_peak", hpfPeak);
    pt.PutFloat("pitch_lfo_gain", pitchLFOGain);
    pt.PutFloat("pitch_lfo_freq", pitchLFOFreq);
    pt.PutFloat("cutoff_lfo_gain", cutoffLFOGain);
    pt.PutFloat("cutoff_lfo_freq", cutoffLFOFreq);
    serial::SaveInNewChildOf(pt, "amp_env_spec", ampEnvSpec);
    pt.PutFloat("cutoff_env_gain", cutoffEnvGain);
    serial::SaveInNewChildOf(pt, "cutoff_env_spec", cutoffEnvSpec);
    pt.PutFloat("pitch_env_gain", pitchEnvGain);
    serial::SaveInNewChildOf(pt, "pitch_env_spec", pitchEnvSpec);
}
void Patch::Load(serial::Ptree pt) {
    int version = 0;
    pt.TryGetInt("version", &version);

    name = pt.GetString("name");
    gainFactor = pt.GetFloat("gain_factor");
    if (version >= 1) {
        osc1Waveform = StringToWaveform(pt.GetString("osc1_waveform").c_str());
        osc2Waveform = StringToWaveform(pt.GetString("osc2_waveform").c_str());
        detune = pt.GetFloat("detune");
        oscFader = pt.GetFloat("osc_fader");
    }
    cutoffFreq = pt.GetFloat("cutoff_freq");
    cutoffK = pt.GetFloat("cutoff_k");
    if (version >= 3) {
        hpfCutoffFreq = pt.GetFloat("hpf_cutoff_freq");
        hpfPeak = pt.GetFloat("hpf_peak");
    }
    pitchLFOGain = pt.GetFloat("pitch_lfo_gain");
    pitchLFOFreq = pt.GetFloat("pitch_lfo_freq");
    cutoffLFOGain = pt.GetFloat("cutoff_lfo_gain");
    cutoffLFOFreq = pt.GetFloat("cutoff_lfo_freq");
    ampEnvSpec.Load(pt.GetChild("amp_env_spec"));
    cutoffEnvGain = pt.GetFloat("cutoff_env_gain");
    cutoffEnvSpec.Load(pt.GetChild("cutoff_env_spec"));
    if (version >= 2) {
        pitchEnvGain = pt.GetFloat("pitch_env_gain");
        pitchEnvSpec.Load(pt.GetChild("pitch_env_spec"));
    }
}

void InitStateData(StateData& state, int channel) {
    state = StateData();
    state.channel = channel;
}

float calcMultiplier(float startLevel, float endLevel, long numSamples) {
    assert(numSamples > 0l);
    assert(startLevel > 0.f);
    double recip = 1.0 / numSamples;
    double endOverStart = endLevel / startLevel;
    double val = pow(endOverStart, recip);
    return (float) val;
}

struct ADSREnvSpecInTicks {
    long attackTime = 0l;
    long decayTime = 0l;
    float sustainLevel = 0.f;
    long releaseTime = 0l;
    float minValue = 0.01f;
};

void adsrEnvelope(ADSREnvSpecInTicks const& spec, ADSREnvState& state) {
    switch (state.phase) {
        case ADSRPhase::Closed:
            state.currentValue = 0.f;
            break;
        case ADSRPhase::Attack:
            // EXPONENTIAL ATTACK
            // if (state.ticksSincePhaseStart == 0) {
            //     if (spec.attackTimeInTicks == 0) {
            //         state.currentValue = 1.f;
            //         state.multiplier = 1.f;
            //     } else {
            //         state.currentValue = spec.minValue;
            //         state.multiplier = calcMultiplier(spec.minValue, 1.f, spec.attackTimeInTicks);
            //     }
            // }
            // state.currentValue *= state.multiplier;
            // ++state.ticksSincePhaseStart;
            // if (state.ticksSincePhaseStart >= spec.attackTimeInTicks) {
            //     state.phase = ADSRPhase::Decay;
            //     state.ticksSincePhaseStart = 0;
            // }
            // LINEAR ATTACK code
            if (state.ticksSincePhaseStart == 0) {
                state.currentValue = 0.f;
                if (spec.attackTime == 0) {
                    state.currentValue = 1.f;
                }
            } else {
                state.currentValue = (float)state.ticksSincePhaseStart / (float)spec.attackTime;
            }
            ++state.ticksSincePhaseStart;
            if (state.ticksSincePhaseStart >= spec.attackTime) {
                state.phase = ADSRPhase::Decay;
                state.ticksSincePhaseStart = 0;
            }
            break;
        case ADSRPhase::Decay:
            if (state.ticksSincePhaseStart == 0) {
                if (spec.decayTime == 0) {
                    state.currentValue = spec.sustainLevel;
                    state.multiplier = 1.f;
                } else {
                    state.currentValue = 1.f;
                    state.multiplier = calcMultiplier(1.f, spec.sustainLevel, spec.decayTime);
                }
            }
            state.currentValue *= state.multiplier;
            ++state.ticksSincePhaseStart;
            if (state.ticksSincePhaseStart >= spec.decayTime) {
                state.phase = ADSRPhase::Sustain;
                state.ticksSincePhaseStart = 0;
            }
            break;
        case ADSRPhase::Sustain:
            // Do nothing. waiting for external signal to enter Release.
            break;
        case ADSRPhase::Release: {
            if (state.ticksSincePhaseStart == 0) {
                if (spec.releaseTime == 0) {
                    state.multiplier = 0.f;
                } else {
                    // Keep current level. might have hit release before hitting
                    // sustain level
                    //
                    // TODO: calculate release from sustain to 0, or from
                    // current level to 0? I like sustain better.
                    state.multiplier = calcMultiplier(spec.sustainLevel, spec.minValue, spec.releaseTime);
                }
            }
            state.currentValue *= state.multiplier;
            ++state.ticksSincePhaseStart;
            if (state.ticksSincePhaseStart >= spec.releaseTime) {
                state.phase = ADSRPhase::Closed;
                state.ticksSincePhaseStart = -1;
                state.currentValue = 0.f;
            }
            break;
        }
    }
    // This is important right now because when we modify attack times, it
    // can cause the precomputed multiplier to be applied for longer than
    // expected and thus result in gains > 1.
    state.currentValue = std::min(1.f, state.currentValue);
}

// https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
//
// For now assumes peak is 0,4
enum class FilterType { LowPass, HighPass };
float UpdateFilter(float const dt, float const cutoff, float const peak, float const input, FilterType const filterType, FilterState& state) {
    float res = peak / 4.f;
    float g = tan(kPi * cutoff * dt);
    float k = 2 - 2*res;
    assert((1 + g*(g+k)) != 0.f);
    float a1 = 1 / (1 + g*(g+k));
    float a2 = g*a1;
    float a3 = g*a2;
	    
    float v3 = input - state.ic2eq;
    float v1 = a1*state.ic1eq + a2*v3;
    float v2 = state.ic2eq + a2*state.ic1eq + a3*v3;
    state.ic1eq = 2*v1 - state.ic1eq;
    state.ic2eq = 2*v2 - state.ic2eq;

    switch (filterType) {
        case FilterType::LowPass: return v2;
        case FilterType::HighPass: {
            float output = input - k*v1 - v2;
            return output;
        }
    }
}

// TODO: make this process an entire buffer.
// TODO: early exit if envelope is closed.
// TODO: remove redundant args now that we pass Patch in.
float ProcessVoice(
    Voice& voice, int const sampleRate, float pitchLFOValue,
	float modulatedCutoff, float cutoffK,
    ADSREnvSpecInTicks const& ampEnvSpec,
	ADSREnvSpecInTicks const& cutoffEnvSpec, float const cutoffEnvGain,
	ADSREnvSpecInTicks const& pitchEnvSpec, float const pitchEnvGain,
    Patch const& patch) {
    float const dt = 1.0f / sampleRate;

    // Now use the LFO value to get a new frequency.
    float modulatedF = voice.oscillators[0].f * powf(2.0f, pitchLFOValue);

	// Modulate by pitch envelope.
	adsrEnvelope(pitchEnvSpec, voice.pitchEnvState);
	modulatedF *= powf(2.f, pitchEnvGain * voice.pitchEnvState.currentValue);
	// TODO: clamp F?

    float v = 0.0f;
    for (int oscIx = 0; oscIx < voice.oscillators.size(); ++oscIx) {
        Oscillator& osc = voice.oscillators[oscIx];

	    

        if (osc.phase >= 2*kPi) {
            osc.phase -= 2*kPi;
        }

        float oscF = modulatedF;
        if (oscIx > 0) {
            oscF = modulatedF * powf(2.f, patch.detune);
        }

        float phaseChange = 2 * kPi * oscF / sampleRate;
        float oscGain;
        if (oscIx == 0) {
            oscGain = 1.f - patch.oscFader;
        } else {
            oscGain = patch.oscFader;
        }

        float oscV = 0.f;
        Waveform waveform = (oscIx == 0) ? patch.osc1Waveform : patch.osc2Waveform;
        switch (waveform) {
            case Waveform::Saw: {
                oscV = GenerateSaw(osc.phase, phaseChange);
                break;
            }
            case Waveform::Square: {
                oscV = GenerateSquare(osc.phase, phaseChange);
                break;
            }
            case Waveform::Count: {
                std::cout << "Invalid waveform" << std::endl;
                assert(false);
            }
        }

        float oscWithGain = oscV * oscGain;
        v += oscWithGain;
        osc.phase += phaseChange;
    }

    // Cutoff envelope
    adsrEnvelope(cutoffEnvSpec, voice.cutoffEnvState);
    modulatedCutoff += patch.cutoffEnvGain * voice.cutoffEnvState.currentValue;
	// The fancy filter below blows up for some reason if we don't include
	// this line.
	modulatedCutoff = math_util::Clamp(modulatedCutoff, 0.f, 22050.f);

    // ladder filter
#if 0
    float const rc = 1 / modulatedCutoff;
    float const a = dt / (rc + dt);
    v -= patch.cutoffK * voice.lp3;

    voice.lp0 = a*v + (1-a)*voice.lp0;
    voice.lp1 = a*(voice.lp0) + (1-a)*voice.lp1;
    voice.lp2 = a*(voice.lp1) + (1-a)*voice.lp2;
    voice.lp3 = a*(voice.lp2) + (1-a)*voice.lp3;
    v = voice.lp3;

#else	
    v = UpdateFilter(dt, modulatedCutoff, cutoffK, v, FilterType::LowPass, voice.lpfState);

    v = UpdateFilter(dt, patch.hpfCutoffFreq, patch.hpfPeak, v, FilterType::HighPass, voice.hpfState);
#endif
	
    // Amplitude envelope
    // TODO: move this up top so we can early-exit if the envelope is closed and save some computation.
    // TODO: consider only doing the envelope computation once per buffer frame.
    adsrEnvelope(ampEnvSpec, voice.ampEnvState);
    if (voice.ampEnvState.phase == ADSRPhase::Closed) {
        voice.currentMidiNote = -1;
    }
    v *= voice.ampEnvState.currentValue;


    // Apply velocity
    v *= voice.velocity;
        
    return v;
}

Voice* FindVoiceForNoteOn(StateData& state, int const midiNote) {
    // First, look for a voice already playing the note we want to play and
    // use that. If no such voice, look for any voices with closed
    // envelopes. If no such voice, look for voice that started closing the
    // longest time ago. If no such voice, then we don't play this note,
    // sorry bro
    int oldestClosingIx = -1;
    int oldestClosingAge = -1;
    for (int i = 0; i < state.voices.size(); ++i) {
        Voice& v = state.voices[i];
        if (v.currentMidiNote == midiNote) {
            return &v;
        }
        if (v.ampEnvState.phase == ADSRPhase::Closed) {
            oldestClosingIx = i;
            oldestClosingAge = std::numeric_limits<int>::max();
        } else if (v.ampEnvState.phase == ADSRPhase::Release && v.ampEnvState.ticksSincePhaseStart >= oldestClosingAge/*v.ampEnvTicksSinceNoteOff >= oldestClosingAge*/) {
            oldestClosingIx = i;
            oldestClosingAge = v.ampEnvState.ticksSincePhaseStart;
        }
    }
    if (oldestClosingIx >= 0) {
        return &state.voices[oldestClosingIx];
    }
    return nullptr;
}

Voice* FindVoiceForNoteOff(StateData& state, int midiNote) {
    // NOTE: we should be able to return as soon as we find a voice matching
    // the given note, but I'm doing the full iteration to check for
    // correctness for now.
    Voice* resultVoice = nullptr;
    for (Voice& v : state.voices) {
        if (v.currentMidiNote == midiNote) {
            assert(resultVoice == nullptr);
            resultVoice = &v;
        }
    }
    return resultVoice;
}

void ConvertADSREnvSpec(ADSREnvSpec const& spec, ADSREnvSpecInTicks& specInTicks, int sampleRate) {
    specInTicks.attackTime = (long)(spec.attackTime * sampleRate);
    specInTicks.decayTime = (long)(spec.decayTime * sampleRate);
    specInTicks.sustainLevel = spec.sustainLevel;
    specInTicks.releaseTime = (long)(spec.releaseTime * sampleRate);
    specInTicks.minValue = spec.minValue;
}

float& GetSynthParam(Patch& patch, audio::SynthParamType paramType) {
    switch (paramType) {
        case audio::SynthParamType::Gain:
            return patch.gainFactor;
            break;
        case audio::SynthParamType::Osc1Waveform:
            assert(false);
            // return static_cast<double>(static_cast<int>(patch.osc1Waveform));
            break;
        case audio::SynthParamType::Osc2Waveform:
            assert(false);
            // return static_cast<double>(static_cast<int>(patch.osc2Waveform));
            break;
        case audio::SynthParamType::Detune:
            return patch.detune;
            break;
        case audio::SynthParamType::OscFader:
            return patch.oscFader;
            break;
        case audio::SynthParamType::Cutoff:
            return patch.cutoffFreq;
            break;
        case audio::SynthParamType::Peak:
            return patch.cutoffK;
            break;
        case audio::SynthParamType::HpfCutoff:
            return patch.hpfCutoffFreq;
            break;
        case audio::SynthParamType::HpfPeak:
            return patch.hpfPeak;
            break;
        case audio::SynthParamType::PitchLFOGain:
            return patch.pitchLFOGain;
            break;
        case audio::SynthParamType::PitchLFOFreq:
            return patch.pitchLFOFreq;
            break;
        case audio::SynthParamType::CutoffLFOGain:
            return patch.cutoffLFOGain;
            break;
        case audio::SynthParamType::CutoffLFOFreq:
            return patch.cutoffLFOFreq;
            break;
        case audio::SynthParamType::AmpEnvAttack:
            return patch.ampEnvSpec.attackTime;
            break;
        case audio::SynthParamType::AmpEnvDecay:
            return patch.ampEnvSpec.decayTime;
            break;
        case audio::SynthParamType::AmpEnvSustain:
            return patch.ampEnvSpec.sustainLevel;
            break;
        case audio::SynthParamType::AmpEnvRelease:
            return patch.ampEnvSpec.releaseTime;
            break;
        case audio::SynthParamType::CutoffEnvGain:
            return patch.cutoffEnvGain;
            break;
        case audio::SynthParamType::CutoffEnvAttack:
            return patch.cutoffEnvSpec.attackTime;
            break;
        case audio::SynthParamType::CutoffEnvDecay:
            return patch.cutoffEnvSpec.decayTime;
            break;
        case audio::SynthParamType::CutoffEnvSustain:
            return patch.cutoffEnvSpec.sustainLevel;
            break;
        case audio::SynthParamType::CutoffEnvRelease:
            return patch.cutoffEnvSpec.releaseTime;
            break;
        case audio::SynthParamType::PitchEnvGain:
            return patch.pitchEnvGain;
            break;
        case audio::SynthParamType::PitchEnvAttack:
            return patch.pitchEnvSpec.attackTime;
            break;
        case audio::SynthParamType::PitchEnvDecay:
            return patch.pitchEnvSpec.decayTime;
            break;
        case audio::SynthParamType::PitchEnvSustain:
            return patch.pitchEnvSpec.sustainLevel;
            break;
        case audio::SynthParamType::PitchEnvRelease:
            return patch.pitchEnvSpec.releaseTime;
            break;
        default:
            std::cout << "Received unsupported synth param " << static_cast<int>(paramType) << std::endl;
            assert(false);
            break;
    }    
}

void SetSynthParam(audio::SynthParamType paramType, double newValue, int newValueInt, Patch& patch) {
    switch (paramType) {
        case audio::SynthParamType::Gain:
            patch.gainFactor = newValue;
            break;
        case audio::SynthParamType::Osc1Waveform:
            // int newValueInt = static_cast<int>(newValue);
            // patch.osc1Waveform = newValueInt;
            patch.osc1Waveform = static_cast<synth::Waveform>(newValueInt);
            break;
        case audio::SynthParamType::Osc2Waveform:
            // int newValueInt = static_cast<int>(newValue);
            // patch.osc2Waveform = newValueInt;
            patch.osc2Waveform = static_cast<synth::Waveform>(newValueInt);
            break;
        case audio::SynthParamType::Detune:
            patch.detune = newValue;
            break;
        case audio::SynthParamType::OscFader:
            patch.oscFader = newValue;
            break;
        case audio::SynthParamType::Cutoff:
            patch.cutoffFreq = newValue;
            break;
        case audio::SynthParamType::Peak:
            patch.cutoffK = newValue;
            break;
        case audio::SynthParamType::HpfCutoff:
            patch.hpfCutoffFreq = newValue;
            break;
        case audio::SynthParamType::HpfPeak:
            patch.hpfPeak = newValue;
            break;
        case audio::SynthParamType::PitchLFOGain:
            patch.pitchLFOGain = newValue;
            break;
        case audio::SynthParamType::PitchLFOFreq:
            patch.pitchLFOFreq = newValue;
            break;
        case audio::SynthParamType::CutoffLFOGain:
            patch.cutoffLFOGain = newValue;
            break;
        case audio::SynthParamType::CutoffLFOFreq:
            patch.cutoffLFOFreq = newValue;
            break;
        case audio::SynthParamType::AmpEnvAttack:
            patch.ampEnvSpec.attackTime = newValue;
            break;
        case audio::SynthParamType::AmpEnvDecay:
            patch.ampEnvSpec.decayTime = newValue;
            break;
        case audio::SynthParamType::AmpEnvSustain:
            patch.ampEnvSpec.sustainLevel = newValue;
            break;
        case audio::SynthParamType::AmpEnvRelease:
            patch.ampEnvSpec.releaseTime = newValue;
            break;
        case audio::SynthParamType::CutoffEnvGain:
            patch.cutoffEnvGain = newValue;
            break;
        case audio::SynthParamType::CutoffEnvAttack:
            patch.cutoffEnvSpec.attackTime = newValue;
            break;
        case audio::SynthParamType::CutoffEnvDecay:
            patch.cutoffEnvSpec.decayTime = newValue;
            break;
        case audio::SynthParamType::CutoffEnvSustain:
            patch.cutoffEnvSpec.sustainLevel = newValue;
            break;
        case audio::SynthParamType::CutoffEnvRelease:
            patch.cutoffEnvSpec.releaseTime = newValue;
            break;
        case audio::SynthParamType::PitchEnvGain:
            patch.pitchEnvGain = newValue;
            break;
        case audio::SynthParamType::PitchEnvAttack:
            patch.pitchEnvSpec.attackTime = newValue;
            break;
        case audio::SynthParamType::PitchEnvDecay:
            patch.pitchEnvSpec.decayTime = newValue;
            break;
        case audio::SynthParamType::PitchEnvSustain:
            patch.pitchEnvSpec.sustainLevel = newValue;
            break;
        case audio::SynthParamType::PitchEnvRelease:
            patch.pitchEnvSpec.releaseTime = newValue;
            break;
        default:
            std::cout << "Received unsupported synth param " << static_cast<int>(paramType) << std::endl;
            break;
    }
}
    

void Process(
    StateData* state, boost::circular_buffer<audio::Event> const& pendingEvents,
    float* outputBuffer, int const numChannels, int const samplesPerFrame,
    int const sampleRate, unsigned long frameStartTickTime) {

    Patch& patch = state->patch;

    int currentEventIx = 0;
    for(int i = 0; i < samplesPerFrame; ++i) {
        // Handle events
        while (true) {
            if (currentEventIx >= pendingEvents.size()) {
                break;
            }
            audio::Event const& e = pendingEvents[currentEventIx];
            if (e.timeInTicks > frameStartTickTime + i) {
                // This event gets handled later.
                break;
            }
            if (e.channel != state->channel) {
                // Not meant for this channel. Skip this message.
                ++currentEventIx;
                continue;
            }
            ++currentEventIx;
            switch (e.type) {
                case audio::EventType::NoteOn: {
                    Voice* v = FindVoiceForNoteOn(*state, e.midiNote);
                    if (v != nullptr) {
                        v->oscillators[0].f = synth::MidiToFreq(e.midiNote);
                        v->currentMidiNote = e.midiNote;
                        v->velocity = e.velocity;
                        if (v->ampEnvState.phase != synth::ADSRPhase::Closed) {
                            v->ampEnvState.ticksSincePhaseStart = (unsigned long) (v->ampEnvState.currentValue * patch.ampEnvSpec.attackTime * sampleRate);
                        } else {
                            v->ampEnvState.ticksSincePhaseStart = 0;
                        }
                        v->ampEnvState.phase = synth::ADSRPhase::Attack;
                        v->cutoffEnvState.phase = synth::ADSRPhase::Attack;
                        v->cutoffEnvState.ticksSincePhaseStart = v->ampEnvState.ticksSincePhaseStart;
                        v->pitchEnvState.phase = synth::ADSRPhase::Attack;
                        v->pitchEnvState.ticksSincePhaseStart = v->ampEnvState.ticksSincePhaseStart;
                    } else {
                        std::cout << "couldn't find a note for noteon" << std::endl;
                    }
                    break;
                }
                case audio::EventType::NoteOff: {
                    Voice* v = FindVoiceForNoteOff(*state, e.midiNote);
                    if (v != nullptr) {
                        v->ampEnvState.phase = synth::ADSRPhase::Release;
                        v->ampEnvState.ticksSincePhaseStart = 0;
                        v->cutoffEnvState.phase = synth::ADSRPhase::Release;
                        v->cutoffEnvState.ticksSincePhaseStart = 0;
                        v->pitchEnvState.phase = synth::ADSRPhase::Release;
                        v->pitchEnvState.ticksSincePhaseStart = 0;
                    }
                    break;
                }
                case audio::EventType::AllNotesOff: {
                    for (Voice& v : state->voices) {
                        if (v.currentMidiNote != -1) {
                            v.ampEnvState.phase = synth::ADSRPhase::Release;
                            v.ampEnvState.ticksSincePhaseStart = 0;
                            v.cutoffEnvState.phase = synth::ADSRPhase::Release;
                            v.cutoffEnvState.ticksSincePhaseStart = 0;
                            v.pitchEnvState.phase = synth::ADSRPhase::Release;
                            v.pitchEnvState.ticksSincePhaseStart = 0;
                        }
                    }
                    break;
                }
                case audio::EventType::SynthParam: {
                    if (e.paramChangeTime != 0) {
                        // Find a free automation
                        Automation* pA = nullptr;
                        for (Automation& a : state->automations) {
                            if (a._active) {
                                pA = &a;
                                break;
                            }
                        }
                        if (pA == nullptr) {
                            printf("Failed to find a free automation!\n");
                            break;
                        }
                        pA->_active = true;
                        pA->_synthParamType = e.param;
                        pA->_desiredValue = e.newParamValue;
                        pA->_desiredTickTime = e.paramChangeTime;
                        break;
                    }
                    SetSynthParam(e.param, e.newParamValue, e.newParamValueInt, patch);
                }
                default: {
                    break;
                }
            }
        }

        // Now apply automations!
        for (Automation& a : state->automations) {
            // use first order lag for now
            if (!a._active) {
                continue;
            }
            float& currentValue = GetSynthParam(patch, a._synthParamType);
            // TODO ACTUALLY USE _desiredTickTime!!!
            currentValue += 0.01 * (a._desiredValue - currentValue);
        }

        // Get pitch LFO value
        if (state->pitchLFOPhase >= 2*kPi) {
            state->pitchLFOPhase -= 2*kPi;
        }
        // Make LFO a sine wave for now.

        float const pitchLFOValue = patch.pitchLFOGain * sinf(state->pitchLFOPhase);
        state->pitchLFOPhase += (patch.pitchLFOFreq * 2*kPi / sampleRate);

        // Get cutoff LFO value
        if (state->cutoffLFOPhase >= 2*kPi) {
            state->cutoffLFOPhase -= 2*kPi;
        }
        // Make LFO a sine wave for now.
        float const cutoffLFOValue = patch.cutoffLFOGain * sinf(state->cutoffLFOPhase);
        state->cutoffLFOPhase += (patch.cutoffLFOFreq * 2*kPi / sampleRate);
        // float const modulatedCutoff = patch.cutoffFreq * powf(2.0f, cutoffLFOValue);
        float const modulatedCutoff = math_util::Clamp(patch.cutoffFreq + 20000*cutoffLFOValue, 0.f, 44100.f);


        ADSREnvSpecInTicks ampEnvSpec, cutoffEnvSpec, pitchEnvSpec;
        ConvertADSREnvSpec(patch.ampEnvSpec, ampEnvSpec, sampleRate);
        ConvertADSREnvSpec(patch.cutoffEnvSpec, cutoffEnvSpec, sampleRate);
        ConvertADSREnvSpec(patch.pitchEnvSpec, pitchEnvSpec, sampleRate);

        float v = 0.0f;
        for (Voice& voice : state->voices) {
            v += ProcessVoice(
                voice, sampleRate, pitchLFOValue, modulatedCutoff, patch.cutoffK,
                ampEnvSpec, cutoffEnvSpec, patch.cutoffEnvGain,
                pitchEnvSpec, patch.pitchEnvGain, patch);
        }

        // Apply final gain. Map from linear [0,1] to exponential from -80db to 0db.
        {
            float const startAmp = 0.01f;
            float const factor = 1.0f / startAmp;
            float gain = startAmp*powf(factor, patch.gainFactor);
            v *= gain;
        }

        for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
            *outputBuffer++ += v;
        }
    }
}
}
