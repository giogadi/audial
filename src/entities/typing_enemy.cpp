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
    if (g._editMode) {
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
        if (_numHits == _text.length()) {
            bool success = g._neEntityManager->TagForDestroy(_id);
            assert(success);
        }

        switch (_hitBehavior) {
            case HitBehavior::SingleAction: {
                int hitActionIx = (_numHits - 1) % _hitActions.size();
                SeqAction& a = *_hitActions[hitActionIx];

                a.Execute(g);
                break;
            }
            case HitBehavior::AllActions: {
                for (auto const& pAction : _hitActions) {
                    pAction->Execute(g);
                }
                break;
            }
        }       
    }
}
