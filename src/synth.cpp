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

    void InitStateData(StateData& state) {
        state.left_phase = state.right_phase = 0.0f;
        state.f = 440.0f;
        state.lp0 = 0.0f;
        state.lp1 = 0.0f;
        state.lp2 = 0.0f;
        state.lp3 = 0.0f;
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
                if (fe._sampleIx == i) {
                    audio::Event const& e = fe._e;
                    switch (e.type) {
                        case audio::EventType::NoteOn: {
                            state->f = synth::MidiToFreq(e.midiNote);
                            state->ampEnvTicksSinceStart = 0;
                            state->ampEnvState = synth::AdsrState::Opening;
                            break;
                        }
                        case audio::EventType::NoteOff: {
                            // TODO: ignoring the note. assuming monophony for now
                            state->ampEnvTicksSinceStart = 0;
                            state->ampEnvState = synth::AdsrState::Closing;                
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    ++frameEventIx;
                } else {
                    break;
                }
            }

            // Get pitch LFO value
            if (state->pitchLFOPhase >= 2*kPi) {
                state->pitchLFOPhase -= 2*kPi;
            }
            // Make LFO a sine wave for now.
            
            float const pitchLFOValue = state->pitchLFOGain * sinf(state->pitchLFOPhase);
            state->pitchLFOPhase += (state->pitchLFOFreq * 2*kPi / sampleRate);

            // Now use the LFO value to get a new frequency.
            float const modulatedF = state->f * powf(2.0f, pitchLFOValue);

            // use left phase for both channels for now.
            if (state->left_phase >= 2*kPi) {
                state->left_phase -= 2*kPi;
            }

            float phaseChange = 2*kPi*modulatedF / sampleRate;

            float v = 0.0f;
            {
                // v = GenerateSquare(state->left_phase, phaseChange);
                v = GenerateSaw(state->left_phase, phaseChange);
                state->left_phase += phaseChange;
            }

            // Get cutoff LFO value
            if (state->cutoffLFOPhase >= 2*kPi) {
                state->cutoffLFOPhase -= 2*kPi;
            }
            // Make LFO a sine wave for now.
            float const cutoffLFOValue = state->cutoffLFOGain * sinf(state->cutoffLFOPhase);
            state->cutoffLFOPhase += (state->cutoffLFOFreq * 2*kPi / sampleRate);

            float const modulatedCutoff = state->cutoffFreq * powf(2.0f, cutoffLFOValue);

            // ladder filter
            // TODO: should we put this in the oversampling?
            float const rc = 1 / modulatedCutoff;
            float const a = dt / (rc + dt);
            v -= k*state->lp3;

            state->lp0 = a*v + (1-a)*state->lp0;
            state->lp1 = a*(state->lp0) + (1-a)*state->lp1;
            state->lp2 = a*(state->lp1) + (1-a)*state->lp2;
            state->lp3 = a*(state->lp2) + (1-a)*state->lp3;
            v = state->lp3;

            // Amplitude envelope
            float ampEnvValue = 0.0f;
            switch (state->ampEnvState) {
                case AdsrState::Closed: break;
                case AdsrState::Opening: {
                    if (state->ampEnvTicksSinceStart < attackTimeInTicks) {
                        // attack phase
                        float t;
                        if (attackTimeInTicks == 0) {
                            t = 1.0f;
                        } else {
                            t = fmin(1.0f, (float) state->ampEnvTicksSinceStart / (float) attackTimeInTicks);
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
                            int ticksSinceDecayStart = state->ampEnvTicksSinceStart - attackTimeInTicks;
                            t = fmin(1.0f, (float) ticksSinceDecayStart / (float) decayTimeInTicks);
                        }                        
                        float const sustain = fmax(kSmallAmplitude, state->ampEnvSustainLevel);
                        ampEnvValue = 1.0f*powf(sustain, t);
                    }
                    state->lastNoteOnAmpEnvValue = ampEnvValue;
                    break;
                }
                case AdsrState::Closing: {
                    // release phase. release time defined as how long it takes
                    // to get from value just before release down to -80db or
                    // w/e.
                    if (state->ampEnvTicksSinceStart > releaseTimeInTicks) {
                        state->ampEnvState = AdsrState::Closed;
                        break;
                    }
                    float t;
                    if (releaseTimeInTicks == 0) {
                        t = 1.0f;
                    } else {
                        t = fmin(1.0f, (float) state->ampEnvTicksSinceStart / (float) releaseTimeInTicks);
                    }
                    ampEnvValue = state->lastNoteOnAmpEnvValue*powf(kSmallAmplitude / state->lastNoteOnAmpEnvValue, t);                    
                    break;
                }
            }
            ++state->ampEnvTicksSinceStart;

            v *= ampEnvValue;

            for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                *outputBuffer++ += v;    
            }
        }
    }
}