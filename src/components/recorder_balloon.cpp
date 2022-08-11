#include "recorder_balloon.h"

#include "components/transform.h"
#include "components/events_on_hit.h"
#include "components/model_instance.h"
#include "components/sequencer.h"
#include "entity.h"
#include "game_manager.h"
#include "beat_clock.h"
#include "renderer.h"
#include "beat_time_event.h"

bool RecorderBalloonComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _transform = e.FindComponentOfType<TransformComponent>();
    if (_transform.expired()) {
        return false;
    }

    _model = e.FindComponentOfType<ModelComponent>();
    if (_model.expired()) {
        return false;
    }

    _sequencer = e.FindComponentOfType<SequencerComponent>();
    if (_sequencer.expired()) {
        return false;
    }

    auto const& onHitComp = e.FindComponentOfType<EventsOnHitComponent>().lock();
    if (onHitComp == nullptr) {
        return false;
    }
    onHitComp->AddOnHitCallback(
        std::bind(&RecorderBalloonComponent::OnHit, this, std::placeholders::_1));

    _g = &g;

    int numNotes = std::round(4 / _denom);
    _recordedNotes.assign(numNotes, RecordInfo());

    _numHits = 0;

    return true;
}

void RecorderBalloonComponent::OnHit(EntityId other) {
    _wasHit = true;
}

void RecorderBalloonComponent::MaybeCommitSequence() {
    int measureNumber = std::floor(_g->_beatClock->GetBeatTime() / 4.0) - 1;

    // Count the recorded notes and see if we hit the minimum amount.
    int numRecorded = 0;
    for (RecordInfo const& r : _recordedNotes) {
        bool yes = r._hit && r._measureNumber == measureNumber;
        numRecorded += static_cast<int>(yes);
    }

    if (numRecorded >= _numDesiredHits) {
        std::shared_ptr<SequencerComponent> sequencer = _sequencer.lock();
        assert(sequencer != nullptr);

        sequencer->ClearSequence();
        for (int i = 0, n = _recordedNotes.size(); i < n; ++i) {
            RecordInfo const& r = _recordedNotes[i];
            if (r._hit && r._measureNumber == measureNumber) {
                // Let's make the BeatTimeEvent!!!!
                BeatTimeEvent b_e;
                b_e._e.type = audio::EventType::PlayPcm;
                b_e._e.channel = 0;
                b_e._e.midiNote = 0;
                b_e._e.velocity = 1.f;
                b_e._beatTime = _denom * i;
                sequencer->AddToSequenceDangerous(b_e);
            }
        }
        // Last event to mark the end of the sequence.
        BeatTimeEvent b_e;
        b_e._beatTime = 4.0;
        sequencer->AddToSequenceDangerous(b_e);

        sequencer->Reset();
        sequencer->SortEventsByTime();
        // This should put the sequencer in sync with this component. Set the
        // loop's start to the beat time of the beginning of the measure.
        sequencer->_currentLoopStartBeatTime = (double)(measureNumber * 4);
    }

    // Erase any old entries.
    for (RecordInfo& r : _recordedNotes) {
        if (r._hit && r._measureNumber <= measureNumber) {
            r._hit = false;
            r._measureNumber = -1;
        }
    }
}

void RecorderBalloonComponent::Update(float dt) {
    int beatIx = static_cast<int>(std::floor(_g->_beatClock->GetBeatTime())) % 4;

    if (_wasHit) {
        _wasHit = false;

        // Mute sequencer so player can hear what they're playing. We'll unmute at the beginning of the new measure.
        auto seq = _sequencer.lock();
        assert(seq != nullptr);
        seq->_muted = true;

        ++_numHits;

        // Figure out exactly where on the beat timeline this hit sound will play.
        double playTime = BeatClock::GetNextBeatDenomTime(_g->_beatClock->GetBeatTime(), _denom);
        int numDenoms = _recordedNotes.size();
        int playTimeInDenoms = static_cast<int>(std::round(playTime / _denom));
        int denomIx = playTimeInDenoms % numDenoms;
        _recordedNotes[denomIx]._hit = true;
        _recordedNotes[denomIx]._measureNumber = std::floor(playTime / 4.0);
    }    

    if (beatIx == 0 && _g->_beatClock->IsNewBeat()) {
        _numHits = 0;
        MaybeCommitSequence();
        auto seq = _sequencer.lock();
        assert(seq);
        seq->_muted = false;
    }

    float factor = std::min(1.f, (float) _numHits / (float) _numDesiredHits);
    Vec4 color(1.f, 1.f - factor, 1.f - factor, 1.f);

    auto const& model = _model.lock();
    assert(model != nullptr);
    model->_color = color;

    std::shared_ptr<TransformComponent> transform = _transform.lock();

    // Draw the beat blocks above the cube.
    float constexpr kBeatBlockSize = 0.2f;
    float constexpr kBeatBlockSpacing = 0.2f;
    int constexpr kNumBlocks = 4;
    float beatBlocksTotalLength = (kNumBlocks * kBeatBlockSize) + (kBeatBlockSpacing * (kNumBlocks - 1));

    Mat4 beatBlockTrans;
    beatBlockTrans.ScaleUniform(kBeatBlockSize);
    beatBlockTrans.SetTranslation(transform->GetWorldPos());
    {
        float dx = -((0.5f * beatBlocksTotalLength) - (0.5f * kBeatBlockSize));
        beatBlockTrans.Translate(Vec3(dx, 1.f, 0.f));
    }

    for (int i = 0; i < kNumBlocks; ++i) {
        Vec4 beatBlockColor;
        if (i == beatIx) {
            beatBlockColor = Vec4(1.f, 0.f, 0.f, 1.f);
        } else {
            beatBlockColor = Vec4(1.f, 1.f, 1.f, 1.f);
        }
        _g->_scene->DrawCube(beatBlockTrans, beatBlockColor);

        float dx = kBeatBlockSize + kBeatBlockSpacing;
        beatBlockTrans.Translate(Vec3(dx, 0.f, 0.f));
    }
}