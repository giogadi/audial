#include "param_automator.h"

#include "game_manager.h"
#include "beat_clock.h"
#include "step_sequencer.h"

void ParamAutomatorEntity::Init(GameManager& g) {
    ne::Entity* e = g._neEntityManager->FindEntityByName(_seqEntityName);
    if (e == nullptr) {
        printf("ParamAutomatorEntity::Init (\"%s\"): could not find entity with name \"%s\"\n", _name.c_str(), _seqEntityName.c_str());
        return;
    }
    if (e->_id._type != ne::EntityType::StepSequencer) {
        printf("ParamAutomatorEntity::Init: (\"%s\"): entity found of name \"%s\" was not a seq entity!\n", _name.c_str(), _seqEntityName.c_str());
        return;
    }
    _seqId = e->_id;
    _automateStartTime = g._beatClock->GetBeatTimeFromEpoch();
}

void ParamAutomatorEntity::Update(GameManager& g, float dt) {
    if (!_seqId.IsValid()) {
        return;
    }
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double const automateTime = beatTime - _automateStartTime;
    if (_automateStartTime > beatTime) {
        return;
    }
    // TODO: This early return might cause us to never get to _endVelocity.
    if (automateTime >= _desiredAutomateTime) {
        g._neEntityManager->TagForDestroy(_id);
        return;
    }
    double factor = automateTime / _desiredAutomateTime;
    assert(factor >= 0.0);
    assert(factor <= 1.0);

    float velocity = _startVelocity + factor * (_endVelocity - _startVelocity);
    StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(_seqId));
    if (seq == nullptr) {
        printf("ParamAutomator (\"%s\"): Sequencer was destroyed!\n", _name.c_str());
    } else {
        for (StepSequencerEntity::SeqStep& s : seq->_initialMidiSequence) {
            s._velocity = velocity;
        }
    }
}
