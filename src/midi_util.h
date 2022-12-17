#pragma once

#include <string_view>

inline int GetMidiNote(char noteName, int octaveNum) {
    noteName = tolower(noteName);
    assert(noteName >= 'a' && noteName <= 'g');
    assert(octaveNum >= 0);
    assert(octaveNum < 8);
    // c0 is 12.
    int midiNum = 0;
    switch (noteName) {        
        case 'c': midiNum = 12; break;
        case 'd': midiNum = 14; break;
        case 'e': midiNum = 16; break;
        case 'f': midiNum = 17; break;
        case 'g': midiNum = 19; break;
        case 'a': midiNum = 21; break;
        case 'b': midiNum = 23; break;
    }
    midiNum += 12 * octaveNum;
    return midiNum;
}

inline int GetMidiNote(std::string_view noteName) {
    assert(noteName.length() == 2 || noteName.length() == 3);
    char octaveChar = noteName[1];
    int octaveNum = octaveChar - '0';
    int modifier = 0;
    if (noteName.length() == 3) {
        if (noteName[2] == '+') {
            modifier = 1;
        } else if (noteName[2] == '-') {
            modifier = -1;
        }
    }
    int note = GetMidiNote(noteName[0], octaveNum) + modifier;
    return note;
}

inline void GetNoteName(int midiNote, std::string& name) {
    int octave = (midiNote - 12) / 12;
    char octaveChar = '0' + octave;
    int offsetFromC = midiNote % 12;
    name.clear();
    name.reserve(3);
    switch (offsetFromC) {
        case 0:
            name.push_back('C');            
            name.push_back(octaveChar);
            break;
        case 1:
            name.push_back('D');     
            name.push_back(octaveChar);
            name.push_back('-');
            break;
        case 2:
            name.push_back('D');            
            name.push_back(octaveChar);
            break;
        case 3:
            name.push_back('E');     
            name.push_back(octaveChar);
            name.push_back('-');
            break;
        case 4:
            name.push_back('E');            
            name.push_back(octaveChar);
            break;
        case 5:
            name.push_back('F');            
            name.push_back(octaveChar);
            break;
        case 6:
            name.push_back('G');
            name.push_back(octaveChar);
            name.push_back('-');
            break;
        case 7:
            name.push_back('G');
            name.push_back(octaveChar);
            break;
        case 8:
            name.push_back('A');
            name.push_back(octaveChar);
            name.push_back('-');
            break;
        case 9:
            name.push_back('A');
            name.push_back(octaveChar);
            break;
        case 10:
            name.push_back('B');
            name.push_back(octaveChar);
            name.push_back('-');
            break;
        case 11:
            name.push_back('B');
            name.push_back(octaveChar);
            break;
        default:
            assert(false);
            name = "***";
            break;
    }
}
