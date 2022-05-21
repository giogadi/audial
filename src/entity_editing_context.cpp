#include "entity_editing_context.h"

#include "imgui/imgui.h"

#include "component.h"
#include "input_manager.h"
#include "entity_picking.h"
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

        TransformComponent const& cameraTransform =
            *(g._sceneManager->_cameras.front().lock()->_transform.lock());
        std::weak_ptr<Entity> pickedEntity = PickEntity(
            *g._entityManager, mouseX, mouseY, windowWidth, windowHeight, fovy, aspectRatio, zNear,
            cameraTransform);
        if (!pickedEntity.expired()) {
            _selectedEntity = pickedEntity;
            Entity const& entity = *_selectedEntity.lock();
            std::cout << "PICKED " << entity._name << std::endl;
            // Find the ix of this entity
            _selectedEntityIx = -1;
            for (int i = 0; i < g._entityManager->_entities.size(); ++i) {
                if (g._entityManager->_entities[i].get() == &entity) {
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
    if (_selectedEntity.expired()) {
        return;
    }
    auto transform = _selectedEntity.lock()->FindComponentOfType<TransformComponent>().lock();
    if (transform == nullptr) {
        _selectedEntity.reset();
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
    transform->SetPos(transform->GetPos() + inputVec.GetNormalized() * moveSpeed * dt);
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
        _selectedEntity = entities._entities[selectedIx];
    }
    // Now show a little panel for each component on the selected entity.    
    if (selectedIx < entities._entities.size() && selectedIx >= 0) {
        Entity& e = *entities._entities[*_selectedEntityIx];
        int const numComponents = e.GetNumComponents();
        for (int i = 0; i < numComponents; ++i) {
            Component& comp = e.GetComponent(i);
            char const* compName = ComponentTypeToString(comp.Type());
            if (ImGui::CollapsingHeader(compName)) {
                comp.DrawImGui();
            }
        }
    }
    ImGui::End();
}