#include "synth.h"

#include <iostream>

#include "audio_util.h"
#include "constants.h"
#include "math_util.h"

using audio::SynthParamType;

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
    if (phase < kPi) {
        v = 1.0f;
    } else {
        v = -1.0f;
    }
    // polyblep
    // float dt = phaseChange / (2*kPi);
    // float t = phase / (2*kPi);
    // v += Polyblep(t, dt);
    // v -= Polyblep(fmod(t + 0.5f, 1.0f), dt);
    return v;
}

float GenerateSaw(float const phase, float const phaseChange) {
    float v = (phase / kPi) - 1.0f;
    // polyblep
    // float dt = phaseChange / (2*kPi);
    // float t = phase / (2*kPi);
    // v -= Polyblep(t, dt);
    return v;
}
}  // namespace

void InitStateData(StateData& state, int channel, int const sampleRate, int const samplesPerFrame, int const numBufferChannels) {
    state = StateData();
    state.channel = channel;
    state.voiceScratchBuffer = new float[samplesPerFrame * numBufferChannels];
    state.sampleRate = sampleRate;
    state.framesPerBuffer = samplesPerFrame;

    int constexpr kSamplesPerCutoffEnvModulate = 1;
    state.samplesPerMoogCutoffUpdate = std::min(samplesPerFrame, kSamplesPerCutoffEnvModulate);

    for (Voice& v : state.voices) {
        v.moogLpfState.reset(sampleRate);
    }
}

void DestroyStateData(StateData& state) {
    delete[] state.voiceScratchBuffer;
    state.voiceScratchBuffer = nullptr;
}

float constexpr kMinValue = 0.001f;

float calcMultiplier(float startLevel, float endLevel, int64_t numSamples) {
    assert(numSamples > 0l);
    assert(startLevel > 0.f);
    assert(startLevel <= 1.f);
    assert(endLevel <= 1.f);
    double recip = 1.0 / numSamples;
    double endOverStart = endLevel / startLevel;
    double val = pow(endOverStart, recip);
    return (float) val;
}

void AdsrTick(ADSREnvSpecInternal const& spec, ADSRStateNew* state) {
    ADSRPhase prevPhase = state->phase;
    switch (state->phase) {
        case ADSRPhase::Attack: {
            state->atkAuxVal *= state->multiplier;
            state->value = 1 - state->atkAuxVal;
            if (state->value >= 0.95f) {
                state->value = 1.f;
                state->phase = ADSRPhase::Decay;
                state->targetLevel = spec.sustainLevel;
                if (spec.decayTime <= 0) {
                    state->value = spec.sustainLevel;
                    state->multiplier = 1.f;
                } else {
                    state->multiplier = calcMultiplier(1.f, kMinValue, spec.decayTime);
                }
            }
            
            break;
        }
        case ADSRPhase::Decay: {
            state->value *= state->multiplier;
            // I use the stored targetLevel instead of the spec's sustain level
            // so that we don't get weird behavior when changing sustain level
            // while a note is being held.
            if (state->value <= state->targetLevel) {
                state->value = state->targetLevel;
                state->phase = ADSRPhase::Sustain;
            }           
            break;
        }
        case ADSRPhase::Sustain: {
            break;
        }
        case ADSRPhase::Release: {
            state->value *= state->multiplier;
            if (state->value <= kMinValue) {
                state->value = 0.f;
                state->phase = ADSRPhase::Closed;
            }           
            break;
        }
        case ADSRPhase::Closed: {
            break;
        }
    }
    if (prevPhase != state->phase) {
        state->ticksSincePhaseStart = 0;
    } else {
        ++state->ticksSincePhaseStart;
    }
}

struct ADSREnvSpecInTicks {
    int64_t attackTime = 0l;
    int64_t decayTime = 0l;
    float sustainLevel = 0.f;
    int64_t releaseTime = 0l;
    float minValue = 0.01f;
};

void adsrEnvelope(ADSREnvSpecInTicks const& spec, int const samplesPerFrame, ADSREnvState& state) {
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
            state.ticksSincePhaseStart += samplesPerFrame;
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
                    state.multiplier = calcMultiplier(1.f, spec.sustainLevel, (float)spec.decayTime / (float)samplesPerFrame);
                }
            }
            state.currentValue *= state.multiplier;
            state.ticksSincePhaseStart += samplesPerFrame;
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
                    state.multiplier = calcMultiplier(spec.sustainLevel, spec.minValue, (float)spec.releaseTime / (float) samplesPerFrame);
                }
            }
            state.currentValue *= state.multiplier;
            state.ticksSincePhaseStart += samplesPerFrame;
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
float UpdateFilter(float const a1, float const a2, float const a3, float const k, float const input, FilterType const filterType, FilterState& state) {
    float v3 = input - state.ic2eq;
    float v1 = a1 * state.ic1eq + a2 * v3;
    float v2 = state.ic2eq + a2 * state.ic1eq + a3 * v3;
    state.ic1eq = 2 * v1 - state.ic1eq;
    state.ic2eq = 2 * v2 - state.ic2eq;

    switch (filterType) {
    case FilterType::LowPass: return v2;
    case FilterType::HighPass: {
        float output = input - k * v1 - v2;
        return output;
    }
    }
    assert(false);
    return 0.f;
}

void ProcessVoice(Voice& voice, int const sampleRate, float pitchLFOValue,
    float modulatedCutoff, ADSREnvSpecInternal const& ampEnvSpec,
    ADSREnvSpecInternal const& cutoffEnvSpec,
    ADSREnvSpecInTicks const& pitchEnvSpec,
                  Patch const& patch, float* outputBuffer, int const numChannels, int const samplesPerFrame, int const samplesPerMoogCutoffUpdate) {
    float const dt = 1.f / sampleRate;

    // portamento
    if (voice.postPortamentoF <= 0.f) {
        voice.postPortamentoF = voice.oscillators[0].f;
    }

    float const framesPerOctave = patch.Get(SynthParamType::Portamento) * sampleRate / static_cast<float>(samplesPerFrame);
    if (framesPerOctave <= 0.f) {
        voice.postPortamentoF = voice.oscillators[0].f;
    }
    else {
        float const kPortamentoFactor = powf(2.f, 1.f / framesPerOctave);
        if (voice.postPortamentoF > voice.oscillators[0].f) {
            voice.postPortamentoF /= kPortamentoFactor;
            voice.postPortamentoF = std::max(voice.oscillators[0].f, voice.postPortamentoF);
        }
        else if (voice.postPortamentoF < voice.oscillators[0].f) {
            voice.postPortamentoF *= kPortamentoFactor;
            voice.postPortamentoF = std::min(voice.oscillators[0].f, voice.postPortamentoF);
        }
    }

    // Now use the LFO value to get a new frequency.
    float modulatedF = voice.postPortamentoF * powf(2.0f, pitchLFOValue);

    // Modulate by pitch envelope.
    adsrEnvelope(pitchEnvSpec, samplesPerFrame, voice.pitchEnvState);
    modulatedF *= powf(2.f, patch.Get(audio::SynthParamType::PitchEnvGain) * voice.pitchEnvState.currentValue);
    // TODO: clamp F?

    // final gain. Map from linear [0,1] to exponential from -80db to 0db.
    float const startAmp = 0.01f;
    float const factor = 1.0f / startAmp;
    float gain = 0.f;
    float const patchGain = patch.Get(SynthParamType::Gain);
    if (patchGain > 0.f) {
        gain = startAmp * powf(factor, patch.Get(SynthParamType::Gain));
    }    

    // float lpfA1, lpfA2, lpfA3, lpfK;  // filter shit
    // {
    //     float res = patch.Get(SynthParamType::Peak) / 4.f;
    //     float g = tan(kPi * modulatedCutoff * dt);
    //     lpfK = 2 - 2 * res;
    //     assert((1 + g * (g + lpfK)) != 0.f);
    //     lpfA1 = 1 / (1 + g * (g + lpfK));
    //     lpfA2 = g * lpfA1;
    //     lpfA3 = g * lpfA2;
    // }

    // TODO: only run this if cutoff has changed.
    float hpfA1, hpfA2, hpfA3, hpfK;  // filter shit
    {
        float res = patch.Get(SynthParamType::HpfPeak) / 4.f;
        float g = tan(kPi * patch.Get(SynthParamType::HpfCutoff) * dt);
        hpfK = 2 - 2 * res;
        assert((1 + g * (g + hpfK)) != 0.f);
        hpfA1 = 1 / (1 + g * (g + hpfK));
        hpfA2 = g * hpfA1;
        hpfA3 = g * hpfA2;
    }

    

    for (int oscIx = 0; oscIx < voice.oscillators.size(); ++oscIx) {
        Oscillator& osc = voice.oscillators[oscIx];
        float oscF = modulatedF;
        if (oscIx > 0) {
            oscF = modulatedF * powf(2.f, patch.Get(SynthParamType::Detune));
        }
        float phaseChange = 2 * kPi * oscF / sampleRate;
        float oscGain;
        if (oscIx == 0) {
            oscGain = 1.f - patch.Get(SynthParamType::OscFader);
        }
        else {
            oscGain = patch.Get(SynthParamType::OscFader);
        }
        Waveform const waveform = (oscIx == 0) ? patch.GetOsc1Waveform() : patch.GetOsc2Waveform();
        int outputIx = 0;
        for (int sampleIx = 0; sampleIx < samplesPerFrame; ++sampleIx) {
            if (osc.phase >= 2 * kPi) {
                osc.phase -= 2 * kPi;
            }
            float oscV = 0.f;
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
            for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                outputBuffer[outputIx++] += oscWithGain;
            }

            osc.phase += phaseChange;
        }
    }

    // Apply filters and some more gains
    {
        int outputIx = 0;
        int cutoffModulateCounter = 0;
        for (int sampleIx = 0; sampleIx < samplesPerFrame; ++sampleIx) {
            float v = outputBuffer[outputIx];

            // Cutoff envelope
            if (cutoffModulateCounter <= 0) {
                cutoffModulateCounter = samplesPerMoogCutoffUpdate;
                AdsrTick(cutoffEnvSpec, &voice.cutoffEnvState);
                float c = modulatedCutoff + patch.Get(SynthParamType::CutoffEnvGain) * voice.cutoffEnvState.value;
                c = math_util::Clamp(c, 0.f, 20000.f);
                float peak = patch.Get(SynthParamType::Peak);
                voice.moogLpfState.setFilterParams(c, peak);
            }
            --cutoffModulateCounter;                
            
            // v = UpdateFilter(lpfA1, lpfA2, lpfA3, lpfK, v, FilterType::LowPass, voice.lpfState);
            filter::FilterOutput* fo = voice.moogLpfState.process(v);
            // v = fo->filter[filter::ANM_LPF4];
            v = fo->filter[filter::LPF4];
            v = UpdateFilter(hpfA1, hpfA2, hpfA3, hpfK, v, FilterType::HighPass, voice.hpfState);
            AdsrTick(ampEnvSpec, &voice.ampEnvState);
            if (voice.ampEnvState.phase == ADSRPhase::Closed) {
                voice.currentMidiNote = -1;
            }
            v *= voice.ampEnvState.value;
            v *= voice.velocity;
            v *= gain;

            for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                outputBuffer[outputIx++] = v;
            }
        }
    }
}

namespace {
    int ADSRPhaseScore(ADSRPhase phase) {
        switch (phase) {
        case ADSRPhase::Closed: return 4;
        case ADSRPhase::Attack: return 0;
        case ADSRPhase::Decay: return 1;
        case ADSRPhase::Sustain: return 2;
        case ADSRPhase::Release: return 3;
        }
        return 0;
    }
}

Voice* FindVoiceForNoteOn(StateData& state, int const midiNote) {
    int bestIx = -1;
    std::pair<int, int> bestPhaseAgePair = std::make_pair(-1, -1);
    int numVoices = 1;
    if (!state.patch.Get(SynthParamType::Mono)) {
        numVoices = state.voices.size();
    }
    for (int i = 0; i < numVoices; ++i) {
        Voice& v = state.voices[i];
        if (v.currentMidiNote == midiNote) {
            return &v;
        }
        int thisPhaseScore = ADSRPhaseScore(v.ampEnvState.phase);
        if (thisPhaseScore > bestPhaseAgePair.first) {
            bestIx = i;
            bestPhaseAgePair = std::make_pair(thisPhaseScore, v.ampEnvState.ticksSincePhaseStart);
        } else if (thisPhaseScore == bestPhaseAgePair.first) {
            if (v.ampEnvState.ticksSincePhaseStart > bestPhaseAgePair.second) {
                bestIx = i;
                bestPhaseAgePair = std::make_pair(thisPhaseScore, v.ampEnvState.ticksSincePhaseStart);
            }
        }
    }
    if (bestIx >= 0) {
        return &state.voices[bestIx];
    }
    return nullptr;
}

Voice* FindVoiceForNoteOff(StateData& state, int midiNote, int noteOffId) {
    // NOTE: we should be able to return as soon as we find a voice matching
    // the given note, but I'm doing the full iteration to check for
    // correctness for now.
    Voice* resultVoice = nullptr;
    for (Voice& v : state.voices) {
        if (v.currentMidiNote == midiNote) {
            if (noteOffId == 0 || v.noteOnId == 0 || (v.noteOnId == noteOffId)) {
                assert(resultVoice == nullptr);
                resultVoice = &v;
            }
        }
    }
    return resultVoice;
}

void ConvertADSREnvSpec(ADSREnvSpec const& spec, ADSREnvSpecInTicks& specInTicks, int sampleRate) {
    specInTicks.attackTime = (int64_t)(spec.attackTime * sampleRate);
    specInTicks.decayTime = (int64_t)(spec.decayTime * sampleRate);
    specInTicks.sustainLevel = spec.sustainLevel;
    specInTicks.releaseTime = (int64_t)(spec.releaseTime * sampleRate);
    specInTicks.minValue = spec.minValue;
}

void ConvertADSREnvSpec(ADSREnvSpec const& spec, ADSREnvSpecInternal& specInternal, float sampleRate, int samplesPerModulation) {
    specInternal.modulationHz = sampleRate / samplesPerModulation;
    specInternal.attackTime = spec.attackTime * sampleRate / samplesPerModulation;
    specInternal.decayTime = spec.decayTime * sampleRate / samplesPerModulation;
    specInternal.sustainLevel = spec.sustainLevel;
    specInternal.releaseTime = spec.releaseTime * sampleRate / samplesPerModulation;
}

// "Samples" is simply the number of iterations we plan to run this envelope. In some cases it might actually be number of samples;
// in other cases, it might be number of buffers; or something in between. It depends on how the caller updates the envelope.
void ADSRNoteOn(ADSREnvSpecInternal const& spec, ADSRStateNew& adsrState) {
    adsrState.ticksSincePhaseStart = 0;
    adsrState.phase = synth::ADSRPhase::Attack;
    if (spec.attackTime <= 0.f) {
        adsrState.multiplier = 1.f;
        adsrState.value = 1.f;
        adsrState.atkAuxVal = 0.f;
    } else {
        adsrState.multiplier = calcMultiplier(1.f, kMinValue, spec.attackTime);
        adsrState.atkAuxVal = 1.f - adsrState.value;
    }
    if (adsrState.value < kMinValue) {
        adsrState.value = kMinValue;
    }
}

void NoteOn(StateData& state, int midiNote, float velocity, int noteOnId) {
    Voice* v = FindVoiceForNoteOn(state, midiNote);
    if (v != nullptr) {
        v->oscillators[0].f = synth::MidiToFreq(midiNote);
        for (int oscIx = 1; oscIx < v->oscillators.size(); ++oscIx) {
            Oscillator& osc = v->oscillators[oscIx];
            osc.phase = v->oscillators[0].phase;
        }
        v->currentMidiNote = midiNote;
        v->velocity = velocity;
        v->noteOnId = noteOnId;
        
        ADSRNoteOn(state.ampEnvSpecInternal, v->ampEnvState);
        ADSRNoteOn(state.cutoffEnvSpecInternal, v->cutoffEnvState);

        v->pitchEnvState.phase = synth::ADSRPhase::Attack;
        v->pitchEnvState.ticksSincePhaseStart = v->ampEnvState.ticksSincePhaseStart;
    } else {
        std::cout << "couldn't find a note for noteon" << std::endl;
    }
}

void ADSRNoteOff(ADSREnvSpecInternal const& spec, ADSRStateNew& adsrState) {
    adsrState.phase = synth::ADSRPhase::Release;
    adsrState.ticksSincePhaseStart = 0;
    if (spec.releaseTime <= 0.f) {
        adsrState.multiplier = 1.f;
        adsrState.value = 0.f;
    } else {
        adsrState.multiplier = calcMultiplier(1.f, kMinValue, spec.releaseTime);
    }
}

void NoteOff(StateData& state, int midiNote, int noteOnId) {
    Voice* v = FindVoiceForNoteOff(state, midiNote, noteOnId);
    if (v != nullptr) {
        ADSRNoteOff(state.ampEnvSpecInternal, v->ampEnvState);
        ADSRNoteOff(state.cutoffEnvSpecInternal, v->cutoffEnvState);
        
        v->pitchEnvState.phase = synth::ADSRPhase::Release;
        v->pitchEnvState.ticksSincePhaseStart = 0;
    }
}
void AllNotesOff(StateData& state) {
    for (Voice& v : state.voices) {
        if (v.currentMidiNote != -1) {
            v.ampEnvState.phase = synth::ADSRPhase::Release;
            v.ampEnvState.ticksSincePhaseStart = 0;
            v.cutoffEnvState.phase = synth::ADSRPhase::Release;
            v.cutoffEnvState.ticksSincePhaseStart = 0;
            v.pitchEnvState.phase = synth::ADSRPhase::Release;
            v.pitchEnvState.ticksSincePhaseStart = 0;
        }
    }
}

void OnParamChange(StateData& state, audio::SynthParamType paramType, float newValue, Voice* v) {
    switch (paramType) {
    case audio::SynthParamType::AmpEnvAttack:
    case audio::SynthParamType::AmpEnvDecay:
    case audio::SynthParamType::AmpEnvSustain:
    case audio::SynthParamType::AmpEnvRelease:
        ConvertADSREnvSpec(state.patch.GetAmpEnvSpec(), state.ampEnvSpecInternal, state.sampleRate, 1);
        break;
    case audio::SynthParamType::CutoffEnvAttack:
    case audio::SynthParamType::CutoffEnvDecay:
    case audio::SynthParamType::CutoffEnvSustain:
    case audio::SynthParamType::CutoffEnvRelease:
        ConvertADSREnvSpec(state.patch.GetCutoffEnvSpec(), state.cutoffEnvSpecInternal, state.sampleRate, state.samplesPerMoogCutoffUpdate);
        break;

        // So we didn't need this at all???? COOL
        // case audio::SynthParamType::AmpEnvAttack: {
        //     float attackTime = newValue;
        //     if (attackTime <= 0.f) {
        //         v->ampEnvState.multiplier = 1.f;
        //         v->ampEnvState.value = 1.f;
        //     } else {
        //         int64_t numSamples = attackTime * state.sampleRate;
        //         v->ampEnvState.multiplier = calcMultiplier(kMinValue, 1.f, numSamples);
        //     }
        //     break;
        // }
        // case audio::SynthParamType::AmpEnvRelease: {
        //     float releaseTime = newValue;
        //     if (releaseTime <= 0.f) {
        //         v->ampEnvState.multiplier = 1.f;
        //         v->ampEnvState.value = 0.f;
        //     } else {
        //         int64_t numSamples = releaseTime * state.sampleRate;
        //         v->ampEnvState.multiplier = calcMultiplier(1.f, kMinValue, numSamples);
        //     }
        //     break;
        // }

        
        // MIGHT NOT NEED THIS IF WE'RE CALLING STUFF EVERY BLOCK?
        // case audio::SynthParamType::Cutoff: {
        //     float peak = state.patch.Get(audio::SynthParamType::Peak);
        //     for (Voice& v : state.voices) {
        //         v.moogLpfState.setFilterParams(newValue, peak);
        //     }
        //     break;
        // }
        // case audio::SynthParamType::Peak: {
        //     float cutoff = state.patch.Get(audio::SynthParamType::Cutoff);
        //     for (Voice& v : state.voices) {
        //         v.moogLpfState.setFilterParams(cutoff, newValue);
        //     }
        //     break;
        // }
        default: break;
    }
}

void Process(StateData* state, boost::circular_buffer<audio::PendingEvent> const& pendingEvents,
    float* outputBuffer, int const numChannels, int const samplesPerFrame,
    int const sampleRate, int64_t currentBufferCounter) {
    
    Patch& patch = state->patch;

    // Handle all the events that will happen in this buffer frame.
    int64_t frameStartTickTime = currentBufferCounter * samplesPerFrame;
    int64_t frameLastTickTime = frameStartTickTime + samplesPerFrame;
    int currentEventIx = 0;
    while (true) {
        if (currentEventIx >= pendingEvents.size()) {
            break;
        }
        audio::PendingEvent const& p_e = pendingEvents[currentEventIx];
        if (currentBufferCounter < p_e._runBufferCounter) {
            break;
        }
        audio::Event const& e = p_e._e;
        if (e.channel != state->channel) {
            // Not meant for this channel. Skip this message.
            ++currentEventIx;
            continue;
        }
        ++currentEventIx;
        switch (e.type) {
            case audio::EventType::NoteOn: {
                NoteOn(*state, e.midiNote, e.velocity, e.noteOnId);
                break;
            }
            case audio::EventType::NoteOff: {
                NoteOff(*state, e.midiNote, e.noteOnId);
                break;
            }
            case audio::EventType::AllNotesOff: {
                AllNotesOff(*state);
                break;
            }
            case audio::EventType::SynthParam: {
                if (e.paramChangeTimeSecs > 0.0) {
                    // Find a free automation. Also, ensure that there's no
                    // existing automation on this param.
                    for (Automation& a : state->automations) {
                        if (a._active && a._synthParamType == e.param) {
                            a._active = false;
                        }
                    }
                    Automation* pA = nullptr;
                    for (Automation& a : state->automations) {
                        if (!a._active) {
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
                    pA->_startValue = patch.Get(e.param);
                    pA->_desiredValue = e.newParamValue;
                    pA->_startTickTime = frameStartTickTime;
                    int64_t changeTimeInTicks = e.paramChangeTimeSecs * sampleRate;
                    pA->_endTickTime = pA->_startTickTime + changeTimeInTicks;
                    for (Voice& v : state->voices) {
                        OnParamChange(*state, e.param, e.newParamValue, &v);
                    }
                    break;
                }
                patch.Get(e.param) = e.newParamValue;

                for (Voice& v : state->voices) {
                    OnParamChange(*state, e.param, e.newParamValue, &v);
                }
                break;
            }
            default: {
                break;
            }
        }
    }

    // Now apply automations!
    for (Automation& a : state->automations) {
        if (!a._active) {
            continue;
        }
        if (frameStartTickTime > a._endTickTime) {
            patch.Get(a._synthParamType) = a._desiredValue;
            a._active = false;
            continue;
        }
        double totalTime = a._endTickTime - a._startTickTime;
        assert(totalTime != 0.0);
        double timeSoFar = frameStartTickTime - a._startTickTime;
        double factor = std::min(timeSoFar / totalTime, 1.0);
        float& currentValue = patch.Get(a._synthParamType);
        float newValue = a._startValue + factor * (a._desiredValue - a._startValue);
        currentValue = newValue;

        for (Voice& v : state->voices) {
            OnParamChange(*state, a._synthParamType, newValue, &v);
        }
    }

    // Get pitch LFO value
    if (state->pitchLFOPhase >= 2 * kPi) {
        state->pitchLFOPhase -= 2 * kPi;
    }
    // Make LFO a sine wave.
    float const pitchLFOValue = patch.Get(SynthParamType::PitchLFOGain) * sinf(state->pitchLFOPhase);
    state->pitchLFOPhase += (patch.Get(SynthParamType::PitchLFOFreq) * 2 * kPi * samplesPerFrame / sampleRate);

    // Get cutoff LFO value
    if (state->cutoffLFOPhase >= 2 * kPi) {
        state->cutoffLFOPhase -= 2 * kPi;
    }
    // Make LFO a sine wave for now.
    float const cutoffLFOValue = patch.Get(SynthParamType::CutoffLFOGain) * sinf(state->cutoffLFOPhase);
    state->cutoffLFOPhase += (patch.Get(SynthParamType::CutoffLFOFreq) * 2 * kPi * samplesPerFrame / sampleRate);
    // float const modulatedCutoff = patch.cutoffFreq * powf(2.0f, cutoffLFOValue);
    float const modulatedCutoff = math_util::Clamp(patch.Get(SynthParamType::Cutoff) + 10000 * cutoffLFOValue, 0.f, 20000.f);

    ADSREnvSpecInTicks pitchEnvSpec;
    ConvertADSREnvSpec(patch.GetPitchEnvSpec(), pitchEnvSpec, sampleRate);

    for (Voice& voice : state->voices) {
        // zero out the voice scratch buffer.
        memset(state->voiceScratchBuffer, 0, numChannels * samplesPerFrame * sizeof(float));
        ProcessVoice(voice, sampleRate, pitchLFOValue, modulatedCutoff, state->ampEnvSpecInternal, state->cutoffEnvSpecInternal, pitchEnvSpec, patch, state->voiceScratchBuffer, numChannels, samplesPerFrame, state->samplesPerMoogCutoffUpdate);
        for (int outputIx = 0; outputIx < numChannels * samplesPerFrame; ++outputIx) {
            outputBuffer[outputIx] += state->voiceScratchBuffer[outputIx];
        }
    }
}

}
