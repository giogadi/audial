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
            Entity& entity = *pickedEntity.lock();
            std::cout << "PICKED " << entity._name << std::endl;
            // Call each components' on-pick code.
            entity.OnEditPickComponents();
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

    UpdateSelectedPositionFromInput(dt, *g._inputManager, *g._entityManager);
}

void EntityEditingContext::UpdateSelectedPositionFromInput(float dt, InputManager const& input, EntityManager& entities) {
    std::shared_ptr<Entity> entity;
    if (_selectedEntityIx >= 0 && _selectedEntityIx < entities._entities.size()) {
        entity = entities._entities[_selectedEntityIx];
    } else {
        return;
    }
    auto transform = entity->FindComponentOfType<TransformComponent>().lock();
    if (transform == nullptr) {
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

    {
        // Saving
        char filenameBuffer[256];
        strcpy(filenameBuffer, _saveFilename.c_str());
        ImGui::InputText("Filename", filenameBuffer, 256);
        _saveFilename = filenameBuffer;

        if (ImGui::Button("Save")) {
            if (SaveEntities(_saveFilename.c_str(), entities)) {
                std::cout << "Saved entities to \"" << _saveFilename << "\"." << std::endl;
            }
        }
    }

    // Add Entity
    if (ImGui::Button("Add Entity")) {
        std::shared_ptr<Entity> newEntity = entities.AddEntity().lock();
        newEntity->_name = "new_entity";
        newEntity->AddComponentOrDie<TransformComponent>();
        newEntity->ConnectComponentsOrDie(g);
    }

    if (entities._entities.empty()) {
        ImGui::End();
        return;
    }
    std::vector<char const*> entityNames;
    entityNames.reserve(entities._entities.size());
    for (auto const& e : entities._entities) {
        entityNames.push_back(e->_name.c_str());
    }

    ImGui::ListBox("##", &_selectedEntityIx, entityNames.data(), /*numItems=*/entityNames.size());

    // Duplicate Entity
    if (_selectedEntityIx < entities._entities.size() && _selectedEntityIx >= 0) {
        if (ImGui::Button("Duplicate Entity")) {
            std::stringstream entityData;
            SaveEntity(entityData, *entities._entities[_selectedEntityIx]);
            std::shared_ptr<Entity> newEntity = entities.AddEntity().lock();
            LoadEntity(entityData, *newEntity);
            newEntity->_name += "(dupe)";
            newEntity->ConnectComponentsOrDeactivate(g, /*failures=*/nullptr);
        }
    }

    // Remove Entity
    if (_selectedEntityIx < entities._entities.size() && _selectedEntityIx >= 0) {        
        if (ImGui::Button("Remove Entity")) {
            entities.DestroyEntity(_selectedEntityIx);
            if (_selectedEntityIx >= entities._entities.size()) {
                _selectedEntityIx = -1;
            }
        }
    }

    // Now show a little panel for each component on the selected entity.
    if (_selectedEntityIx < entities._entities.size() && _selectedEntityIx >= 0) {        
        Entity& e = *entities._entities[_selectedEntityIx];

        char entityName[128];
        strcpy(entityName, e._name.c_str());
        ImGui::InputText("Entity name", entityName, 128);
        e._name = entityName;

        // Add Component
        if (ImGui::CollapsingHeader("Add Component")) {
            int constexpr numComponentTypes = static_cast<int>(ComponentType::NumTypes);
            ImGui::Combo("##", &_selectedComponentIx, gComponentTypeStrings, numComponentTypes);
            if (ImGui::Button("Add")) {
                ComponentType compType = static_cast<ComponentType>(_selectedComponentIx);
                std::weak_ptr<Component> pComp = e.TryAddComponentOfType(compType);
                if (pComp.expired()) {
                    std::cout << "Warning: entity \"" << e._name <<
                        "\" already contains a component of type \"" <<
                        gComponentTypeStrings[_selectedComponentIx] << "\"." << std::endl;
                } else {
                    e.ConnectComponentsOrDeactivate(g, /*failures=*/nullptr);
                }
            }
        }

        for (int i = 0; i < e.GetNumComponents(); ++i) {            
            Component& comp = e.GetComponent(i);
            char const* compName = ComponentTypeToString(comp.Type());
            if (ImGui::CollapsingHeader(compName)) {
                // TODO LMAO
                char buttonId[] = "DeleteXXX";
                sprintf(buttonId, "Delete%d", i);
                if (ImGui::Button(buttonId)) {
                    e.RemoveComponent(i);
                    e.ConnectComponentsOrDeactivate(g);
                    --i;
                    continue;
                }
                bool needsReconnect = comp.DrawImGui();
                if (needsReconnect) {
                    e.ConnectComponentsOrDeactivate(g, /*failures=*/nullptr);
                }
            }
        }
    }
    ImGui::End();
}