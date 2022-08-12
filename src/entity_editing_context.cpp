#include "entity_editing_context.h"

#include <iostream>

#include "imgui/imgui.h"

#include "serial.h"
#include "matrix.h"
#include "entity_manager.h"
#include "game_manager.h"
#include "input_manager.h"
#include "entity_picking.h"
#include "renderer.h"
#include "constants.h"
#include "components/transform.h"
#include "components/camera.h"

static float gScrollFactorForDebugCameraMove = 1.f;

bool EntityEditingContext::Init(GameManager& g) {
    _axesModel = g._scene->GetMesh("axes");
    assert(_axesModel != nullptr);
    // Initialize the debug camera's position by looking for an arbitrary camera
    // component in the Scene.
    g._entityManager->ForEveryActiveEntity([&g](EntityId eId) {
        Entity* entity = g._entityManager->GetEntity(eId);
        assert(entity != nullptr);
        std::shared_ptr<CameraComponent> camComp = entity->FindComponentOfType<CameraComponent>().lock();
        if (camComp == nullptr) {
            return;
        }
        std::shared_ptr<TransformComponent> camTransComp = camComp->_transform.lock();
        assert(camTransComp != nullptr);
        g._scene->_camera._transform = camTransComp->GetWorldMat4();
    });
    return true;
}

void EntityEditingContext::Update(
    float dt, bool editMode, GameManager& g) {
    if (!editMode) {
        return;
    }

    double mouseX, mouseY;
    g._inputManager->GetMousePos(mouseX, mouseY);
    if (g._inputManager->IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        _selectedEntityId = PickEntity(
            *g._entityManager, mouseX, mouseY, g._windowWidth, g._windowHeight, g._scene->_camera);
        if (_selectedEntityId.IsValid()) {
            Entity& entity = *g._entityManager->GetEntity(_selectedEntityId);
            // std::cout << "PICKED " << entity._name << std::endl;
            // Call each components' on-pick code.
            entity.OnEditPickComponents();
        }
    }

    UpdateSelectedPositionFromInput(dt, g);

    // Use scroll input to move debug camera around.
    double scrollX = 0.0, scrollY = 0.0;
    g._inputManager->GetMouseScroll(scrollX, scrollY);
    if (scrollX != 0.0 || scrollY != 0.0) {
        Vec3 camMotionVec = gScrollFactorForDebugCameraMove * Vec3(-scrollX, 0.f, -scrollY);
        g._scene->_camera._transform.Translate(camMotionVec);
    }

    // // If no entity is selected, update debug camera from user input.
    // if (!_selectedEntityId.IsValid()) {
    //     InputManager const& input = *g._inputManager;

    //     Vec3 inputVec(0.0f,0.f,0.f);
    //     if (input.IsKeyPressed(InputManager::Key::W)) {
    //         inputVec._z -= 1.0f;
    //     }
    //     if (input.IsKeyPressed(InputManager::Key::S)) {
    //         inputVec._z += 1.0f;
    //     }
    //     if (input.IsKeyPressed(InputManager::Key::A)) {
    //         inputVec._x -= 1.0f;
    //     }
    //     if (input.IsKeyPressed(InputManager::Key::D)) {
    //         inputVec._x += 1.0f;
    //     }

    //     bool hasInput = inputVec._x != 0.f || inputVec._y != 0.f || inputVec._z != 0.f;
    //     if (hasInput) {
    //         float moveSpeed = 3.f;
    //         g._scene->_camera._transform.Translate(inputVec.GetNormalized() * moveSpeed * dt);
    //     }
    // }
}

namespace {
bool ProjectScreenPointToXZPlane(
    int screenX, int screenY, int windowWidth, int windowHeight, renderer::Camera const& camera, Vec3* outPoint) {
    Vec3 rayStart, rayDir;
    GetPickRay(screenX, screenY, windowWidth, windowHeight, camera, &rayStart, &rayDir);

    // Find where the ray intersects the XZ plane. This is where on the ray the y-value hits 0.
    if (std::abs(rayDir._y) > 0.0001) {
        float rayParamAtXZPlane = -rayStart._y / rayDir._y;
        if (rayParamAtXZPlane > 0.0) {
            *outPoint = rayStart + (rayDir * rayParamAtXZPlane);
            return true;
        }            
    }

    return false;
}
}

void EntityEditingContext::UpdateSelectedPositionFromInput(float dt, GameManager& g) {
    EntityManager& entities = *g._entityManager;
    Entity* entity = entities.GetEntity(_selectedEntityId);
    if (entity == nullptr) {
        _selectedEntityId = EntityId::InvalidId();
        return;
    }

    auto transform = entity->FindComponentOfType<TransformComponent>().lock();
    if (transform == nullptr) {
        return;
    }

    InputManager const& input = *g._inputManager;

    // If user is dragging mouse around, use mouse motion to move object.
    if (input.IsKeyPressed(InputManager::MouseButton::Left)) {
        // Can we translate mouse position to world position? Well, the "simple"
        // way is to just project the mouse position onto the XZ plane.
        double screenX, screenY;
        input.GetMousePos(screenX, screenY);
        Vec3 pickedPosOnXZPlane;
        if (ProjectScreenPointToXZPlane(screenX, screenY, g._windowWidth, g._windowHeight, g._scene->_camera, &pickedPosOnXZPlane)) {
            transform->SetWorldPos(pickedPosOnXZPlane);
        }
        // Vec3 rayStart, rayDir;
        // GetPickRay(screenX, screenY, windowWidth, windowHeight, g._scene->_camera, &rayStart, &rayDir);

        // // Find where the ray intersects the XZ plane. This is where on the ray the y-value hits 0.
        // if (std::abs(rayDir._y) > 0.0001) {
        //     float rayParamAtXZPlane = -rayStart._y / rayDir._y;
        //     if (rayParamAtXZPlane > 0.0) {
        //         Vec3 projOnXZPlane = rayStart + (rayDir * rayParamAtXZPlane);
        //         transform->SetWorldPos(projOnXZPlane);
        //     }            
        // }
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

    bool hasInput = inputVec._x != 0.f || inputVec._y != 0.f || inputVec._z != 0.f;
    if (hasInput) {
        float moveSpeed = 3.f;
        transform->SetWorldPos(transform->GetWorldPos() + inputVec.GetNormalized() * moveSpeed * dt);
    }    

    // Update axes cursor
    renderer::ColorModelInstance& m = g._scene->DrawMesh();
    m._mesh = _axesModel;
    m._transform = transform->GetWorldMat4();
    m._topLayer = true;
}

void EntityEditingContext::DrawEntityImGui(EntityId id, GameManager& g, int* selectedComponentIx, bool connectComponents) {
    Entity& e = *g._entityManager->GetEntity(id);
    char entityName[128];
    strcpy(entityName, e._name.c_str());
    ImGui::InputText("Entity name", entityName, 128);
    e._name = entityName;

    // Add Component
    if (ImGui::CollapsingHeader("Add Component")) {
        int constexpr numComponentTypes = static_cast<int>(ComponentType::NumTypes);
        ImGui::Combo("##Components", selectedComponentIx, gComponentTypeStrings, numComponentTypes);
        if (ImGui::Button("Add")) {
            ComponentType compType = static_cast<ComponentType>(*selectedComponentIx);
            std::weak_ptr<Component> pComp = e.TryAddComponentOfType(compType);
            if (pComp.expired()) {
                std::cout << "Warning: entity \"" << e._name <<
                    "\" already contains a component of type \"" <<
                    gComponentTypeStrings[*selectedComponentIx] << "\"." << std::endl;
            } else if (connectComponents) {
                e.ConnectComponentsOrDeactivate(id, g, /*failures=*/nullptr);
            }
        }
    }

    for (int i = 0; i < e.GetNumComponents(); ++i) {
        Component& comp = e.GetComponent(i);
        char const* compName = ComponentTypeToString(comp.Type());
        if (ImGui::CollapsingHeader(compName)) {
            ImGui::PushID(i);
            if (ImGui::Button("Delete##")) {
                e.RemoveComponent(i);
                if (connectComponents) {
                    e.ConnectComponentsOrDeactivate(id, g);
                }
                --i;
                ImGui::PopID();
                continue;
            }
            bool needsReconnect = comp.DrawImGui();
            if (needsReconnect && connectComponents) {
                e.ConnectComponentsOrDeactivate(id, g, /*failures=*/nullptr);
            }
            ImGui::PopID();
        }
    }
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
            if (entities.Save(_saveFilename.c_str())) {
                std::cout << "Saved entities to \"" << _saveFilename << "\"." << std::endl;
            }
        }
    }

    // Add Entity
    if (ImGui::Button("Add Entity")) {
        EntityId newId = entities.AddEntityToBack();
        Entity& newEntity = *entities.GetEntity(newId);
        newEntity._name = "new_entity";
        {
            std::shared_ptr<TransformComponent> t = newEntity.AddComponentOrDie<TransformComponent>().lock();
            // Place new entity at center of current debug camera view.
            Vec3 pos(0.f, 0.f, 0.f);
            ProjectScreenPointToXZPlane(g._windowWidth / 2, g._windowHeight / 2, g._windowWidth, g._windowHeight, g._scene->_camera, &pos);
            t->SetWorldPos(pos);
        }
        newEntity.ConnectComponentsOrDie(newId, g);
        _selectedEntityId = newId;
    }

    int numEntities = 0;
    entities.ForEveryActiveAndInactiveEntity([&numEntities](EntityId id) {
        ++numEntities;
    });

    if (numEntities == 0) {
        ImGui::End();
        return;
    }


    std::vector<std::string> entityNames;
    std::vector<char const*> entityNamesCstr;
    std::vector<EntityId> entityIds;
    entityNames.reserve(numEntities);
    entityNamesCstr.reserve(numEntities);
    int entityIndex = 0;
    int selectedEntityIndex = -1;
    entities.ForEveryActiveAndInactiveEntity([&](EntityId id) {
        Entity const& e = *entities.GetEntity(id);
        if (entities.IsActive(id)) {
            entityNames.push_back(e._name);
        } else {
            entityNames.push_back(e._name + " (inactive)");
        }
        entityNamesCstr.push_back(entityNames.back().c_str());
        if (_selectedEntityId == id) {
            selectedEntityIndex = entityIndex;
        }
        ++entityIndex;
        entityIds.push_back(id);
    });

    if (selectedEntityIndex < 0) {
        _selectedEntityId = EntityId::InvalidId();
    }

    bool selectedChanged = ImGui::ListBox("##Entities", &selectedEntityIndex, entityNamesCstr.data(), /*numItems=*/entityNamesCstr.size());
    if (selectedChanged && selectedEntityIndex >= 0) {
        _selectedEntityId = entityIds[selectedEntityIndex];
    }

    // Active/inactive
    if (_selectedEntityId.IsValid()) {
        bool active = entities.IsActive(_selectedEntityId);
        bool changed = ImGui::Checkbox("Active", &active);
        if (changed) {
            if (active) {
                entities.ActivateEntity(_selectedEntityId, g);
            } else {
                entities.DeactivateEntity(_selectedEntityId);
            }
        }
    }

    // Duplicate Entity
    if (_selectedEntityId.IsValid()) {
        if (ImGui::Button("Duplicate Entity")) {
            Entity const& entity = *entities.GetEntity(_selectedEntityId);
            ptree entityData;
            entity.Save(entityData);
            EntityId newEntityId = entities.AddEntityToBack(entities.IsActive(_selectedEntityId));
            Entity& newEntity = *entities.GetEntity(newEntityId);
            newEntity.Load(entityData);
            newEntity._name += "(dupe)";
            if (entities.IsActive(newEntityId)) {
                newEntity.ConnectComponentsOrDeactivate(newEntityId, g, /*failures=*/nullptr);
            }
        }
    }

    // Remove Entity
    if (_selectedEntityId.IsValid()) {
        if (ImGui::Button("Remove Entity")) {
            entities.TagEntityForDestroy(_selectedEntityId, EntityDestroyType::EditDestroyComponents);
            _selectedEntityId = EntityId::InvalidId();
        }
    }

    // Save Entity as prefab
    if (_selectedEntityId.IsValid()) {
        char filename[256];
        strncpy(filename, _prefabFilename.c_str(), 255);
        bool changed = ImGui::InputText("Prefab filename##FilenamePrefab", filename, 255);
        if (changed) {
            _prefabFilename = filename;
        }

        if (ImGui::Button("Save Prefab##")) {
            Entity const& entity = *entities.GetEntity(_selectedEntityId);
            entity.Save(_prefabFilename.c_str());
        }
        if (ImGui::Button("Load Prefab##")) {
            Entity& e = *entities.GetEntity(_selectedEntityId);
            e.EditDestroy();
            e.Load(_prefabFilename.c_str());
            if (entities.IsActive(_selectedEntityId)) {
                e.ConnectComponentsOrDeactivate(_selectedEntityId, g, /*failures=*/nullptr);
            }
        }
    }

    // Now show a little panel for each component on the selected entity.
    if (_selectedEntityId.IsValid()) {
        bool active = entities.IsActive(_selectedEntityId);
        DrawEntityImGui(_selectedEntityId, g, &_selectedComponentIx, /*connectComponents=*/active);
    }

    ImGui::End();
}