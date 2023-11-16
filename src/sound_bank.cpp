#include "sound_bank.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

// TODO: need to convert the following properly to 44100Hz:
// kick_deep, snare, liquid_dnb, liquid_chords, liquid_dnb_drums, tech_chords, tech_drum_groove, feel_vox_*, check_it_out_g
void SoundBank::LoadSounds(int sampleRate) {
    _soundNames = {
        "kick_deep.wav"
        , "woodblock_reverb_mono.wav"
        , "snare.wav"
        , "liquid_dnb.wav"
        , "liquid_chords.wav"
        , "liquid_dnb_drums.wav"
        , "tech_chords.wav"
        , "tech_drum_groove.wav"
        , "hihat_open_909.wav"
        , "clap_909.wav"
        , "feeling_vocals/v1.wav"
        , "feeling_vocals/v2.wav"
        , "feeling_vocals/v3.wav"
        , "feeling_vocals/v4.wav"
        , "feeling_vocals/v5.wav"
        , "feeling_vocals/v6.wav"
        , "feeling_vocals/v7.wav"
        , "feeling_vocals/v8.wav"
        , "feeling_vocals/v9.wav"
        , "kick_909.wav"
        , "snare_909.wav"
        , "hihat_closed_909.wav"
        , "biar_bunny_cbb.wav"
        , "biar_is_a_dbb.wav"
        , "biar_satell_dc.wav"
        , "biar_ite_cant_dbb.wav"
        , "check_it_out.wav"
        , "crash_909.wav"
        , "cymbal_roll.wav"
    };
    _exclusiveGroups = {
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        -1,
        -1,
        -1,
        1,
        1,
        1,
        1,
        -1,
        -1,
        -1
    };
    assert(_soundNames.size() == _exclusiveGroups.size());
    char filename[1024];
    if (sampleRate == 44100) {
        strcpy(filename, "data/sounds/sr44100/");
    } else {
        if (sampleRate != 48000) {
            printf("SoundBank: WARNING unhandled sample rate: %d\n", sampleRate);
        }
        strcpy(filename, "data/sounds/");
    }
    int nameStartIx = strlen(filename);
    char* nameStart = &(filename[nameStartIx]);
    for (int i = 0, n = _soundNames.size(); i < n; ++i) {        
        strcpy(nameStart, _soundNames[i]);
        audio::PcmSound sound;
        unsigned int numChannels;
        unsigned int thisSampleRate;
        sound._buffer = drwav_open_file_and_read_pcm_frames_f32(
            filename, &numChannels, &thisSampleRate, &sound._bufferLength, /*???*/NULL);
        if (sound._buffer == nullptr) {
            printf("SoundBank: error loading sample \"%s\"\n", _soundNames[i]);
        }
        assert(numChannels == 1);
        // if (thisSampleRate != sampleRate) {
        //     printf("SoundBank: WARNING expected sample rate of %d, but sample \"%s\" is %u\n", sampleRate, _soundNames[i], thisSampleRate);
        // }
        assert(sound._buffer != nullptr);
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
