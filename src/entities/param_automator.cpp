#include "param_automator.h"

#include "math_util.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "step_sequencer.h"
#include "sequencer.h"
#include "audio.h"

void ParamAutomatorEntity::LoadDerived(serial::Ptree pt) {
    _props.Load(pt);
}

void ParamAutomatorEntity::SaveDerived(serial::Ptree pt) const {
    _props.Save(pt);
}

void ParamAutomatorEntity::InitDerived(GameManager& g) {
    if (_props._seqEditorId.IsValid()) {
        ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_props._seqEditorId, ne::EntityType::StepSequencer);
        if (e == nullptr) {
            e = g._neEntityManager->FindEntityByEditorIdAndType(_props._seqEditorId, ne::EntityType::Sequencer);
        }
        if (e == nullptr) {
            printf("ParamAutomatorEntity::Init (\"%s\"): could not find seq/stepseq entity with name \"%lld\"\n", _name.c_str(), _props._seqEditorId._id);
            return;
        }
        _seqId = e->_id;
    }
    _automateStartTime = g._beatClock->GetBeatTimeFromEpoch();

    if (_props._relative) {
        if (_props._synth) {
            synth::Patch const& patch = g._audioContext->_state.synths[_props._channel].patch;
            float currentVal = patch.Get(_props._synthParam);
            _startValueAdjusted = currentVal + _props._startValue;
            _endValueAdjusted = currentVal + _props._endValue;                        
        }
    } else {
        _startValueAdjusted = _props._startValue;
        _endValueAdjusted = _props._endValue;
    }
}

ne::Entity::ImGuiResult ParamAutomatorEntity::ImGuiDerived(GameManager& g) {
    bool needsInit = _props.ImGui();
    return needsInit ? ImGuiResult::NeedsInit : ImGuiResult::Done;
}

void ParamAutomatorEntity::Update(GameManager& g, float dt) {
    if (!_props._synth && !_seqId.IsValid()) {
        return;
    }
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double const automateTime = beatTime - _automateStartTime;
    if (_automateStartTime > beatTime) {
        return;
    }
    // We don't early-return here so we ensure we actually set to the end value.
    if (automateTime >= _props._desiredAutomateTime || _props._desiredAutomateTime <= 0.0) {
        g._neEntityManager->TagForDestroy(_id);
    }
    double factor = 1.0;
    if (_props._desiredAutomateTime > 0.0) {
        factor = math_util::Clamp(automateTime / _props._desiredAutomateTime, 0.0, 1.0);
    }
    float newValue = _startValueAdjusted + factor * (_endValueAdjusted - _startValueAdjusted);
    
    if (_props._synth) {
        audio::Event e;
        e.type = audio::EventType::SynthParam;
        e.channel = _props._channel;
        e.delaySecs = 0.0;
        e.param = _props._synthParam;
        e.newParamValue = newValue;
        e.paramChangeTimeSecs = 0.0;
        g._audioContext->AddEvent(e);
    } else {
        ne::Entity* seqBaseEntity = g._neEntityManager->GetEntity(_seqId);
        if (seqBaseEntity->_id._type == ne::EntityType::StepSequencer) {
            StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
            if (seq == nullptr) {
                printf("ParamAutomator (\"%s\"): Sequencer was destroyed!\n", _name.c_str());
            } else {
                switch (_props._stepSeqParam) {
                    case StepSeqParamType::Velocities:
                        seq->SetAllVelocitiesPermanent(newValue);
                        break;
                    case StepSeqParamType::NoteLength:
                        seq->_noteLength = newValue;
                        break;
                    case StepSeqParamType::Count: assert(false);
                }
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
