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


}
