#include "sound_bank.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

void SoundBank::LoadSounds() {
    _soundNames = {
        "kick"
        , "block"
        , "snare"
        , "liquid_dnb_loop"
        , "liquid_dnb_chords"
        , "liquid_dnb_drums"
        , "tech_chords"
        , "tech_drum_groove"
        , "hihat_open_909"
        , "clap_909"
    };
    std::vector<char const*> soundFilenames = {
        "data/sounds/kick_deep.wav"
        , "data/sounds/woodblock_reverb_mono.wav"
        , "data/sounds/snare_909.wav"
        , "data/sounds/liquid_dnb.wav"
        , "data/sounds/liquid_chords.wav"
        , "data/sounds/liquid_dnb_drums.wav"
        , "data/sounds/tech_chords.wav"
        , "data/sounds/tech_drum_groove.wav"
        , "data/sounds/hihat_open_909.wav"
        , "data/sounds/clap_909.wav"
    };
    assert(soundFilenames.size() == _soundNames.size());
    for (int i = 0, n = soundFilenames.size(); i < n; ++i) {
        char const* filename = soundFilenames[i];
        audio::PcmSound sound;
        unsigned int numChannels;
        unsigned int sampleRate;
        sound._buffer = drwav_open_file_and_read_pcm_frames_f32(
            filename, &numChannels, &sampleRate, &sound._bufferLength, /*???*/NULL);
        assert(numChannels == 1);
        assert(sampleRate == 44100);
        assert(sound._buffer != nullptr);
        printf("Loaded \"%s\": %d channels, %llu samples.\n", filename, numChannels, sound._bufferLength);
        _sounds.push_back(std::move(sound));
    }
}

void SoundBank::Destroy() {
    for (audio::PcmSound& sound : _sounds) {
        drwav_free(sound._buffer, /*???*/NULL);
    }
}

int SoundBank::GetSoundIx(char const* soundName) const {
    for (int i = 0, n = _soundNames.size(); i < n; ++i) {
        if (strcmp(soundName, _soundNames[i]) == 0) {
            return i;
        }
    }
    return -1;
}
