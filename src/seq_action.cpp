#include "seq_action.h"

#include "entities/param_automator.h"
#include "entities/step_sequencer.h"
#include "audio.h"
#include "midi_util.h"

void SpawnAutomatorSeqAction::Load(GameManager& g, std::istream& input) {
    std::string paramName;
    input >> paramName;
    if (paramName == "seq_velocity") {
        _synth = false;
        input >> _seqEntityName;
    } else {
        _synth = true;
        _synthParam = audio::StringToSynthParamType(paramName.c_str());
        input >> _channel;
    }
    input >> _startValue;
    input >> _endValue;
    input >> _desiredAutomateTime;
}

void SpawnAutomatorSeqAction::Execute(GameManager& g) {
    ParamAutomatorEntity* e = static_cast<ParamAutomatorEntity*>(g._neEntityManager->AddEntity(ne::EntityType::ParamAutomator));
    e->_startValue = _startValue;
    e->_endValue = _endValue;
    e->_desiredAutomateTime = _desiredAutomateTime;
    e->_synth = _synth;
    e->_synthParam = _synthParam;
    e->_channel = _channel;
    e->_seqEntityName = _seqEntityName;
    e->Init(g);
}

void RemoveEntitySeqAction::Load(GameManager& g, std::istream& input) {
    input >> _entityName;
}

void RemoveEntitySeqAction::Execute(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_entityName);
    if (e) {
        g._neEntityManager->TagForDestroy(e->_id);
    } else {
        printf("RemoveEntitySeqAction could not find entity with name \"%s\"\n", _entityName.c_str());
    }
}

void ChangeStepSequencerSeqAction::Execute(GameManager& g) {
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq) {
        seq->SetNextSeqStep(g, _midiNote, _velocity);
    } else {
        printf("ChangeStepSequencerSeqAction: no seq entity!!\n");
        return;
    }
}

void ChangeStepSequencerSeqAction::Load(GameManager& g, std::istream& input) {
    std::string seqName;
    input >> seqName;
    ne::Entity* e = g._neEntityManager->FindEntityByName(seqName);
    if (e) {
        _seqId = e->_id;
    } else {
        printf("ChangeStepSequencerSeqAction: could not find seq entity \"%s\"\n", seqName.c_str());
    }
    input >> _midiNote;
    input >> _velocity;
}

void NoteOnOffSeqAction::Execute(GameManager& g) {
    BeatTimeEvent b_e;
    b_e._beatTime = 0.0;
    b_e._e.type = audio::EventType::NoteOn;
    b_e._e.channel = _channel;
    b_e._e.midiNote = _midiNote;
    b_e._e.velocity =_velocity;
                
    audio::Event e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);

    b_e._e.type = audio::EventType::NoteOff;
    b_e._beatTime = _noteLength;
    e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);
}

void NoteOnOffSeqAction::Load(GameManager& g, std::istream& input) {
    std::string token, key, value;
    while (!input.eof()) {
        {
            input >> token;
            std::size_t delimIx = token.find_first_of(':');
            if (delimIx == std::string::npos) {
                printf("Token missing \":\" - \"%s\"\n", token.c_str());
                continue;
            }

            key = token.substr(0, delimIx);
            value = token.substr(delimIx+1);
        }
        if (key == "channel") {
            _channel = std::stoi(value);
        } else if (key == "note") {
            _midiNote = GetMidiNote(value.c_str());
        } else if (key == "length") {
            _noteLength = std::stod(value);
        } else if (key == "velocity") {
            _velocity = std::stof(value);
        } else if (key == "quantize") {
            _quantizeDenom = std::stod(value);
        } else {
            printf("NoteOnOffSeqAction::Load: unknown key \"%s\"\n", key.c_str());
        }
    }
}

void BeatTimeEventSeqAction::Execute(GameManager& g) {
    audio::Event e = GetEventAtBeatOffsetFromNextDenom(_quantizeDenom, _b_e, *g._beatClock, /*slack=*/0.0625);
    g._audioContext->AddEvent(e);
}

void BeatTimeEventSeqAction::Load(GameManager& g, std::istream& input) {
    ReadBeatEventFromTextLine(*g._soundBank, input, _b_e);
}
