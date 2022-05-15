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

        state.gainFactor = 0.7f;
        state.cutoffFreq = 44100.0f;
        state.cutoffK = 0.0f;
        state.pitchLFOFreq = 1.0f;
        state.pitchLFOGain = 0.0f;
        state.pitchLFOPhase = 0.0f;
        state.cutoffLFOFreq = 10.0f;
        state.cutoffLFOGain = 0.0f;
        state.cutoffLFOPhase = 0.0f;
        state.ampEnvAttackTime = 0.01f;
        state.ampEnvDecayTime = 0.1f;
        state.ampEnvSustainLevel = 0.5f;
        state.ampEnvReleaseTime = 0.5f;
    }

    // TODO: make this process an entire buffer.
    // TODO: early exit if envelope is closed.
    float ProcessVoice(
        Voice& voice, int const sampleRate, float pitchLFOValue, float modulatedCutoff, float cutoffK,
        int const attackTimeInTicks, int const decayTimeInTicks, float const ampEnvSustainLevel, int const releaseTimeInTicks) {
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
        float ampEnvValue = 0.f;
        if (voice.ampEnvTicksSinceADSPhaseStart >= 0.f) {
            // TODO: do we want to let the ADS curve continue evolving after note-off, or should we freeze it at note-off?
            float adsEnvValue = 0.f;
            switch (voice.adsPhase) {
                case ADSPhase::Attack: {
                    float t;
                    if (attackTimeInTicks == 0) {
                        t = 1.0f;
                    } else {
                        t = fmin(1.0f, (float) voice.ampEnvTicksSinceADSPhaseStart / (float) attackTimeInTicks);
                    }
                    float const startAmp = kSmallAmplitude;
                    float const factor = 1.0f / startAmp;
                    adsEnvValue = startAmp*powf(factor, t);
                    ++voice.ampEnvTicksSinceADSPhaseStart;
                    if (voice.ampEnvTicksSinceADSPhaseStart >= attackTimeInTicks) {
                        voice.adsPhase = ADSPhase::Decay;
                        voice.ampEnvTicksSinceADSPhaseStart = 0;
                    }
                    break;
                }
                case ADSPhase::Decay: {
                    float t;
                    if (decayTimeInTicks == 0) {
                        t = 1.0f;
                    } else {;
                        t = fmin(1.0f, (float) voice.ampEnvTicksSinceADSPhaseStart / (float) decayTimeInTicks);
                    }                        
                    float const sustain = fmax(kSmallAmplitude, ampEnvSustainLevel);
                    adsEnvValue = 1.0f*powf(sustain, t);
                    ++voice.ampEnvTicksSinceADSPhaseStart;
                    if (voice.ampEnvTicksSinceADSPhaseStart >= decayTimeInTicks) {
                        voice.adsPhase = ADSPhase::Sustain;
                        voice.ampEnvTicksSinceADSPhaseStart = 0;
                    }
                    break;
                }
                case ADSPhase::Sustain: {
                    adsEnvValue = ampEnvSustainLevel;
                    break;
                }
            }
            float ampReleaseEnv = 1.f;
            if (voice.ampEnvTicksSinceNoteOff >= 0) {
                float t;
                if (releaseTimeInTicks == 0) {
                    t = 1.0f;
                } else {
                    t = fmin(1.0f, (float) voice.ampEnvTicksSinceNoteOff / (float) releaseTimeInTicks);
                }
                ampReleaseEnv = powf(kSmallAmplitude, t);
                ++voice.ampEnvTicksSinceNoteOff;
                if (voice.ampEnvTicksSinceNoteOff >= releaseTimeInTicks) {
                    // Reset envelope state.
                    voice.ampEnvTicksSinceADSPhaseStart = -1;
                    voice.ampEnvTicksSinceNoteOff = -1;
                    voice.currentMidiNote = -1;
                }
            }

            ampEnvValue = adsEnvValue * ampReleaseEnv;
        }

        v *= ampEnvValue;

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
            if (v.currentMidiNote == -1) {
                oldestClosingIx = i;
                oldestClosingAge = std::numeric_limits<int>::max();
            } else if (v.ampEnvTicksSinceNoteOff >= oldestClosingAge) {
                oldestClosingIx = i;
                oldestClosingAge = v.ampEnvTicksSinceNoteOff;
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

    void Process(
        StateData* state, audio::EventsThisFrame const& frameEvents, int eventCount,
        float* outputBuffer, int const numChannels, int const framesPerBuffer, int const sampleRate) {

        float const dt = 1.0f / sampleRate;
        float const k = state->cutoffK;  // between [0,4], unstable at 4
        int const attackTimeInTicks = state->ampEnvAttackTime * sampleRate;
        int const decayTimeInTicks = state->ampEnvDecayTime * sampleRate;
        int const releaseTimeInTicks = state->ampEnvReleaseTime * sampleRate;
        
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
                            v->adsPhase = synth::ADSPhase::Attack;
                            v->ampEnvTicksSinceADSPhaseStart = 0;
                            v->ampEnvTicksSinceNoteOff = -1;
                            v->currentMidiNote = e.midiNote;
                        } else {
                            std::cout << "couldn't find a note for noteon" << std::endl;
                        }  
                        break;
                    }
                    case audio::EventType::NoteOff: {
                        Voice* v = FindVoiceForNoteOff(*state, e.midiNote);
                        if (v != nullptr) {
                            v->ampEnvTicksSinceNoteOff = 0;
                            // v->ampEnvTicksSinceStart = 0;
                            // v->ampEnvState = synth::AdsrState::Closing;
                        }                   
                        break;
                    }
                    case audio::EventType::SynthParam: {
                        switch (e.param) {
                            case audio::ParamType::Gain:
                                state->gainFactor = e.newParamValue;
                                break;
                            case audio::ParamType::Cutoff:
                                // value should be [0,1]. freq(0) = 0, freq(1) =
                                // sampleRate, but how do we scale it
                                // exponentially? so that 0.2 is twice the
                                // frequency of 0.1? Do we want that?
                                state->cutoffFreq = e.newParamValue * sampleRate;
                                break;
                            case audio::ParamType::Peak:
                                state->cutoffK = e.newParamValue * 4.f;
                                break;
                            case audio::ParamType::PitchLFOGain:
                                state->pitchLFOGain = e.newParamValue;
                                break;
                            case audio::ParamType::PitchLFOFreq:
                                state->pitchLFOFreq = e.newParamValue;
                                break;
                            case audio::ParamType::CutoffLFOGain:
                                state->cutoffLFOGain = e.newParamValue;
                                break;
                            case audio::ParamType::CutoffLFOFreq:
                                state->cutoffLFOFreq = e.newParamValue;
                                break;
                            case audio::ParamType::AmpEnvAttack:
                                state->ampEnvAttackTime = e.newParamValue;
                                break;
                            case audio::ParamType::AmpEnvDecay:
                                state->ampEnvDecayTime = e.newParamValue;
                                break;
                            case audio::ParamType::AmpEnvSustain:
                                state->ampEnvSustainLevel = e.newParamValue;
                                break;
                            case audio::ParamType::AmpEnvRelease:
                                state->ampEnvReleaseTime = e.newParamValue;
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
            
            float const pitchLFOValue = state->pitchLFOGain * sinf(state->pitchLFOPhase);
            state->pitchLFOPhase += (state->pitchLFOFreq * 2*kPi / sampleRate);

            // Get cutoff LFO value
            if (state->cutoffLFOPhase >= 2*kPi) {
                state->cutoffLFOPhase -= 2*kPi;
            }
            // Make LFO a sine wave for now.
            float const cutoffLFOValue = state->cutoffLFOGain * sinf(state->cutoffLFOPhase);
            state->cutoffLFOPhase += (state->cutoffLFOFreq * 2*kPi / sampleRate);
            float const modulatedCutoff = state->cutoffFreq * powf(2.0f, cutoffLFOValue);

            float v = 0.0f;
            for (Voice& voice : state->voices) {
                v += ProcessVoice(
                    voice, sampleRate, pitchLFOValue, modulatedCutoff, k, attackTimeInTicks, decayTimeInTicks,
                    state->ampEnvSustainLevel, releaseTimeInTicks);
            }

            // Apply final gain. Map from linear [0,1] to exponential from -80db to 0db.
            {
                float const startAmp = 0.01f;
                float const factor = 1.0f / startAmp;
                float gain = startAmp*powf(factor, state->gainFactor);
                v *= gain;
            }

            for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                *outputBuffer++ += v;    
            }
        }
    }
}