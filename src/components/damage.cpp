#include "damage.h"

#include "imgui/imgui.h"

#include "rigid_body.h"
#include "entity_manager.h"
#include "game_manager.h"
#include "components/events_on_hit.h"

bool DamageComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    auto const onHit = e.FindComponentOfType<EventsOnHitComponent>().lock();
    if (onHit == nullptr) {
        return false;
    }
    onHit->AddOnHitCallback(std::bind(&DamageComponent::OnHit, this, std::placeholders::_1));

    _entityManager = g._entityManager;
    _entityId = id;
    return true;
}

void DamageComponent::OnHit(EntityId other) {
    _wasHit = true;
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