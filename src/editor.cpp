#include "editor.h"

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "new_entity.h"
#include "input_manager.h"
#include "beat_clock.h"
#include "renderer.h"
#include "entity_picking.h"

static float gScrollFactorForDebugCameraMove = 1.f;

void Editor::Init(GameManager* g) {
    _g = g;
    _axesMesh = _g->_scene->GetMesh("axes");
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

double gClickX = 0.f;
double gClickY = 0.f;
bool gMovingObject = false;
}

void Editor::UpdateSelectedPositionFromInput(float dt) {
    ne::EntityManager& entities = *_g->_neEntityManager;
    ne::Entity* entity = entities.GetEntity(_selectedEntityId);
    if (entity == nullptr) {
        _selectedEntityId = ne::EntityId();
        return;
    }

    InputManager const& input = *_g->_inputManager;

    if (input.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        gMovingObject = false;
        input.GetMousePos(gClickX, gClickY);
    } else if (input.IsKeyPressed(InputManager::MouseButton::Left)) {
        double screenX, screenY;
        input.GetMousePos(screenX, screenY);
        if (!gMovingObject) {
            double screenX, screenY;
            input.GetMousePos(screenX, screenY);
            float const kMinDistBeforeMove = 0.01 * _g->_windowWidth;
            float dx = gClickX - screenX;
            float dy = gClickY - screenY;
            float distFromClick2 = dx*dx + dy*dy;
            if (distFromClick2 >= kMinDistBeforeMove*kMinDistBeforeMove) {
                gMovingObject = true;
            }
        }
        if (gMovingObject) {
            Vec3 pickedPosOnXZPlane;
            if (ProjectScreenPointToXZPlane(
                screenX, screenY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &pickedPosOnXZPlane)) {
                entity->_transform.SetTranslation(pickedPosOnXZPlane);
            }
        }
    } else {
        gMovingObject = false;
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
        Vec3 p = entity->_transform.GetPos() + inputVec.GetNormalized() * moveSpeed * dt;
        entity->_transform.SetTranslation(p);
    }    

    // // Update axes cursor
    // renderer::ColorModelInstance& m = _g->_scene->DrawMesh();
    // m._mesh = _axesModel;
    // m._transform = transform->GetWorldMat4();
    // m._topLayer = true;
}

void Editor::Update(float dt) {
    if (_g->_inputManager->IsKeyPressedThisFrame(InputManager::Key::I)) {
        _visible = !_visible;
    }
    
    double mouseX, mouseY;
    _g->_inputManager->GetMousePos(mouseX, mouseY);
    if (_g->_inputManager->IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        ne::Entity* picked = PickEntity(
            *_g->_neEntityManager, mouseX, mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera);
        if (picked == nullptr) {
            _selectedEntityId = ne::EntityId();
        } else {
            _selectedEntityId = picked->_id;
            picked->OnEditPick(*_g);
        }       
    }

    UpdateSelectedPositionFromInput(dt);

    // Use scroll input to move debug camera around.
    double scrollX = 0.0, scrollY = 0.0;
    _g->_inputManager->GetMouseScroll(scrollX, scrollY);
    if (scrollX != 0.0 || scrollY != 0.0) {
        Vec3 camMotionVec = gScrollFactorForDebugCameraMove * Vec3(-scrollX, 0.f, -scrollY);
        _g->_scene->_camera._transform.Translate(camMotionVec);
    }

    // Draw axes
    if (ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(_selectedEntityId)) {
        renderer::ColorModelInstance& axesModel = _g->_scene->DrawMesh();
        axesModel._mesh = _axesMesh;
        axesModel._topLayer = true;
        axesModel._transform = selectedEntity->_transform;
    }

    if (_visible) {
        DrawWindow();
    }
}

void Editor::DrawWindow() {
    ImGui::Begin("Editor");

    imgui_util::InputText<256>("Save filename", &_saveFilename);
    if (ImGui::Button("Save")) {
        serial::Ptree pt = serial::Ptree::MakeNew();
        serial::Ptree scriptPt = pt.AddChild("script");
        scriptPt.PutDouble("bpm", _g->_beatClock->GetBpm());
        serial::Ptree entitiesPt = scriptPt.AddChild("new_entities");
        for (ne::EntityId id : _entityIds) {
            ne::Entity* entity = _g->_neEntityManager->GetEntity(id);
            if (entity == nullptr) {
                continue;
            }
            assert(id._type == entity->_id._type);            
            serial::SaveInNewChildOf(entitiesPt, ne::gkEntityTypeNames[(int)id._type], *entity);
        }
        if (pt.WriteToFile(_saveFilename.c_str())) {
            printf("Saved script to \"%s\".\n", _saveFilename.c_str());
        } else {
            printf("FAILED to save script to \"%s\".\n", _saveFilename.c_str());
        }
    }

    // Entities  
    if (ImGui::BeginListBox("Entities")) {
        for (auto entityIdIter = _entityIds.begin(); entityIdIter != _entityIds.end(); /*no inc*/) {
            ne::EntityId& entityId = *entityIdIter;
            ne::Entity* entity = _g->_neEntityManager->GetEntity(entityId);

            // Clean up any missing entities
            if (entity == nullptr) {
                // printf("HOWDY: entity ID %d missing\n", entityId._id);
                entityIdIter = _entityIds.erase(entityIdIter);
                continue;
            }
            
            ImGui::PushID(entityId._id);
            bool selected = entityId._id == _selectedEntityId._id;
            bool clicked = ImGui::Selectable(entity->_name.c_str(), selected);
            if (clicked) {
                _selectedEntityId = entityId;
            }
            ImGui::PopID();

            ++entityIdIter;
        }
        ImGui::EndListBox();
    }    

    if (ImGui::BeginCombo("Entity types", ne::gkEntityTypeNames[_selectedEntityTypeIx])) {
        for (int typeIx = 0; typeIx < ne::gkNumEntityTypes; ++typeIx) {
            bool selected = typeIx == _selectedEntityTypeIx;
            bool clicked = ImGui::Selectable(ne::gkEntityTypeNames[typeIx], selected);
            if (clicked) {
                _selectedEntityTypeIx = typeIx;
            }
        }
        ImGui::EndCombo();
    }    

    // Add Entities
    if (ImGui::Button("Add Entity")) {
        ne::Entity* entity = _g->_neEntityManager->AddEntity((ne::EntityType)_selectedEntityTypeIx);
        entity->_name = "new_entity";
        _entityIds.push_back(entity->_id);
        entity->Init(*_g);
    }

    if (ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(_selectedEntityId)) {        
        if (ImGui::Button("Remove Entity")) {
            assert(_g->_neEntityManager->TagForDestroy(_selectedEntityId));
        }
    }

    if (ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(_selectedEntityId)) {
        if (selectedEntity->ImGui(*_g) == ne::Entity::ImGuiResult::NeedsInit) {
            selectedEntity->Destroy(*_g);
            selectedEntity->Init(*_g);
        }
    }

    ImGui::End();
}