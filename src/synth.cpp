#include "synth.h"

#include <iostream>

#include "audio_util.h"
#include "constants.h"

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
                        // Keep current level. might have hit release before hitting sustain level
                        // TODO: calculate release from sustain to 0, or from current level to 0? I like sustain better.
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

    // TODO: make this process an entire buffer.
    // TODO: early exit if envelope is closed.
    // TODO: remove redundant args now that we pass Patch in.
    float ProcessVoice(
        Voice& voice, int const sampleRate, float pitchLFOValue, float modulatedCutoff, float cutoffK,
        ADSREnvSpecInTicks const& ampEnvSpec, ADSREnvSpecInTicks const& cutoffEnvSpec, float const cutoffEnvGain,
        Patch const& patch) {
        float const dt = 1.0f / sampleRate;

        // Now use the LFO value to get a new frequency.
        float const modulatedF = voice.oscillators[0].f * powf(2.0f, pitchLFOValue);

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
            }

            float oscWithGain = oscV * oscGain;
            v += oscWithGain;
            osc.phase += phaseChange;
        }

        // Cutoff envelope
        adsrEnvelope(cutoffEnvSpec, voice.cutoffEnvState);
        modulatedCutoff += patch.cutoffEnvGain * voice.cutoffEnvState.currentValue;

        // ladder filter
        float const rc = 1 / modulatedCutoff;
        float const a = dt / (rc + dt);
        v -= patch.cutoffK * voice.lp3;

        voice.lp0 = a*v + (1-a)*voice.lp0;
        voice.lp1 = a*(voice.lp0) + (1-a)*voice.lp1;
        voice.lp2 = a*(voice.lp1) + (1-a)*voice.lp2;
        voice.lp3 = a*(voice.lp2) + (1-a)*voice.lp3;
        v = voice.lp3;

        // Amplitude envelope
        // TODO: move this up top so we can early-exit if the envelope is closed and save some computation.
        // TODO: consider only doing the envelope computation once per buffer frame.
        adsrEnvelope(ampEnvSpec, voice.ampEnvState);
        if (voice.ampEnvState.phase == ADSRPhase::Closed) {
            voice.currentMidiNote = -1;
        }
        v *= voice.ampEnvState.currentValue;

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

    void Process(
        StateData* state, boost::circular_buffer<audio::Event> const& pendingEvents,
        float* outputBuffer, int const numChannels, int const samplesPerFrame,
        int const sampleRate, unsigned long frameStartTickTime) {

        Patch& patch = state->patch;

        int currentEventIx = 0;
        for(int i = 0; i < samplesPerFrame; ++i)
        {
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
                            v->ampEnvState.phase = synth::ADSRPhase::Attack;
                            v->ampEnvState.ticksSincePhaseStart = 0;
                            v->cutoffEnvState.phase = synth::ADSRPhase::Attack;
                            v->cutoffEnvState.ticksSincePhaseStart = 0;
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
                            }
                        }
                        break;
                    }
                    case audio::EventType::SynthParam: {
                        switch (e.param) {
                            case audio::SynthParamType::Gain:
                                patch.gainFactor = e.newParamValue;
                                break;
                            case audio::SynthParamType::Cutoff:
                                patch.cutoffFreq = e.newParamValue;
                                break;
                            case audio::SynthParamType::Peak:
                                patch.cutoffK = e.newParamValue;
                                break;
                            case audio::SynthParamType::PitchLFOGain:
                                patch.pitchLFOGain = e.newParamValue;
                                break;
                            case audio::SynthParamType::PitchLFOFreq:
                                patch.pitchLFOFreq = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffLFOGain:
                                patch.cutoffLFOGain = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffLFOFreq:
                                patch.cutoffLFOFreq = e.newParamValue;
                                break;
                            case audio::SynthParamType::AmpEnvAttack:
                                patch.ampEnvSpec.attackTime = e.newParamValue;
                                break;
                            case audio::SynthParamType::AmpEnvDecay:
                                patch.ampEnvSpec.decayTime = e.newParamValue;
                                break;
                            case audio::SynthParamType::AmpEnvSustain:
                                patch.ampEnvSpec.sustainLevel = e.newParamValue;
                                break;
                            case audio::SynthParamType::AmpEnvRelease:
                                patch.ampEnvSpec.releaseTime = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffEnvGain:
                                patch.cutoffEnvGain = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffEnvAttack:
                                patch.cutoffEnvSpec.attackTime = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffEnvDecay:
                                patch.cutoffEnvSpec.decayTime = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffEnvSustain:
                                patch.cutoffEnvSpec.sustainLevel = e.newParamValue;
                                break;
                            case audio::SynthParamType::CutoffEnvRelease:
                                patch.cutoffEnvSpec.releaseTime = e.newParamValue;
                                break;
                            default:
                                std::cout << "Received unsupported synth param " << static_cast<int>(e.param) << std::endl;
                                break;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
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
            float const modulatedCutoff = patch.cutoffFreq * powf(2.0f, cutoffLFOValue);

            ADSREnvSpecInTicks ampEnvSpec, cutoffEnvSpec;
            ConvertADSREnvSpec(patch.ampEnvSpec, ampEnvSpec, sampleRate);
            ConvertADSREnvSpec(patch.cutoffEnvSpec, cutoffEnvSpec, sampleRate);

            float v = 0.0f;
            for (Voice& voice : state->voices) {
                v += ProcessVoice(
                    voice, sampleRate, pitchLFOValue, modulatedCutoff, patch.cutoffK, ampEnvSpec, cutoffEnvSpec, patch.cutoffEnvGain, patch);
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