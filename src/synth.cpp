#include "synth.h"

#include <iostream>

#include "audio_util.h"

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
        float ampEnvValue = 0.0f;
        switch (voice.ampEnvState) {
            case AdsrState::Closed: break;
            case AdsrState::Opening: {
                if (voice.ampEnvTicksSinceStart < attackTimeInTicks) {
                    // attack phase
                    float t;
                    if (attackTimeInTicks == 0) {
                        t = 1.0f;
                    } else {
                        t = fmin(1.0f, (float) voice.ampEnvTicksSinceStart / (float) attackTimeInTicks);
                    }
                    float const startAmp = kSmallAmplitude;
                    float const factor = 1.0f / startAmp;
                    ampEnvValue = startAmp*powf(factor, t);
                } else {
                    // decay phase
                    float t;
                    if (decayTimeInTicks == 0) {
                        t = 1.0f;
                    } else {
                        int ticksSinceDecayStart = voice.ampEnvTicksSinceStart - attackTimeInTicks;
                        t = fmin(1.0f, (float) ticksSinceDecayStart / (float) decayTimeInTicks);
                    }                        
                    float const sustain = fmax(kSmallAmplitude, ampEnvSustainLevel);
                    ampEnvValue = 1.0f*powf(sustain, t);
                }
                voice.lastNoteOnAmpEnvValue = ampEnvValue;
                break;
            }
            case AdsrState::Closing: {
                // release phase. release time defined as how long it takes
                // to get from value just before release down to -80db or
                // w/e.
                if (voice.ampEnvTicksSinceStart > releaseTimeInTicks) {
                    voice.ampEnvState = AdsrState::Closed;
                    break;
                }
                float t;
                if (releaseTimeInTicks == 0) {
                    t = 1.0f;
                } else {
                    t = fmin(1.0f, (float) voice.ampEnvTicksSinceStart / (float) releaseTimeInTicks);
                }
                ampEnvValue = voice.lastNoteOnAmpEnvValue*powf(kSmallAmplitude / voice.lastNoteOnAmpEnvValue, t);                    
                break;
            }
        }
        ++voice.ampEnvTicksSinceStart;

        v *= ampEnvValue;

        return v;
    }

    Voice* FindVoiceForNoteOn(StateData& state) {
        // Find closing notes.
        int oldestClosingIx = -1;
        int oldestClosingAge = -1;
        for (int i = 0; i < state.voices.size(); ++i) {
            Voice& v = state.voices[i];
            if (v.ampEnvState == AdsrState::Closed) {
                return &v;
            }
            if (v.ampEnvState == AdsrState::Closing && v.ampEnvTicksSinceStart > oldestClosingAge) {
                oldestClosingIx = i;
                oldestClosingAge = v.ampEnvTicksSinceStart;
            }
        }
        if (oldestClosingIx >= 0) {
            return &state.voices[oldestClosingIx];
        }
        return nullptr;
    }

    Voice* FindVoiceForNoteOff(int midiNote, StateData& state) {
        for (Voice& v : state.voices) {
            if (v.ampEnvState == AdsrState::Opening && v.lastMidiNote == midiNote) {
                return &v;
            }
        }
        return nullptr;
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
                        Voice* v = FindVoiceForNoteOn(*state);
                        if (v != nullptr) {
                            v->f = synth::MidiToFreq(e.midiNote);
                            v->ampEnvTicksSinceStart = 0;
                            v->ampEnvState = synth::AdsrState::Opening;
                            v->lastMidiNote = e.midiNote;
                        } else {
                            std::cout << "couldn't find a note for noteon" << std::endl;
                        }  
                        break;
                    }
                    case audio::EventType::NoteOff: {
                        Voice* v = FindVoiceForNoteOff(e.midiNote, *state);
                        if (v != nullptr) {
                            v->ampEnvTicksSinceStart = 0;
                            v->ampEnvState = synth::AdsrState::Closing;
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

            for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                *outputBuffer++ += v;    
            }
        }
    }
}