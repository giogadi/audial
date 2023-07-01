#include "set_enemy_hittable.h"

#include "imgui_util.h"
#include "entities/typing_enemy.h"

void SetEnemyHittableSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("entity_tag", _entityTag);
    pt.PutBool("hittable", _hittable);
}

void SetEnemyHittableSeqAction::LoadDerived(serial::Ptree pt) {
    _hittable = false;
    _entityTag = -1;
    _entityTag = pt.GetInt("entity_tag");
    _hittable = pt.GetBool("hittable");
}

void SetEnemyHittableSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityTag >> _hittable;
}

bool SetEnemyHittableSeqAction::ImGui() {
    ImGui::InputInt("Entity tag", &_entityTag);
    ImGui::Checkbox("Hittable", &_hittable);
    return false;
}

void SetEnemyHittableSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    std::vector<ne::Entity*> enemies;
    g._neEntityManager->FindEntitiesByTagAndType(_entityTag, ne::EntityType::TypingEnemy, true, true, &enemies);
    for (ne::Entity* entity : enemies) {
        TypingEnemyEntity* e = (TypingEnemyEntity*) entity;
        e->SetHittable(_hittable);
    }
}
