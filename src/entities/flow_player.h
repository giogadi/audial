#pragma once

#include <deque>

#include "new_entity.h"

struct FlowPlayerEntity : public ne::Entity {
    // serialized
    float _selectionRadius = -1.f;  // ignored if < 0
    float _launchVel = 4.f; // > 0
    float _decel = 2.f;  // > 0

    // non-serialized
    Vec3 _vel;
    // TODO: use a ring buffer instead
    std::deque<Vec3> _posHistory;  // start is most recent
    int _framesSinceLastSample = 0;
    
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};
