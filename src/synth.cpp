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
    float dt = phaseChange / (2*kPi);
    float t = phase / (2*kPi);
    v += Polyblep(t, dt);
    v -= Polyblep(fmod(t + 0.5f, 1.0f), dt);
    return v;
}

float GenerateSaw(float const phase, float const phaseChange) {
    float v = (phase / kPi) - 1.0f;
    // polyblep
    float dt = phaseChange / (2*kPi);
    float t = phase / (2*kPi);
    v -= Polyblep(t, dt);
    return v;
}
}  // namespace

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
    assert(false);
    return 0.f;
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
            oscF = modulatedF * powf(2.f, patch.Get(SynthParamType::Detune));
        }

        float phaseChange = 2 * kPi * oscF / sampleRate;
        float oscGain;
        if (oscIx == 0) {
            oscGain = 1.f - patch.Get(SynthParamType::OscFader);
        } else {
            oscGain = patch.Get(SynthParamType::OscFader);
        }

        float oscV = 0.f;
        Waveform waveform = (oscIx == 0) ? patch.GetOsc1Waveform() : patch.GetOsc2Waveform();
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
    modulatedCutoff += patch.Get(SynthParamType::CutoffEnvGain) * voice.cutoffEnvState.currentValue;
	// The fancy filter below blows up for some reason if we don't include
	// this line.
	modulatedCutoff = math_util::Clamp(modulatedCutoff, 0.f, 22050.f);

    // ladder filter
#if 0
    float const rc = 1 / modulatedCutoff;
    float const a = dt / (rc + dt);
    v -= patch.Get(SynthParamType::Peak) * voice.lp3;

    voice.lp0 = a*v + (1-a)*voice.lp0;
    voice.lp1 = a*(voice.lp0) + (1-a)*voice.lp1;
    voice.lp2 = a*(voice.lp1) + (1-a)*voice.lp2;
    voice.lp3 = a*(voice.lp2) + (1-a)*voice.lp3;
    v = voice.lp3;

#else	
    v = UpdateFilter(dt, modulatedCutoff, cutoffK, v, FilterType::LowPass, voice.lpfState);

    v = UpdateFilter(dt, patch.Get(SynthParamType::HpfCutoff), patch.Get(SynthParamType::HpfPeak), v, FilterType::HighPass, voice.hpfState);
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
    int numVoices = 1;
    if (!state.patch.Get(SynthParamType::Mono)) {
        numVoices = state.voices.size();
    }
    for (int i = 0; i < numVoices; ++i) {
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

void Process(
    StateData* state, boost::circular_buffer<audio::Event> const& pendingEvents,
    float* outputBuffer, int const numChannels, int const samplesPerFrame,
    int const sampleRate, unsigned long frameStartTickTime) {

    Patch& patch = state->patch;

    int currentEventIx = 0;
    for(int i = 0; i < samplesPerFrame; ++i) {
        unsigned long currentTickTime = frameStartTickTime + i;
        // Handle events
        while (true) {
            if (currentEventIx >= pendingEvents.size()) {
                break;
            }
            audio::Event const& e = pendingEvents[currentEventIx];
            if (e.timeInTicks > currentTickTime) {
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
                            v->ampEnvState.ticksSincePhaseStart = (unsigned long) (v->ampEnvState.currentValue * patch.Get(SynthParamType::AmpEnvAttack) * sampleRate);
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
                        // Find a free automation. Also, ensure that there's no
                        // existing automation on this param.
                        Automation* pA = nullptr;
                        for (Automation& a : state->automations) {
                            if (a._active) {
                                if (a._synthParamType == e.param) {
                                    pA = nullptr;
                                    break;
                                }
                            } else if (pA == nullptr) {
                                pA = &a;
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
                        pA->_startTickTime = currentTickTime;
                        pA->_endTickTime = pA->_startTickTime + e.paramChangeTime;
                        break;
                    }
                    patch.Get(e.param) = e.newParamValue;
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
            if (currentTickTime > a._endTickTime) {
                a._active = false;
                continue;
            }
            double totalTime = a._endTickTime - a._startTickTime;
            assert(totalTime != 0.0);
            double timeSoFar = currentTickTime - a._startTickTime;
            double factor = timeSoFar / totalTime;
            float& currentValue = patch.Get(a._synthParamType);
            float newValue = a._startValue + factor * (a._desiredValue - a._startValue);
            currentValue = newValue;  
        }

        // Get pitch LFO value
        if (state->pitchLFOPhase >= 2*kPi) {
            state->pitchLFOPhase -= 2*kPi;
        }
        // Make LFO a sine wave for now.

        float const pitchLFOValue = patch.Get(SynthParamType::PitchLFOGain) * sinf(state->pitchLFOPhase);
        state->pitchLFOPhase += (patch.Get(SynthParamType::PitchLFOFreq) * 2*kPi / sampleRate);

        // Get cutoff LFO value
        if (state->cutoffLFOPhase >= 2*kPi) {
            state->cutoffLFOPhase -= 2*kPi;
        }
        // Make LFO a sine wave for now.
        float const cutoffLFOValue = patch.Get(SynthParamType::CutoffLFOGain) * sinf(state->cutoffLFOPhase);
        state->cutoffLFOPhase += (patch.Get(SynthParamType::CutoffLFOFreq) * 2*kPi / sampleRate);
        // float const modulatedCutoff = patch.cutoffFreq * powf(2.0f, cutoffLFOValue);
        float const modulatedCutoff = math_util::Clamp(patch.Get(SynthParamType::Cutoff) + 20000*cutoffLFOValue, 0.f, 44100.f);


        ADSREnvSpecInTicks ampEnvSpec, cutoffEnvSpec, pitchEnvSpec;
        ConvertADSREnvSpec(patch.GetAmpEnvSpec(), ampEnvSpec, sampleRate);
        ConvertADSREnvSpec(patch.GetCutoffEnvSpec(), cutoffEnvSpec, sampleRate);
        ConvertADSREnvSpec(patch.GetPitchEnvSpec(), pitchEnvSpec, sampleRate);

        float v = 0.0f;
        for (Voice& voice : state->voices) {
            v += ProcessVoice(
                voice, sampleRate, pitchLFOValue, modulatedCutoff, patch.Get(SynthParamType::Peak),
                ampEnvSpec, cutoffEnvSpec, patch.Get(SynthParamType::CutoffEnvGain),
                pitchEnvSpec, patch.Get(SynthParamType::PitchEnvGain), patch);
        }

        // Apply final gain. Map from linear [0,1] to exponential from -80db to 0db.
        {
            float const startAmp = 0.01f;
            float const factor = 1.0f / startAmp;
            float gain = startAmp*powf(factor, patch.Get(SynthParamType::Gain));
            v *= gain;
        }

        for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
            *outputBuffer++ += v;
        }
    }
}
}
