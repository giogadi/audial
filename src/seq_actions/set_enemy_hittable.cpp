#include "set_enemy_hittable.h"

#include "imgui_util.h"
#include "entities/typing_enemy.h"

void SetEnemyHittableSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("entity_tag", _entityTag);
    serial::SaveInNewChildOf(pt, "editor_id", _editorId);
    pt.PutBool("hittable", _hittable);
}

void SetEnemyHittableSeqAction::LoadDerived(serial::Ptree pt) {
    _hittable = false;
    _entityTag = -1;
    _entityTag = pt.GetInt("entity_tag");
    _editorId = EditorId();
    serial::LoadFromChildOf(pt, "editor_id", _editorId);
    _hittable = pt.GetBool("hittable");
}

void SetEnemyHittableSeqAction::LoadDerived(LoadInputs const& loadInputs, std::istream& input) {
    input >> _entityTag >> _hittable;
}

bool SetEnemyHittableSeqAction::ImGui() {
    ImGui::InputInt("Entity tag", &_entityTag);
    imgui_util::InputEditorId("Entity editor ID", &_editorId);
    ImGui::Checkbox("Hittable", &_hittable);
    return false;
}

void SetEnemyHittableSeqAction::InitDerived(GameManager& g) {
    _entityId = ne::EntityId();
    if (ne::Entity* e = g._neEntityManager->FindEntityByEditorIdAndType(_editorId, ne::EntityType::TypingEnemy)) {
        _entityId = e->_id;
    }
}

void SetEnemyHittableSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    std::vector<ne::Entity*> enemies;
    if (_entityTag >= 0) {
        g._neEntityManager->FindEntitiesByTagAndType(_entityTag, ne::EntityType::TypingEnemy, true, true, &enemies);
    }    
    if (ne::Entity* e = g._neEntityManager->GetEntity(_entityId)) {
        enemies.push_back(e);
    }
    for (ne::Entity* entity : enemies) {
        TypingEnemyEntity* e = (TypingEnemyEntity*) entity;
        e->SetHittable(_hittable);
    }
}
