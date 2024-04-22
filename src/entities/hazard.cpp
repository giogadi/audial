#include "entities/hazard.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "entities/resource.h"
#include "geometry.h"

void HazardEntity::SaveDerived(serial::Ptree pt) const {
    SeqAction::SaveActionsInChildNode(pt, "destroy_actions", _p.destroyActions);
}

void HazardEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    SeqAction::LoadActionsFromChildNode(pt, "destroy_actions", _p.destroyActions);
}

ne::BaseEntity::ImGuiResult HazardEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (SeqAction::ImGui("Destroy actions", _p.destroyActions)) {
        result = ImGuiResult::NeedsInit;
    }
    return result;
}

void HazardEntity::InitDerived(GameManager& g) {
    _s = State();
    for (auto const& pAction : _p.destroyActions) {
        pAction->Init(g);
    }
}

void HazardEntity::UpdateDerived(GameManager& g, float dt) {
    for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::Resource); !iter.Finished(); iter.Next()) {
        ResourceEntity* r = static_cast<ResourceEntity*>(iter.GetEntity());
        assert(r);
        bool overlap = geometry::IsPointInBoundingBox2d(r->_transform.Pos(), _transform);
        if (overlap) {
            r->StartDestroy(g);
            for (auto const& pAction : _p.destroyActions) {
                pAction->Execute(g);
            }
            break;
        }
    }

}

void HazardEntity::Draw(GameManager& g, float dt) {
    BaseEntity::Draw(g, dt);
}

void HazardEntity::OnEditPick(GameManager& g) {
}
