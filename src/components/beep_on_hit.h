#pragma once

#include <array>
#include <iostream>

#include "component.h"

class BeatClock;
namespace audio {
    struct Context;
}
class RigidBodyComponent;
class TransformComponent;

class BeepOnHitComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::BeepOnHit; }
    BeepOnHitComponent() {
        _midiNotes.fill(-1);
    }
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;

    static void OnHit(
        std::weak_ptr<BeepOnHitComponent> beepComp, std::weak_ptr<RigidBodyComponent> other);

    virtual void Update(float const dt) override;

    virtual void Destroy() override {}

    virtual bool DrawImGui() override;

    virtual void OnEditPick() override;

    // void Save(ptree& pt) const override {
    //     std::cout << "BeepOnHitComponent::Save: UNIMPLEMENTED" << std::endl;
    // }
    // void Load(ptree const& pt) override {
    //     std::cout << "BeepOnHitComponent::Load: UNIMPLEMENTED" << std::endl;
    // }

    std::weak_ptr<TransformComponent> _t;
    std::weak_ptr<RigidBodyComponent> _rb;
    audio::Context* _audio = nullptr;
    BeatClock const* _beatClock = nullptr;
    int _synthChannel = 0;
    std::array<int, 4> _midiNotes;
    double _lastScheduledEvent = -1.0;
    bool _wasHit = false;

private:
    void PlayNotesOnNextDenom(double denom);
};