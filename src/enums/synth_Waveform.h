#pragma once


namespace synth {


enum class Waveform : int {
    
    Square,
    
    Saw,
    
    Noise,
    
    Count
};
extern char const* gWaveformStrings[];
char const* WaveformToString(Waveform e);
Waveform StringToWaveform(char const* s);

bool WaveformImGui(char const* label, Waveform* v);


}

char const* EnumToString(synth::Waveform e);
void StringToEnum(char const* s, synth::Waveform& e);