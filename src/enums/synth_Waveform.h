#pragma once


namespace synth {


enum class Waveform : int {
    
    Square,
    
    Saw,
    
    Count
};
extern char const* gWaveformStrings[];
char const* WaveformToString(Waveform e);
Waveform StringToWaveform(char const* s);
char const* EnumToString(Waveform e);
void StringToEnum(char const* s, Waveform& e);

bool WaveformImGui(char const* label, Waveform* v);


}
