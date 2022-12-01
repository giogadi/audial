#pragma once

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

inline int GetMidiNote(char const* noteName) {
    assert(noteName[0] != '\0' && noteName[1] != '\0');
    char octaveChar = noteName[1];
    int octaveNum = octaveChar - '0';
    int modifier = 0;
    if (noteName[2] == '+') {
        modifier = 1;
    } else if (noteName[2] == '-') {
        modifier = -1;
    }
    int note = GetMidiNote(noteName[0], octaveNum) + modifier;
    return note;
}
