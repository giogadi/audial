#pragma once

#include <string_view>
#include <cassert>

inline constexpr int kMidiMinOctave = 0;
inline constexpr int kMidiMaxOctave = 8;
inline constexpr int kMidiMinNote = 0;
inline constexpr int kMidiMaxNote = 119;

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

inline bool MaybeGetMidiNote(char noteName, int octaveNum, int& midiNum) {
    noteName = tolower(noteName);
    if (noteName < 'a' || noteName > 'g') {
        return false;
    }
    if (octaveNum < 0 || octaveNum >= 8) {
        return false;
    }
    // c0 is 12.
    midiNum = 0;
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
    return true;
}

inline bool MaybeGetMidiNoteFromStr(std::string_view noteName, int& midiNote) {
    if (noteName.length() != 2 && noteName.length() != 3) {
        return false;
    }
    char octaveChar = noteName[1];
    int octaveNum = octaveChar - '0';
    int modifier = 0;
    if (noteName.length() == 3) {
        if (noteName[2] == '+') {
            modifier = 1;
        } else if (noteName[2] == '-') {
            modifier = -1;
        } else {
            return false;
        }
    }
    bool success = MaybeGetMidiNote(noteName[0], octaveNum, midiNote);
    if (success) {
        midiNote += modifier;
    }
    return success;
}

// Returns -1 on failure
inline int GetMidiNote(std::string_view noteName) {
    int note = -1;
    bool success = MaybeGetMidiNoteFromStr(noteName, note);
    if (success) {
        return note;
    } else {
        return -1;
    }
}

inline void GetNoteName(int midiNote, char* nameStr) {
    // ASSUMES nameStr CAN STORE 3 CHARS PLUS \0
    int octave = (midiNote - 12) / 12;
    if (octave < 0 || octave >= 8) {
        strcpy(nameStr, "**");
        return;
    }
    char octaveChar = '0' + octave;
    int offsetFromC = midiNote % 12;
    nameStr[1] = octaveChar;
    nameStr[2] = '\0';
    switch (offsetFromC) {
        case 0:
            nameStr[0] = 'C';
            break;
        case 1:
            nameStr[0] = 'D';
            nameStr[2] = '-';
            break;
        case 2:
            nameStr[0] = 'D';
            break;            
        case 3:
            nameStr[0] = 'E';
            nameStr[2] = '-';
            break;
        case 4:
            nameStr[0] = 'E';
            break;
        case 5:
            nameStr[0] = 'F';
            break;
        case 6:
            nameStr[0] = 'G';
            nameStr[2] = '-';
            break;
        case 7:
            nameStr[0] = 'G';
            break;
        case 8:
            nameStr[0] = 'A';
            nameStr[2] = '-';
            break;
        case 9:
            nameStr[0] = 'A';
            break;
        case 10:
            nameStr[0] = 'B';
            nameStr[2] = '-';
            break;
        case 11:
            nameStr[0] = 'B';
            break;
    }
}

inline void GetNoteName(int midiNote, std::string& name) {
    int octave = (midiNote - 12) / 12;
    if (octave < 0 || octave >= 8) {
        name = "**";
        return;
    }
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
            name = "**";
            break;
    }
}
