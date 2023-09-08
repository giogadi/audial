#include <fstream>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "synth.h"
#include "serial.h"
#include "synth_patch_bank.h"
#include "constants.h"
#include "midi_util.h"

struct MidiEvent {
    bool _on = true;
    int _midiNote = 0;
    float _t = 0.f;
};

void ProcessEvent(synth::StateData& state, MidiEvent const& event) {
    if (event._on) {
        synth::NoteOn(state, event._midiNote, 1.f);
    } else {
        synth::NoteOff(state, event._midiNote);
    }
}

int main(int argc, char** argv) {
    // Accept a patches XML file, and a patch name to use, and then a sequence file (TBD).    
    if (argc != 1 + 3) {
        printf("Expected 3 arguments, only got %d\n", argc - 1);
        return 1;
    }
    std::string patchBankFilename = argv[1];
    std::string patchName = argv[2];
    std::string seqFilename = argv[3];

    synth::PatchBank patchBank;
    if (!serial::LoadFromFile(patchBankFilename.c_str(), patchBank)) {
        printf("Failed to open patch bank at \"%s\"\n", patchBankFilename.c_str());
        return 1;
    }

    synth::Patch* patch = patchBank.GetPatch(patchName);
    if (patch == nullptr) {
        printf("Failed to find patch \"%s\" in patch bank.\n", patchName.c_str());
        return 1;
    }

    /*std::ifstream seqFile(seqFilename.c_str());
    if (!seqFile.is_open()) {
        printf("Failed to open seq file \"%s\"\n", seqFilename.c_str());
        return 1;
    }*/

    int constexpr kSampleRate = 48000;
    int constexpr kBufferSize = 512;
    int constexpr kNumChannels = 2;

    synth::StateData synthState;
    synth::InitStateData(synthState, /*channel=*/0, kBufferSize, /*numBufferChannels=*/kNumChannels);
    synthState.patch = *patch;

    int note = GetMidiNote("A3");
    
    /*std::vector<MidiEvent> midiEvents = {
        {true, note, 0.f},
        {false, note, 0.5f},
        {true, note, 1.f},
        {false, note, 1.5f},
        {true, note, 2.f},
        {false, note, 2.5f},
        {true, note, 3.f},
        {false, note, 3.5f},
        {true, note, 4.f},
        {false, note, 4.5f}
    };*/
    std::vector<MidiEvent> midiEvents;
    for (float t = 0.f; t < 5.f; t += 0.5f) {
        MidiEvent e;
        e._midiNote = note;
        e._on = true;
        e._t = t;
        midiEvents.push_back(e);

        e._on = false;
        e._t += 0.25f;
        midiEvents.push_back(e);
    }

    {
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = kNumChannels;
        format.sampleRate = kSampleRate;
        format.bitsPerSample = 32;
        drwav wav;
        drwav_init_file_write(&wav, "howdy.wav", &format, nullptr);

        float const totalTimeSecs = 5.f;
        int const numBuffers = static_cast<int>(std::ceil(totalTimeSecs * static_cast<float>(kSampleRate) / static_cast<float>(kBufferSize)));
        float const kSecsPerBuffer = static_cast<float>(kBufferSize) / static_cast<float>(kSampleRate);
        boost::circular_buffer<audio::PendingEvent> events;
        float currentTime = 0.f;
        int currentEventIx = 0;
        for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
            if (currentEventIx < midiEvents.size()) {
                if (currentTime >= midiEvents[currentEventIx]._t) {
                    ProcessEvent(synthState, midiEvents[currentEventIx]);
                    ++currentEventIx;
                }
            }

            float buffer[kBufferSize * kNumChannels];
            for (int i = 0; i < kBufferSize * kNumChannels; ++i) {
                buffer[i] = 0.f;
            }
            synth::Process(&synthState, events, buffer, kNumChannels, kBufferSize, kSampleRate, bufferIx);
            for (int i = 0; i < kBufferSize; ++i) {
                printf("%f\n", buffer[kNumChannels * i]);
            }
            int32_t buffer32[kBufferSize * kNumChannels];
            drwav_f32_to_s32(buffer32, buffer, kBufferSize * kNumChannels);
            drwav_write_pcm_frames(&wav, kBufferSize, buffer32);

            currentTime += kSecsPerBuffer;
        }

        drwav_uninit(&wav);
    }

    synth::DestroyStateData(synthState);

    return 0;
}
