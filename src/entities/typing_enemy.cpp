#include "typing_enemy.h"

#include "game_manager.h"
#include "renderer.h"
#include "audio.h"
#include "step_sequencer.h"

namespace {

void ProjectWorldPointToScreenSpace(Vec3 const& worldPos, Mat4 const& viewProjMatrix, int screenWidth, int screenHeight, float& screenX, float& screenY) {
    Vec4 pos(worldPos._x, worldPos._y, worldPos._z, 1.f);
    pos = viewProjMatrix * pos;
    pos.Set(pos._x / pos._w, pos._y / pos._w, pos._z / pos._w, 1.f);
    // map [-1,1] x [-1,1] to [0,sw] x [0,sh]
    screenX = (pos._x * 0.5f + 0.5f) * screenWidth;
    screenY = (-pos._y * 0.5f + 0.5f) * screenHeight;
}

}

void TypingEnemyEntity::Init(GameManager& g) {
    ne::Entity::Init(g);
}

void TypingEnemyEntity::Update(GameManager& g, float dt) {
    if (!IsActive(g)) {
        return;
    }

    Vec3 p  = _transform.GetPos();
    Vec3 dp = _velocity * dt;
    p += dp;
    _transform.SetTranslation(p);

    float screenX, screenY;
    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    ProjectWorldPointToScreenSpace(_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);

    screenX = std::round(screenX);
    screenY = std::round(screenY);
    if (_numHits > 0) {
        std::string_view substr = std::string_view(_text).substr(0, _numHits);
        g._scene->DrawText(substr, screenX, screenY, /*scale=*/1.f, Vec4(1.f,1.f,0.f,1.f));
    }
    if (_numHits < _text.length()) {
        std::string_view substr = std::string_view(_text).substr(_numHits);
        g._scene->DrawText(substr, screenX, screenY, /*scale=*/1.f, Vec4(1.f,1.f,1.f,1.f));
    }
    ne::BaseEntity::Update(g, dt);
}

namespace {
static double constexpr kSlack = 0.0625;
void SendEventsFromEventsList(TypingEnemyEntity const& enemy, GameManager& g, std::vector<BeatTimeEvent> const& events) {
    for (BeatTimeEvent const& b_e : events) {
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(enemy._eventStartDenom, b_e, *g._beatClock, kSlack);
        g._audioContext->AddEvent(e);
    }
}
}

bool TypingEnemyEntity::IsActive(GameManager& g) const {
    if (g._editMode) {
        return true;
    }
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (_activeBeatTime >= 0.0 && beatTime < _activeBeatTime) {
        return false;
    }
    if (_inactiveBeatTime >= 0.0 && beatTime > _inactiveBeatTime) {
        return false;
    }
    return true;
}

void TypingEnemyEntity::OnHit(GameManager& g) {
    if (_numHits < _text.length()) {
        ++_numHits;
        // if (_numHits == _text.length()) {
        //     SendEventsFromEventsList(*this, g, _deathEvents);
        //     assert(g._neEntityManager->TagForDestroy(_id));
        // } else {
        //     SendEventsFromEventsList(*this, g, _hitEvents);
        // }

        int hitActionIx = (_numHits - 1) % _hitActions.size();
        Action const& a = _hitActions[hitActionIx];

        switch (a._type) {
            case Action::Type::Seq: {
                StepSequencerEntity* seq = static_cast<StepSequencerEntity*>(g._neEntityManager->GetEntity(a._seqId));
                if (seq == nullptr) {
                    printf("seq-type hit action could not find seq entity!\n");
                    break;
                }
                seq->SetNextSeqStep(g, a._seqMidiNote);
                break;
            }
            case Action::Type::Note: {
                BeatTimeEvent b_e;
                b_e._beatTime = 0.0;
                b_e._e.type = audio::EventType::NoteOn;
                b_e._e.channel = a._noteChannel;
                b_e._e.midiNote = a._noteMidiNote;
                b_e._e.velocity = a._velocity;

                // Should we push to next denom?
                // GetNextBeatDenomTime
                
                audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, /*slack=*/0.0625);
                g._audioContext->AddEvent(e);
                // b_e._e.timeInTicks = 0;
                // g._audioContext->AddEvent(b_e._e);

                b_e._e.type = audio::EventType::NoteOff;
                b_e._beatTime = a._noteLength;
                e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, /*slack=*/0.0625);
                g._audioContext->AddEvent(e);
                // b_e._e.timeInTicks = g._beatClock->GetTimeInTicks() + g._beatClock->BeatTimeToTickTime(a._noteLength);
                // g._audioContext->AddEvent(b_e._e);
                    
                break;
            }
        }
    }
}
