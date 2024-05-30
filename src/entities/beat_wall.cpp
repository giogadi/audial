#include "entities/beat_wall.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "beat_clock.h"
#include "flow_wall.h"
#include "math_util.h"

void BeatWallEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutFloat("spacing", _p.spacing);
    pt.PutInt("num_steps", _p.numSteps);
    serial::PutEnum(pt, "dir", _p.direction);
}

void BeatWallEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetFloat("spacing", &_p.spacing);
    pt.TryGetInt("num_steps", &_p.numSteps);
    serial::TryGetEnum(pt, "dir", _p.direction);
}

ne::BaseEntity::ImGuiResult BeatWallEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;

    ImGui::InputFloat("Spacing", &_p.spacing);

    if (ImGui::InputInt("Num Steps", &_p.numSteps, 1, 4, ImGuiInputTextFlags_EnterReturnsTrue)) {
        _p.numSteps = std::max(1, _p.numSteps);
        result = ImGuiResult::NeedsInit;
    }

    if (DirectionImGui("Direction", &_p.direction)) {
        if (_p.direction == Direction::Center) {
            _p.direction = Direction::Right;
        }
    }

    return result;
}

ne::BaseEntity::ImGuiResult BeatWallEntity::MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}


void BeatWallEntity::InitDerived(GameManager& g) {
    for (ne::EntityId eId : _s.walls) {
        g._neEntityManager->TagForDestroy(eId); 
    }
    _s.walls.clear();

    _s = State();
    
    _s.walls.resize(_p.numSteps);
    for (int ii = 0; ii < _p.numSteps; ++ii) {
        FlowWallEntity* e = (FlowWallEntity*) g._neEntityManager->AddEntity(ne::EntityType::FlowWall);
        _s.walls[ii] = e->_id;
        
        e->_name = "howdy";
        e->_modelName = "cube";
        e->_textureName = "white";
        e->_modelColor = _modelColor;

        e->Init(g);
    }
}

namespace {
    Vec3 DirToVec3(Direction dir) {
        switch (dir) {
            case Direction::Center: return Vec3();
            case Direction::Up: return Vec3(0, 0, -1.f);
            case Direction::Down: return Vec3(0, 0, 1.f);
            case Direction::Left: return Vec3(-1.f, 0, 0.f);
            case Direction::Right: return Vec3(1.f, 0, 0.0f);
            case Direction::Count: assert(false); return Vec3();
        }
    } 
}

void BeatWallEntity::UpdateDerived(GameManager& g, float dt) {
    assert(_p.numSteps == _s.walls.size());

    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double timeInLoop = std::fmod(beatTime, _p.numSteps);
    timeInLoop += _p.beatOffset;
    if (timeInLoop < 0.0) {
        timeInLoop += _p.numSteps;
    } else {
        timeInLoop = std::fmod(timeInLoop, _p.numSteps);
    }
    int stepHole = math_util::Clamp((int)timeInLoop, 0, _p.numSteps - 1);

    Vec3 const& scale = _transform.Scale(); 
    Vec3 dirVec = DirToVec3(_p.direction);

    Vec3 start = _transform.Pos() - 0.5f * Vec3::ElemWiseMult(dirVec, scale);
    Vec3 stepScale = scale;
    for (int ii = 0; ii < 3; ++ii) {
        if (dirVec(ii) != 0.f) {
            stepScale(ii) = (scale(ii) - (_p.numSteps - 1) * _p.spacing) / _p.numSteps;
            stepScale(ii) = std::max(0.001f, stepScale(ii));
        }
    }
    Vec3 pos = start + Vec3::ElemWiseMult(dirVec, 0.5f * stepScale);
    for (int ii = 0; ii < _p.numSteps; ++ii) {
        FlowWallEntity* e = g._neEntityManager->GetEntityAs<FlowWallEntity>(_s.walls[ii]);

        e->_transform.SetPos(pos);
        pos += Vec3::ElemWiseMult(dirVec, stepScale + Vec3(_p.spacing, _p.spacing, _p.spacing));

        e->_transform.SetScale(stepScale); 

        if (ii == stepHole) {
            e->_visible = false;
            e->_canHit = false;
        } else {
            e->_visible = true;
            e->_canHit = true;
        }
    }
}

void BeatWallEntity::Draw(GameManager& g, float dt) {
    BaseEntity::Draw(g, dt);
}

void BeatWallEntity::OnEditPick(GameManager& g) {
}
