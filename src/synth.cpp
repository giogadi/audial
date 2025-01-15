#include "synth.h"

#include <iostream>

#include "audio_util.h"
#include "constants.h"
#include "math_util.h"
#include "rng.h"

using audio::SynthParamType;

#define POLYBLEP 1

namespace synth {
namespace {

int constexpr kSamplesPerCutoffEnvModulate = 64;

#if POLYBLEP
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
#endif

float GenerateSquare(float const phase, float const phaseChange) {
    float v = 0.0f;
    if (phase < kPi) {
        v = 1.0f;
    } else {
        v = -1.0f;
    }
#if POLYBLEP
    // polyblep
     float dt = phaseChange * k1_2Pi;
     float t = phase * k1_2Pi;
     v += Polyblep(t, dt);

     //v -= Polyblep(fmod(t + 0.5f, 1.0f), dt);

     /*float unused;
     float tFrac = modf(t + 0.5f, &unused);
     v -= Polyblep(tFrac, dt);*/

     float tFrac = t + 0.5f;
     if (tFrac >= 1.f) {
         tFrac -= 1.f;
     }
#endif
    return v;
}

float GenerateSaw(float const phase, float const phaseChange) {
    float v = (phase * k1_Pi) - 1.0f;
#if POLYBLEP
    // polyblep
     float dt = phaseChange * k1_2Pi;
     float t = phase * k1_2Pi;
     v -= Polyblep(t, dt);
#endif
    return v;
}

float GenerateNoise(rng::State& rng) {
    return rng::GetFloat(rng, -1.f, 1.f);
}

int constexpr kDelayBufferCount = 2 * 65536;
}  // namespace

void InitStateData(StateData& state, int channel, int const sampleRate, int const framesPerBuffer, int const numBufferChannels) {
    state = StateData();
    state.channel = channel;
    state.voiceScratchBuffer = new float[framesPerBuffer * numBufferChannels];
    state.synthScratchBuffer = new float[framesPerBuffer * numBufferChannels];
    state.sampleRate = sampleRate;
    state.framesPerBuffer = framesPerBuffer;
    
    state.samplesPerMoogCutoffUpdate = std::min(framesPerBuffer, kSamplesPerCutoffEnvModulate);

    state.delayBuffer = new float[kDelayBufferCount];
    memset(state.delayBuffer, 0, kDelayBufferCount * sizeof(float));
    state.delayBufferWriteIx = 0;

    for (Voice& v : state.voices) {
        for (uint64_t ii = 0; ii < v.oscillators.size(); ++ii) {
            for (int jj = 0; jj < kMaxUnison; ++jj) {
                v.oscillators[jj].phases[jj] = 0.f;
            }
            rng::Seed(v.oscillators[ii].rng, 1234871 + ii);
        }
        v.moogLpfState.reset((float)sampleRate);
    }
}

void DestroyStateData(StateData& state) {
    delete[] state.voiceScratchBuffer;
    delete[] state.synthScratchBuffer;
    delete[] state.delayBuffer;
    state.voiceScratchBuffer = nullptr;
    state.synthScratchBuffer = nullptr;
    state.delayBuffer = nullptr;
}

float calcMultiplier(float startLevel, float endLevel, int64_t numSamples) {
    assert(numSamples > 0l);
    if (startLevel <= 0.f) {
        startLevel = 0.001f;
    }
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
            state->value = spec.attackOffset + state->value*spec.attackCoeff;
            if (state->value >= 1.f || spec.attackTime <= 0) {
                state->value = 1.f;
                state->phase = ADSRPhase::Decay;
            }
            break;
        }
        case ADSRPhase::Decay: {
            state->value = spec.decayOffset + state->value * spec.decayCoeff;
            if (state->value <= spec.sustainLevel || spec.decayTime <= 0) {
                state->value = spec.sustainLevel;
                state->phase = ADSRPhase::Sustain;
            }
            break;
        }
        case ADSRPhase::Sustain: {
            state->value = spec.sustainLevel;
            break;
        }
        case ADSRPhase::Release: {
            state->value = spec.releaseOffset + state->value * spec.releaseCoeff;
            if (state->value <= 0.f || spec.releaseTime <= 0) {
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

void adsrEnvelope(ADSREnvSpecInTicks const& spec, int const framesPerBuffer, ADSREnvState& state) {
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
            state.ticksSincePhaseStart += framesPerBuffer;
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
                    state.multiplier = calcMultiplier(1.f, spec.sustainLevel, spec.decayTime / framesPerBuffer);
                }
            }
            state.currentValue *= state.multiplier;
            state.ticksSincePhaseStart += framesPerBuffer;
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
                    state.multiplier = calcMultiplier(spec.sustainLevel, spec.minValue, spec.releaseTime / framesPerBuffer);
                }
            }
            state.currentValue *= state.multiplier;
            state.ticksSincePhaseStart += framesPerBuffer;
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

// NOTE: This assumes oscFaderGains's size is the same as kNumAnalogOscillators.
void ProcessVoice(Voice& voice, int const sampleRate, float pitchLFOValue,
    float modulatedCutoff, ADSREnvSpecInternal const& ampEnvSpec,
    ADSREnvSpecInternal const& cutoffEnvSpec,
    ADSREnvSpecInTicks const& pitchEnvSpec,
    Patch const& patch, float* outputBuffer, int const numChannels, int const framesPerBuffer, int const samplesPerMoogCutoffUpdate,
    float const* oscFaderGains) {
    float const dt = 1.f / sampleRate;

    // portamento
    if (voice.postPortamentoF <= 0.f) {
        voice.postPortamentoF = voice.oscillators[0].f;
    }

    float const framesPerOctave = patch.Get(SynthParamType::Portamento) * sampleRate / static_cast<float>(framesPerBuffer);
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
    adsrEnvelope(pitchEnvSpec, framesPerBuffer, voice.pitchEnvState);
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
    
    int unisonLevels = static_cast<int>(patch.Get(audio::SynthParamType::Unison));
    unisonLevels = std::min(unisonLevels, (kMaxUnison - 1) / 2);
    int const unison = 2 * unisonLevels + 1;
    float detuneCents = patch.Get(audio::SynthParamType::UnisonDetune);
    for (int oscIx = 0; oscIx < kNumAnalogOscillators; ++oscIx) {
        Oscillator& osc = voice.oscillators[oscIx];
        float oscF = modulatedF;
        if (oscIx > 0) {
            oscF = modulatedF * powf(2.f, patch.Get(SynthParamType::Detune));
        }

        float freqs[kMaxUnison];
        freqs[0] = oscF;
        float unisonGain = 1.f;
        if (unisonLevels > 0) {
            unisonGain = sqrt(1.f / unison);
            float dCents = detuneCents / unisonLevels;
            float detuneLevelCents = 0.f;
            int detuneIx = 1;
            for (int ii = 0; ii < unisonLevels; ++ii) {
                detuneLevelCents += dCents;
                float ratio = std::powf(2.f, detuneLevelCents / 1200.f);
                freqs[detuneIx] = oscF * ratio;
                freqs[detuneIx + 1] = oscF / ratio;
                detuneIx += 2;
            }
        }
        
        float oscGain = oscFaderGains[oscIx];
        float phaseChanges[kMaxUnison];
        for (int ii = 0; ii < unison; ++ii) {
            phaseChanges[ii] = k2Pi * freqs[ii] / sampleRate;
        }
        Waveform const waveform = (oscIx == 0) ? patch.GetOsc1Waveform() : patch.GetOsc2Waveform();
        switch (waveform) {
            case Waveform::Saw: {
                int outputIx = 0;                
                for (int sampleIx = 0; sampleIx < framesPerBuffer; ++sampleIx) {
                    float oscV = 0.f;
                    for (int ii = 0; ii < unison; ++ii) {
                        if (osc.phases[ii] >= k2Pi) {
                            osc.phases[ii] -= k2Pi;
                        }
                        float v = GenerateSaw(osc.phases[ii], phaseChanges[ii]);
                        oscV += unisonGain * v;                        

                        osc.phases[ii] += phaseChanges[ii];
                    }
                    float oscWithGain = oscV * oscGain;
                    for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                        outputBuffer[outputIx++] += oscWithGain;
                    }
                }
                break;
            }
            case Waveform::Square: {
                int outputIx = 0;
                for (int sampleIx = 0; sampleIx < framesPerBuffer; ++sampleIx) {
                    float oscV = 0.f;
                    for (int ii = 0; ii < unison; ++ii) {
                        if (osc.phases[ii] >= k2Pi) {
                            osc.phases[ii] -= k2Pi;
                        }
                        float v = GenerateSquare(osc.phases[ii], phaseChanges[ii]);
                        oscV += unisonGain * v;

                        osc.phases[ii] += phaseChanges[ii];
                    }
                    float oscWithGain = oscV * oscGain;
                    for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                        outputBuffer[outputIx++] += oscWithGain;
                    }
                }
                break;
            }
            case Waveform::Noise: {
                int outputIx = 0;
                for (int sampleIx = 0; sampleIx < framesPerBuffer; ++sampleIx) {
                    float oscV = GenerateNoise(osc.rng);
                    oscV *= oscGain;
                    for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                        outputBuffer[outputIx++] += oscV;
                    }
                }
                break;
            }
            case Waveform::Count: {
                assert(false);
                break;
            }
        }
    }

    // Apply filters and some more gains
    {
        int outputIx = 0;
        int cutoffModulateCounter = 0;
        float gainFactor = voice.velocity * gain;
        for (int sampleIx = 0; sampleIx < framesPerBuffer; ++sampleIx) {
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
            v *= gainFactor;

            for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
                outputBuffer[outputIx++] = v;
            }
        }
    }
}

void ProcessFmVoice(Voice& voice, int const sampleRate, float pitchLFOValue,
    float modulatedCutoff, ADSREnvSpecInternal const& ampEnvSpec,
    ADSREnvSpecInternal const& cutoffEnvSpec,
    ADSREnvSpecInTicks const& pitchEnvSpec,
    Patch const& patch, float* outputBuffer, int const numChannels, int const framesPerBuffer, int const samplesPerMoogCutoffUpdate) {
    /*
    float const dt = 1.f / sampleRate;

    // portamento
    
    if (voice.postPortamentoF <= 0.f) {
        voice.postPortamentoF = voice.oscillators[0].f;
    }

    float const framesPerOctave = patch.Get(SynthParamType::Portamento) * sampleRate / static_cast<float>(samplesPerFrame);
    if (framesPerOctave <= 0.f) {
        voice.postPortamentoF = voice.oscillators[0].f;
    } else {
        float const kPortamentoFactor = powf(2.f, 1.f / framesPerOctave);
        if (voice.postPortamentoF > voice.oscillators[0].f) {
            voice.postPortamentoF /= kPortamentoFactor;
            voice.postPortamentoF = std::max(voice.oscillators[0].f, voice.postPortamentoF);
        } else if (voice.postPortamentoF < voice.oscillators[0].f) {
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
    float modulatedF = voice.oscillators[0].f;
    */

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
    //float hpfA1, hpfA2, hpfA3, hpfK;  // filter shit
    //{
    //    float res = patch.Get(SynthParamType::HpfPeak) / 4.f;
    //    float g = tan(kPi * patch.Get(SynthParamType::HpfCutoff) * dt);
    //    hpfK = 2 - 2 * res;
    //    assert((1 + g * (g + hpfK)) != 0.f);
    //    hpfA1 = 1 / (1 + g * (g + hpfK));
    //    hpfA2 = g * hpfA1;
    //    hpfA3 = g * hpfA2;
    //}

    /*
    for (int oscIx = 0; oscIx < kNumAnalogOscillators; ++oscIx) {
        Oscillator& osc = voice.oscillators[oscIx];
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
    */

    {
        Oscillator& osc = voice.oscillators[1];
        float level = patch.Get(audio::SynthParamType::FMOsc2Level);
        float ratio = patch.Get(audio::SynthParamType::FMOsc2Ratio);
        float oscF = voice.oscillators[0].f * ratio;
        float phaseChange = 2 * kPi * oscF / sampleRate;
        int outputIx = 0;
        for (int sampleIx = 0; sampleIx < framesPerBuffer; ++sampleIx) {
            // modulator outputs values that will then be interpreted as phase offsets in the next pass
            if (osc.phases[0] >= k2Pi) {
                osc.phases[0] -= k2Pi;
            }
            float oscV = sin(osc.phases[0]);
            oscV *= level;
            outputBuffer[outputIx] = oscV;
            outputIx += numChannels;

            osc.phases[0] += phaseChange;
        }
    }    

    Oscillator& osc = voice.oscillators[0];
    float oscF = osc.f;
    float phaseChange = 2 * kPi * oscF / sampleRate;
    int outputIx = 0;
    for (int sampleIx = 0; sampleIx < framesPerBuffer; ++sampleIx) {
        if (osc.phases[0] >= 2 * kPi) {
            osc.phases[0] -= 2 * kPi;
        }

        AdsrTick(ampEnvSpec, &voice.ampEnvState);
        if (voice.ampEnvState.phase == ADSRPhase::Closed) {
            voice.currentMidiNote = -1;
        }

        float phaseOffset = outputBuffer[outputIx];
        float phase = osc.phases[0] + phaseOffset;

        float oscV = sin(phase);
        oscV *= gain;
        oscV *= voice.velocity;
        oscV *= voice.ampEnvState.value;
        for (int channelIx = 0; channelIx < numChannels; ++channelIx) {
            outputBuffer[outputIx++] = oscV;
        }

        osc.phases[0] += phaseChange;
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
    std::pair<int, int64_t> bestPhaseAgePair = std::make_pair(-1, -1);
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
    specInternal.modulationHz = (int64_t)(sampleRate / samplesPerModulation);
    specInternal.attackTime = (int64_t)(spec.attackTime * sampleRate / samplesPerModulation);
    specInternal.decayTime = (int64_t)(spec.decayTime * sampleRate / samplesPerModulation);
    specInternal.sustainLevel = spec.sustainLevel;
    specInternal.releaseTime = (int64_t)(spec.releaseTime * sampleRate / samplesPerModulation);

    specInternal.attackTCO = exp(-1.5f);
    specInternal.decayTCO = exp(-4.95f);
    specInternal.releaseTCO = specInternal.decayTCO;

    // TODO: only update each of these if the value changed
    specInternal.attackCoeff = exp(-log((1.f + specInternal.attackTCO) / specInternal.attackTCO) / specInternal.attackTime);
    specInternal.attackOffset = (1.f + specInternal.attackTCO)*(1.f - specInternal.attackCoeff);

    specInternal.decayCoeff = exp(-log((1.f + specInternal.decayTCO) / specInternal.decayTCO) / specInternal.decayTime);
    specInternal.decayOffset = (specInternal.sustainLevel - specInternal.decayTCO)*(1.f - specInternal.decayCoeff);

    specInternal.releaseCoeff = exp(-log((1.f + specInternal.releaseTCO) / specInternal.releaseTCO) / specInternal.releaseTime);
    specInternal.releaseOffset = -specInternal.releaseTCO*(1.f - specInternal.releaseCoeff);
}

// "Samples" is simply the number of iterations we plan to run this envelope. In some cases it might actually be number of samples;
// in other cases, it might be number of buffers; or something in between. It depends on how the caller updates the envelope.
void ADSRNoteOn(ADSREnvSpecInternal const& spec, ADSRStateNew& adsrState) {
    adsrState.ticksSincePhaseStart = 0;
    adsrState.phase = synth::ADSRPhase::Attack;
}

void NoteOn(StateData& state, int midiNote, float velocity, int noteOnId, int primePortaMidiNote) {
    Voice* v = FindVoiceForNoteOn(state, midiNote);
    if (v != nullptr) {
        v->oscillators[0].f = synth::MidiToFreq(midiNote);
        /*for (int oscIx = 1; oscIx < v->oscillators.size(); ++oscIx) {
            Oscillator& osc = v->oscillators[oscIx];
            osc.phase = v->oscillators[0].phase;
        }*/
        v->currentMidiNote = midiNote;
        v->velocity = velocity;
        v->noteOnId = noteOnId;

        if (primePortaMidiNote >= 0) {
            float f = MidiToFreq(primePortaMidiNote);
            v->postPortamentoF = f;
        }
        
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
        ConvertADSREnvSpec(state.patch.GetAmpEnvSpec(), state.ampEnvSpecInternal, (float) state.sampleRate, 1);
        break;
    case audio::SynthParamType::CutoffEnvAttack:
    case audio::SynthParamType::CutoffEnvDecay:
    case audio::SynthParamType::CutoffEnvSustain:
    case audio::SynthParamType::CutoffEnvRelease:
        ConvertADSREnvSpec(state.patch.GetCutoffEnvSpec(), state.cutoffEnvSpecInternal, (float) state.sampleRate, state.samplesPerMoogCutoffUpdate);
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

void Process(StateData* state, audio::PendingEvent *eventsThisBuffer, int eventsThisBufferCount,
    float* outputBuffer, int const numChannels, int const framesPerBuffer,
    int const sampleRate, int64_t currentBufferCounter) {
    
    Patch& patch = state->patch;

    // Handle all the events that will happen in this buffer frame.
    int64_t bufferStartTickTime = currentBufferCounter * framesPerBuffer;
    for (int eventIx = 0; eventIx < eventsThisBufferCount; ++eventIx) {
        audio::Event const& e = eventsThisBuffer[eventIx]._e; 
        if (e.channel != state->channel) {
            // Not meant for this channel. Skip this message.
            continue;
        }
        switch (e.type) {
            case audio::EventType::NoteOn: {
                NoteOn(*state, e.midiNote, e.velocity, e.noteOnId, e.primePortaMidiNote);
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
                // Ensure that there's no existing automation on this param.
                // TODO: maybe have a "clear all automations" event that gets triggered by ChangePatch to avoid doing this for every param?
                for (Automation& a : state->automations) {
                    if (a._active && a._synthParamType == e.param) {
                        a._active = false;
                    }
                }
                if (e.paramChangeTimeSecs > 0.0) {
                    // Find a free automation.                    
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
                    pA->_startTickTime = bufferStartTickTime;
                    int64_t changeTimeInTicks = (int64_t) (e.paramChangeTimeSecs * sampleRate);
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
        if (bufferStartTickTime > a._endTickTime) {
            patch.Get(a._synthParamType) = a._desiredValue;
            a._active = false;
            continue;
        }
        double totalTime = (double) (a._endTickTime - a._startTickTime);
        assert(totalTime != 0.0);
        double timeSoFar = (double) (bufferStartTickTime - a._startTickTime);
        double factor = std::min(timeSoFar / totalTime, 1.0);
        float& currentValue = patch.Get(a._synthParamType);
        float newValue = -1.f;
        if (a._synthParamType == audio::SynthParamType::Cutoff || a._synthParamType == audio::SynthParamType::CutoffEnvGain) {
            if (factor != 0.0) {
                factor = std::pow(2, 10.0 * factor - 10.0);
            }
        }
        newValue = a._startValue + factor * (a._desiredValue - a._startValue);
        currentValue = newValue;

        for (Voice& v : state->voices) {
            OnParamChange(*state, a._synthParamType, newValue, &v);
        }
    }

    // Get pitch LFO value
    if (state->pitchLFOPhase >= k2Pi) {
        state->pitchLFOPhase -= k2Pi;
    }
    // Make LFO a sine wave.
    float const pitchLFOValue = patch.Get(SynthParamType::PitchLFOGain) * sinf(state->pitchLFOPhase);
    state->pitchLFOPhase += (patch.Get(SynthParamType::PitchLFOFreq) * 2 * kPi * framesPerBuffer / sampleRate);

    // Get cutoff LFO value
    if (state->cutoffLFOPhase >= k2Pi) {
        state->cutoffLFOPhase -= k2Pi;
    }
    // Make LFO a sine wave for now.
    float const cutoffLFOValue = patch.Get(SynthParamType::CutoffLFOGain) * sinf(state->cutoffLFOPhase);
    state->cutoffLFOPhase += (patch.Get(SynthParamType::CutoffLFOFreq) * 2 * kPi * framesPerBuffer / sampleRate);
    // float const modulatedCutoff = patch.cutoffFreq * powf(2.0f, cutoffLFOValue);
    float const modulatedCutoff = math_util::Clamp(patch.Get(SynthParamType::Cutoff) + 10000 * cutoffLFOValue, 0.f, 20000.f);

    ADSREnvSpecInTicks pitchEnvSpec;
    ConvertADSREnvSpec(patch.GetPitchEnvSpec(), pitchEnvSpec, sampleRate);

    // zero out the synth scratch buffer
    memset(state->synthScratchBuffer, 0, numChannels * framesPerBuffer * sizeof(float));

    if (patch.GetIsFm()) {
        for (Voice& voice : state->voices) {
            // zero out the voice scratch buffer.
            memset(state->voiceScratchBuffer, 0, numChannels * framesPerBuffer * sizeof(float));
            ProcessFmVoice(voice, sampleRate, pitchLFOValue, modulatedCutoff, state->ampEnvSpecInternal, state->cutoffEnvSpecInternal, pitchEnvSpec, patch, state->voiceScratchBuffer, numChannels, framesPerBuffer, state->samplesPerMoogCutoffUpdate);
            for (int outputIx = 0; outputIx < numChannels * framesPerBuffer; ++outputIx) {
                state->synthScratchBuffer[outputIx] += state->voiceScratchBuffer[outputIx];
            }
        }
    } else {
        float oscFaderGains[kNumAnalogOscillators];
        static_assert(kNumAnalogOscillators == 2);
        oscFaderGains[0] = sqrt(1.f - patch.Get(SynthParamType::OscFader));
        oscFaderGains[1] = sqrt(patch.Get(SynthParamType::OscFader));               

        for (Voice& voice : state->voices) {
            // zero out the voice scratch buffer.
            memset(state->voiceScratchBuffer, 0, numChannels * framesPerBuffer * sizeof(float));
            ProcessVoice(voice, sampleRate, pitchLFOValue, modulatedCutoff, state->ampEnvSpecInternal, state->cutoffEnvSpecInternal, pitchEnvSpec, patch, state->voiceScratchBuffer, numChannels, framesPerBuffer, state->samplesPerMoogCutoffUpdate, oscFaderGains);
            for (int outputIx = 0; outputIx < numChannels * framesPerBuffer; ++outputIx) {
                state->synthScratchBuffer[outputIx] += state->voiceScratchBuffer[outputIx];
            }     
        }        
    }

    // DELAY
    float const delayTime = math_util::Clamp(patch.Get(SynthParamType::DelayTime), 0.001f, 0.5f);
    float const delayGain = math_util::Clamp(patch.Get(SynthParamType::DelayGain), 0.f, 1.f);
    float const delayFeedback = std::min(1.f, patch.Get(SynthParamType::DelayFeedback));
    int const delaySamples = static_cast<int>(delayTime * sampleRate);
    int delayBufferReadIx = state->delayBufferWriteIx - (numChannels * delaySamples);
    if (delayBufferReadIx < 0) {
        delayBufferReadIx = kDelayBufferCount + delayBufferReadIx;
    }
    assert(delayBufferReadIx >= 0);
    assert(delayBufferReadIx < kDelayBufferCount);
    {
        int writeIx = state->delayBufferWriteIx;
        for (int outputIx = 0; outputIx < numChannels * framesPerBuffer; ++outputIx) {
            if (writeIx >= kDelayBufferCount) {
                writeIx = 0;
            }
            state->delayBuffer[writeIx] = 0.f;
            ++writeIx;
        }
    }
    if (delayGain == 0.f) {
        for (int outputIx = 0; outputIx < numChannels * framesPerBuffer; ++outputIx) {
            outputBuffer[outputIx] += state->synthScratchBuffer[outputIx];
        }
    } else {
        int writeIx = state->delayBufferWriteIx;
        int readIx = delayBufferReadIx;
        for (int outputIx = 0; outputIx < numChannels * framesPerBuffer; ++outputIx) {
            if (writeIx >= kDelayBufferCount) {
                writeIx = 0;
            }
            if (readIx >= kDelayBufferCount) {
                readIx = 0;
            }
            state->delayBuffer[writeIx] += state->synthScratchBuffer[outputIx] + delayFeedback * state->delayBuffer[readIx];
            outputBuffer[outputIx] += state->synthScratchBuffer[outputIx] + delayGain * state->delayBuffer[readIx];
            ++writeIx;
            ++readIx;
        }
    }

    state->delayBufferWriteIx += numChannels * framesPerBuffer;
    if (state->delayBufferWriteIx >= kDelayBufferCount) {
        state->delayBufferWriteIx = 0;
    }

}

}
