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
        state.channel = channel;

        for (int i = 0; i < state.voices.size(); ++i) {
            Voice& v = state.voices[i];
            v.left_phase = v.right_phase = 0.0f;
            v.f = 440.0f;
            v.lp0 = 0.0f;
            v.lp1 = 0.0f;
            v.lp2 = 0.0f;
            v.lp3 = 0.0f;
        }

        state.pitchLFOPhase = 0.0f;
        state.cutoffLFOPhase = 0.0f;

        Patch& patch = state.patch;
        patch.name = "synth";
        patch.gainFactor = 0.7f;
        patch.cutoffFreq = 44100.0f;
        patch.cutoffK = 0.0f;
        patch.pitchLFOFreq = 1.0f;
        patch.pitchLFOGain = 0.0f;
        
        patch.cutoffLFOFreq = 10.0f;
        patch.cutoffLFOGain = 0.0f;        

        patch.ampEnvSpec.attackTime = 0.01f;
        patch.ampEnvSpec.decayTime = 0.1f;
        patch.ampEnvSpec.sustainLevel = 0.5f;
        patch.ampEnvSpec.releaseTime = 0.5f;
        patch.ampEnvSpec.minValue = 0.01f;

        patch.cutoffEnvSpec.minValue = 0.01f;
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
        float minValue = kSmallAmplitude;
    };

    void adsrEnvelope(ADSREnvSpecInTicks const& spec, ADSREnvState& state) {
        switch (state.phase) {
            case ADSRPhase::Closed:
                // Do nothing
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
    float ProcessVoice(
        Voice& voice, int const sampleRate, float pitchLFOValue, float modulatedCutoff, float cutoffK,
        ADSREnvSpecInTicks const& ampEnvSpec, ADSREnvSpecInTicks const& cutoffEnvSpec, float const cutoffEnvGain) {
        float const dt = 1.0f / sampleRate;

        // Now use the LFO value to get a new frequency.
        float const modulatedF = voice.f * powf(2.0f, pitchLFOValue);

        // use left phase for both channels for now.
        if (voice.left_phase >= 2*kPi) {
            voice.left_phase -= 2*kPi;
        }

        float phaseChange = 2*kPi*modulatedF / sampleRate;

        float v = 0.0f;
        {
            // v = GenerateSquare(voice.left_phase, phaseChange);
            v = GenerateSaw(voice.left_phase, phaseChange);
            voice.left_phase += phaseChange;
        }

        // Cutoff envelope
        adsrEnvelope(cutoffEnvSpec, voice.cutoffEnvState);
        modulatedCutoff += cutoffEnvGain * voice.cutoffEnvState.currentValue;

        // ladder filter
        float const rc = 1 / modulatedCutoff;
        float const a = dt / (rc + dt);
        v -= cutoffK*voice.lp3;

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
        StateData* state, audio::EventsThisFrame const& frameEvents, int eventCount,
        float* outputBuffer, int const numChannels, int const framesPerBuffer, int const sampleRate) {

        Patch& patch = state->patch;     
        
        int frameEventIx = 0;
        for(int i = 0; i < framesPerBuffer; ++i)
        {
            // Handle events
            while (true) {
                if (frameEventIx >= eventCount) {
                    break;
                }
                audio::FrameEvent const& fe = frameEvents[frameEventIx];
                audio::Event const& e = fe._e;
                if (e.channel != state->channel) {
                    // Not meant for this channel. Skip this message.
                    ++frameEventIx;
                    continue;
                }
                if (fe._sampleIx != i) {
                    // This event gets processed on a later sample this frame.
                    break;
                }
                switch (e.type) {
                    case audio::EventType::NoteOn: {
                        Voice* v = FindVoiceForNoteOn(*state, e.midiNote);
                        if (v != nullptr) {
                            v->f = synth::MidiToFreq(e.midiNote);
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
                ++frameEventIx;
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
                    voice, sampleRate, pitchLFOValue, modulatedCutoff, patch.cutoffK, ampEnvSpec, cutoffEnvSpec, patch.cutoffEnvGain);
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