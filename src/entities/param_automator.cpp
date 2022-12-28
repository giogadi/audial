#include "param_automator.h"

#include "math_util.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "step_sequencer.h"
#include "sequencer.h"
#include "audio.h"

void ParamAutomatorEntity::Init(GameManager& g) {
    if (!_seqEntityName.empty()) {
        ne::Entity* e = g._neEntityManager->FindEntityByName(_seqEntityName);
        if (e == nullptr) {
            printf("ParamAutomatorEntity::Init (\"%s\"): could not find entity with name \"%s\"\n", _name.c_str(), _seqEntityName.c_str());
            return;
        }
        if (e->_id._type != ne::EntityType::Sequencer &&
            e->_id._type != ne::EntityType::StepSequencer) {
            printf("ParamAutomatorEntity::Init: (\"%s\"): entity found of name \"%s\" was not a seq entity!\n", _name.c_str(), _seqEntityName.c_str());
            return;
        }
        _seqId = e->_id;
    }
    _automateStartTime = g._beatClock->GetBeatTimeFromEpoch();    
}

void ParamAutomatorEntity::Update(GameManager& g, float dt) {
    if (!_synth && !_seqId.IsValid()) {
        return;
    }
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double const automateTime = beatTime - _automateStartTime;
    if (_automateStartTime > beatTime) {
        return;
    }
    // We don't early-return here so we ensure we actually set to the end value.
    if (automateTime >= _desiredAutomateTime) {
        g._neEntityManager->TagForDestroy(_id);
    }
    double factor = math_util::Clamp(automateTime / _desiredAutomateTime, 0.0, 1.0);
    float newValue = _startValue + factor * (_endValue - _startValue);
    
    if (_synth) {
        audio::Event e;
        e.type = audio::EventType::SynthParam;
        e.channel = _channel;
        e.timeInTicks = 0;
        e.param = _synthParam;
        e.newParamValue = newValue;
        e.paramChangeTime = 0;
        g._audioContext->AddEvent(e);
    } else {
        ne::Entity* seqBaseEntity = g._neEntityManager->GetEntity(_seqId);
        if (seqBaseEntity->_id._type == ne::EntityType::StepSequencer) {
            StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
            if (seq == nullptr) {
                printf("ParamAutomator (\"%s\"): Sequencer was destroyed!\n", _name.c_str());
            } else {
                seq->SetAllVelocitiesPermanent(newValue);
            }
        } else if (seqBaseEntity->_id._type == ne::EntityType::Sequencer) {
            SequencerEntity* seq = static_cast<SequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
            if (seq == nullptr) {
                printf("ParamAutomator (\"%s\"): Sequencer was destroyed!\n", _name.c_str());
            } else {
                for (BeatTimeEvent& b_e : seq->_events) {
                    if (b_e._e.type == audio::EventType::NoteOn) {
                        b_e._e.velocity = newValue;
                    } else if (b_e._e.type == audio::EventType::PlayPcm) {
                        b_e._e.pcmVelocity = newValue;
                    }
                }
            }
        } else {
            assert(false);
        }
    }
}
