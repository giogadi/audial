#include "damage.h"

#include "imgui/imgui.h"

#include "rigid_body.h"

bool DamageComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _rb = e.FindComponentOfType<RigidBodyComponent>();
    if (_rb.expired()) {
        return false;
    }
    auto pThisComp = e.FindComponentOfType<DamageComponent>();
    RigidBodyComponent& rb = *_rb.lock();
    rb.AddOnHitCallback(std::bind(&DamageComponent::OnHit, pThisComp, std::placeholders::_1));

    _entityManager = g._entityManager;
    _entityId = id;
    return true;
}

void DamageComponent::OnHit(std::weak_ptr<DamageComponent> damageComp, std::weak_ptr<RigidBodyComponent> other) {
    damageComp.lock()->_wasHit = true;
}

void DamageComponent::Update(float const dt) {
    if (_wasHit) {
        _hp -= 1;
        if (_hp == 0) {
            _entityManager->TagEntityForDestroy(_entityId);
        }
        _wasHit = false;
    }
}

bool DamageComponent::DrawImGui() {
    ImGui::InputScalar("HP##", ImGuiDataType_S32, &_hp, /*step=*/nullptr, /*???*/nullptr, "%d");
    return false;
}

void DamageComponent::Save(ptree& pt) const {
    pt.put("hp", _hp);
}

void DamageComponent::Load(ptree const& pt) {
    _hp = pt.get<int>("hp");
}