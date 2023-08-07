#pragma once

#include "new_entity.h"
#include "seq_action.h"
#include "renderer.h"
#include "random_wander.h"
#include "enums/WaypointFollowerMode.h"

struct FlowWallEntity : public ne::Entity {
    // serialized
    WaypointFollowerMode _moveMode;
    RandomWander _randomWander;
    std::vector<std::unique_ptr<SeqAction>> _hitActions;
    std::vector<Vec3> _polygon;
    float _rotVel = 0.f;
    int _maxHp = -1;
    bool _canHit = true;

    // non-serialized
    double _timeOfLastHit = -1.0;
    Vec4 _currentColor;
    int _hp = -1;
    renderer::Scene::MeshId _meshId;

    void OnHit(GameManager& g);
       
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    FlowWallEntity() = default;
    FlowWallEntity(FlowWallEntity const&) = delete;
    FlowWallEntity(FlowWallEntity&&) = default;
    FlowWallEntity& operator=(FlowWallEntity&&) = default;
};
