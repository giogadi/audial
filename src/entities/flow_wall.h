#pragma once

#include "new_entity.h"
#include "seq_action.h"
#include "renderer.h"
#include "random_wander.h"
#include "enums/WaypointFollowerMode.h"

struct FlowWallEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::FlowWall; }
    static ne::EntityType StaticType() { return ne::EntityType::FlowWall; }
    
    // serialized
    WaypointFollowerMode _moveMode;
    RandomWander _randomWander;
    std::vector<std::unique_ptr<SeqAction>> _hitActions;
    std::vector<Vec3> _polygon;
    float _rotVel = 0.f;
    int _maxHp = -1;
    bool _canHit = true;
    std::vector<EditorId> _childrenEditorIds;  // for making other things bounce

    // non-serialized
    double _timeOfLastHit = -1.0;
    Vec3 _lastHitDirection;
    Vec4 _currentColor;
    int _hp = -1;
    renderer::Scene::MeshId _meshId;
    std::vector<ne::EntityId> _children;

    void OnHit(GameManager& g, Vec3 const& hitDirection);
       
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
