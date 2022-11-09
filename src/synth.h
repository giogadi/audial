#pragma once

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>

#include "boost/circular_buffer.hpp"
#include "boost/property_tree/ptree.hpp"

#include "audio_util.h"
#include "serial.h"
#include "enums/synth_Waveform.h"

using boost::property_tree::ptree;

namespace synth {
    static inline float const kSemitoneRatio = 1.05946309f;
    static inline float const kFifthRatio = 1.49830703f;

    static inline float const kSmallAmplitude = 0.0001f;

    // TODO: use a lookup table!!!
    inline float MidiToFreq(int midi) {
        int const a4 = 69;  // 440Hz
        return 440.0f * pow(2.0f, (midi - a4) / 12.0f);
    }

    enum class ADSRPhase {
        Closed, Attack, Decay, Sustain, Release
    };

    struct ADSREnvState {
        ADSRPhase phase = ADSRPhase::Closed;
        float currentValue = 0.f;
        float multiplier = 0.f;
        long ticksSincePhaseStart = -1;
    };

    struct ADSREnvSpec {
        float attackTime = 0.f;
        float decayTime = 0.f;
        float sustainLevel = 1.f;
        float releaseTime = 0.f;
        float minValue = 0.01f;

        void Save(ptree& pt) const {
            pt.put("attack_time", attackTime);
            pt.put("decay_time", decayTime);
            pt.put("sustain_level", sustainLevel);
            pt.put("release_time", releaseTime);
            pt.put("min_value", minValue);
        }
        void Load(ptree const& pt) {
            attackTime = pt.get<float>("attack_time");
            decayTime = pt.get<float>("decay_time");
            sustainLevel = pt.get<float>("sustain_level");
            releaseTime = pt.get<float>("release_time");
            minValue = pt.get<float>("min_value");
        }
    };

    struct Patch {
        std::string name;
        // This is interpreted linearly from [0,1]. This will get mapped later to [-80db,0db].
        // TODO: consider just having this be a decibel value capped at 0db.
        float gainFactor = 0.7f;

        Waveform osc1Waveform = Waveform::Square;
        Waveform osc2Waveform = Waveform::Square;

        float detune = 0.f;   // 1.0f is a whole octave.
        float oscFader = 0.5f;  // 0.f is fully Osc1, 1.f is fully Osc2.

        float cutoffFreq = 44100.0f;
        float cutoffK = 0.0f;  // [0,4] but 4 is unstable

        // gain of 1.0 coincides with the wave varying across 2 octaves (one octave up and one octave down).
        float pitchLFOGain = 0.0f;
        float pitchLFOFreq = 0.0f;

        // gain of 1.0 coincides with cutoff frequency doubling at LFO peak and halving at LFO valley.
        float cutoffLFOGain = 0.0f;
        float cutoffLFOFreq = 0.0f;

        ADSREnvSpec ampEnvSpec;

        ADSREnvSpec cutoffEnvSpec;
        float cutoffEnvGain = 0.f;

	ADSREnvSpec pitchEnvSpec;
	float pitchEnvGain = 0.f;

        void Save(ptree& pt) const {
            pt.put("version", kVersion);
            pt.put("name", name);
            pt.put("gain_factor", gainFactor);
            pt.put("osc1_waveform", WaveformToString(osc1Waveform));
            pt.put("osc2_waveform", WaveformToString(osc2Waveform));
            pt.put("detune", detune);
            pt.put("osc_fader", oscFader);
            pt.put("cutoff_freq", cutoffFreq);
            pt.put("cutoff_k", cutoffK);
            pt.put("pitch_lfo_gain", pitchLFOGain);
            pt.put("pitch_lfo_freq", pitchLFOFreq);
            pt.put("cutoff_lfo_gain", cutoffLFOGain);
            pt.put("cutoff_lfo_freq", cutoffLFOFreq);
            {
                ptree& newChild = pt.add_child("amp_env_spec", ptree());
                ampEnvSpec.Save(newChild);
            }
            pt.put("cutoff_env_gain", cutoffEnvGain);
            {
                ptree& newChild = pt.add_child("cutoff_env_spec", ptree());
                cutoffEnvSpec.Save(newChild);
            }
	    pt.put("pitch_env_gain", pitchEnvGain);
	    {
		ptree& newChild = pt.add_child("pitch_env_spec", ptree());
                pitchEnvSpec.Save(newChild);
	    }
        }
        void Load(ptree const& pt) {
            int const version = pt.get_optional<int>("version").value_or(0);

            name = pt.get<std::string>("name");
            gainFactor = pt.get<float>("gain_factor");
            if (version >= 1) {
                osc1Waveform = StringToWaveform(pt.get<std::string>("osc1_waveform").c_str());
                osc2Waveform = StringToWaveform(pt.get<std::string>("osc2_waveform").c_str());
                detune = pt.get<float>("detune");
                oscFader = pt.get<float>("osc_fader");
            }
            cutoffFreq = pt.get<float>("cutoff_freq");
            cutoffK = pt.get<float>("cutoff_k");
            pitchLFOGain = pt.get<float>("pitch_lfo_gain");
            pitchLFOFreq = pt.get<float>("pitch_lfo_freq");
            cutoffLFOGain = pt.get<float>("cutoff_lfo_gain");
            cutoffLFOFreq = pt.get<float>("cutoff_lfo_freq");
            ampEnvSpec.Load(pt.get_child("amp_env_spec"));
            cutoffEnvGain = pt.get<float>("cutoff_env_gain");
            cutoffEnvSpec.Load(pt.get_child("cutoff_env_spec"));
	    if (version >= 2) {
		pitchEnvGain = pt.get<float>("pitch_env_gain");
		pitchEnvSpec.Load(pt.get_child("pitch_env_spec"));
	    }
        }

        static int constexpr kVersion = 2;
    };

    int constexpr kNumOscillators = 2;

    struct Oscillator {
        float f = 440.f;
        float phase = 0.f;
    };

    struct Voice {
        // We assign the voice's "center" frequency using oscillators[0]. I know, this is gross.
        std::array<Oscillator, kNumOscillators> oscillators;

        ADSREnvState ampEnvState;
        ADSREnvState cutoffEnvState;
	ADSREnvState pitchEnvState;
        int currentMidiNote = -1;

        // float lp0 = 0.0f;
        // float lp1 = 0.0f;
        // float lp2 = 0.0f;
        // float lp3 = 0.0f;

	// NEW FILTER LET'S GO
	float ic1eq = 0.f;
	float ic2eq = 0.f;
    };

    struct StateData {
        int channel = -1;

        std::array<Voice, 6> voices;

        Patch patch;

        float pitchLFOPhase = 0.0f;
        float cutoffLFOPhase = 0.0f;
    };

    void InitStateData(StateData& state, int channel);

    void Process(
        StateData* state, boost::circular_buffer<audio::Event> const& pendingEvents,
        float* outputBuffer, int const numChannels, int const framesPerBuffer,
        int const sampleRate, unsigned long frameStartTickTime);
}
