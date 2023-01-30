#include "typing_enemy.h"

#include <sstream>

#include "game_manager.h"
#include "renderer.h"
#include "audio.h"
#include "step_sequencer.h"
#include "typing_player.h"

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
    _currentColor = _modelColor;
    if (_sectionId >= 0) {
        int numEntities = 0;
        ne::EntityManager::Iterator eIter = g._neEntityManager->GetIterator(ne::EntityType::TypingPlayer, &numEntities);
        assert(numEntities == 1);
        TypingPlayerEntity* player = static_cast<TypingPlayerEntity*>(eIter.GetEntity());
        assert(player != nullptr);
        player->RegisterSectionEnemy(_sectionId, _id);
    }
    for (auto const& pAction : _hitActions) {
        pAction->InitBase(g);
    }
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

    if (_destroyIfOffscreenLeft) {
        float constexpr kMinX = -10.f;
        if (p._x < kMinX) {
            g._neEntityManager->TagForDestroy(_id);
        }
    }

    float screenX, screenY;
    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    ProjectWorldPointToScreenSpace(_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);

    screenX = std::round(screenX);
    screenY = std::round(screenY);
    float constexpr kTextSize = 0.75f;
    if (_numHits > 0) {
        std::string_view substr = std::string_view(_text).substr(0, _numHits);
        g._scene->DrawText(substr, screenX, screenY, /*scale=*/kTextSize, Vec4(1.f,1.f,0.f,1.f));
    }
    if (_numHits < _text.length()) {
        std::string_view substr = std::string_view(_text).substr(_numHits);
        g._scene->DrawText(substr, screenX, screenY, /*scale=*/kTextSize, _textColor);
    }

    if (_model != nullptr) {
        g._scene->DrawMesh(_model, _transform, _currentColor);
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
    ++_numHits;
    if (_numHits == _text.length()) {
        if (_destroyAfterTyped) {
            bool success = g._neEntityManager->TagForDestroy(_id);
            assert(success);
        }            
    }

    switch (_hitBehavior) {
        case HitBehavior::SingleAction: {
            int hitActionIx = (_numHits - 1) % _hitActions.size();
            SeqAction& a = *_hitActions[hitActionIx];

            a.ExecuteBase(g);
            break;
        }
        case HitBehavior::AllActions: {
            for (auto const& pAction : _hitActions) {
                pAction->ExecuteBase(g);
            }
            break;
        }
    }
}

InputManager::Key TypingEnemyEntity::GetNextKey() const {
    int textIndex = std::min(_numHits, (int) _text.length() - 1);
    char nextChar = std::tolower(_text.at(textIndex));
    int charIx = nextChar - 'a';
    if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
        printf("WARNING: char \'%c\' not in InputManager!\n", nextChar);
        return InputManager::Key::NumKeys;
    }
    InputManager::Key nextKey = static_cast<InputManager::Key>(charIx);
    return nextKey;
}

void TypingEnemyEntity::LoadDerived(serial::Ptree pt) {
    _text = pt.GetString("text");
    serial::Ptree actionsPt = pt.GetChild("hit_actions");
    int numChildren;
    serial::NameTreePair* children = actionsPt.GetChildren(&numChildren);
    _hitActions.clear();
    _hitActions.reserve(numChildren);
    SeqAction::LoadInputs loadInputs;  // default
    for (int i = 0; i < numChildren; ++i) {
        std::string seqActionStr = children[i]._pt.GetStringValue();
        std::stringstream ss(seqActionStr);
        _hitActions.push_back(SeqAction::LoadAction(loadInputs, ss));
    }
    delete[] children;    
    
}

void TypingEnemyEntity::SaveDerived(serial::Ptree pt) const {
    assert(false);  // UNSUPPORTED
    // pt.PutString("text", _text.c_str());
    // serial::Ptree actionsPt = pt.AddChild("hit_actions");
    // for (auto const& pSeqAction : _hitActions) {
    //     serial::Ptree actionPt = actionsPt.AddChild("action");
    //     actionPt.PutString(pSeqAction->
    // }
}

ne::BaseEntity::ImGuiResult TypingEnemyEntity::ImGuiDerived(GameManager& g) {
    return ImGuiResult::Done;
}
