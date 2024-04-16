#include "entities/test.h"

#include <imgui/imgui.h>

#include "game_manager.h"

void TestEntity::SaveDerived(serial::Ptree pt) const {
}

void TestEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
}

ne::BaseEntity::ImGuiResult TestEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}

void TestEntity::InitDerived(GameManager& g) {
    _s = State();
}

void TestEntity::UpdateDerived(GameManager& g, float dt) {
}

void TestEntity::Draw(GameManager& g, float dt) {
    BaseEntity::Draw(g, dt);
}

void TestEntity::OnEditPick(GameManager& g) {
}
