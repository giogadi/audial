#include "entity_editing_context.h"

#include "imgui/imgui.h"

#include "component.h"
#include "input_manager.h"
#include "transform_manager.h"
#include "renderer.h"

void EntityEditingContext::Update(
    float dt, bool editMode, GameManager const& g, int windowWidth, int windowHeight) {
    // if (!editMode) {
    //     return;
    // }

    double mouseX, mouseY;
    g._inputManager->GetMousePos(mouseX, mouseY);
    if (g._inputManager->IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        // TODO: ENSURE RENDERER USES THESE SAME VALUES
        float const fovy = 45.f;
        float const aspectRatio = float(windowWidth) / float(windowHeight);
        float const zNear = 0.1f;

        std::optional<std::pair<TransformComponent*, Entity*>> pickedEntity =
            g._transformManager->PickEntity(mouseX, mouseY, windowWidth, windowHeight, fovy, 
                aspectRatio, zNear, *(g._sceneManager->_cameras.front()->_transform));
        if (pickedEntity.has_value()) {
            std::cout << "PICKED " << pickedEntity->second->_name << std::endl;
            _selectedTransform = pickedEntity->first;
            _selectedEntity = pickedEntity->second;            
            // Find the ix of this entity
            _selectedEntityIx = -1;
            for (int i = 0; i < g._entityManager->_entities.size(); ++i) {
                if (g._entityManager->_entities[i].get() == _selectedEntity) {
                    _selectedEntityIx = i;
                    break;
                }
            }
            if (_selectedEntityIx == -1) {
                std::cout << "WTF Why can't we find the entity we just picked?" << std::endl;
                assert(false);
            }
        }
    }

    UpdateSelectedPositionFromInput(dt, *g._inputManager);
}

void EntityEditingContext::UpdateSelectedPositionFromInput(float dt, InputManager const& input) {
    if (_selectedEntity == nullptr) {
        return;
    }

    Vec3 inputVec(0.0f,0.f,0.f);
    if (input.IsKeyPressed(InputManager::Key::W)) {
        inputVec._z -= 1.0f;
    }
    if (input.IsKeyPressed(InputManager::Key::S)) {
        inputVec._z += 1.0f;
    }
    if (input.IsKeyPressed(InputManager::Key::A)) {
        inputVec._x -= 1.0f;
    }
    if (input.IsKeyPressed(InputManager::Key::D)) {
        inputVec._x += 1.0f;
    }

    if (inputVec._x == 0.f && inputVec._y == 0.f && inputVec._z == 0.f) {
        return;
    }

    float moveSpeed = 3.f;
    _selectedTransform->SetPos(_selectedTransform->GetPos() + inputVec.GetNormalized() * moveSpeed * dt);
}

void EntityEditingContext::DrawEntitiesWindow(EntityManager& entities, GameManager& g) {
    ImGui::Begin("Entities");
    if (entities._entities.empty()) {
        ImGui::End();
        return;
    }
    std::vector<char const*> entityNames;
    entityNames.reserve(entities._entities.size());
    for (auto const& e : entities._entities) {
        entityNames.push_back(e->_name.c_str());
    }
    int selectedIx = _selectedEntityIx.has_value() ? *_selectedEntityIx : 0;
    ImGui::ListBox("##", &selectedIx, entityNames.data(), /*numItems=*/entityNames.size());
    {
        _selectedEntityIx = selectedIx;
        _selectedEntity = entities._entities[selectedIx].get();
        _selectedTransform = _selectedEntity->DebugFindComponentOfType<TransformComponent>();
    }
    // Now show a little panel for each component on the selected entity.    
    if (selectedIx < entities._entities.size() && selectedIx >= 0) {
        Entity const& e = *entities._entities[*_selectedEntityIx];
        for (auto const& cComp : e._components) {
            char const* compName = ComponentTypeToString(cComp->Type());
            if (ImGui::CollapsingHeader(compName)) {
                cComp->DrawImGui();
            }
        }
    }
    ImGui::End();
}