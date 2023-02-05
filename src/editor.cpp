#include "editor.h"

#include <map>

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "new_entity.h"
#include "input_manager.h"
#include "beat_clock.h"
#include "renderer.h"
#include "entity_picking.h"
#include "entities/typing_enemy.h"

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

} // end namespace

void Editor::HandleEntitySelectAndMove() {
    InputManager const& inputManager = *_g->_inputManager;
    
    static std::map<ne::EntityId, Vec3> sSelectedPositionsBeforeMove;
    double mouseX, mouseY;
    inputManager.GetMousePos(mouseX, mouseY);
    enum class ClickMode { Selection, Move };
    static ClickMode sClickMode = ClickMode::Selection;    
    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Tab)) {
        switch (sClickMode) {
            case ClickMode::Selection:
                sClickMode = ClickMode::Move;
                break;
            case ClickMode::Move: {
                sClickMode = ClickMode::Selection;
                // In case we were in the middle of moving some dudes, reset
                // their positions and clear it out
                for (auto const& idPosPair : sSelectedPositionsBeforeMove) {
                    if (ne::Entity* entity = _g->_neEntityManager->GetEntity(idPosPair.first)) {
                        entity->_transform.SetTranslation(idPosPair.second);
                    }
                }
                sSelectedPositionsBeforeMove.clear();
                break;
            }             
        }
    }
    switch (sClickMode) {
        case ClickMode::Selection: {
            if (inputManager.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
                ne::Entity* picked = PickEntity(
                    *_g->_neEntityManager, mouseX, mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera);
                if (picked) {
                    picked->OnEditPick(*_g);
                }
                if (inputManager.IsShiftPressed()) {
                    if (picked) {
                        // Toggle the selection status of the picked entity.
                        int numErased = _selectedEntityIds.erase(picked->_id);
                        if (numErased == 0) {
                            // Was unselected; so select it then!
                            _selectedEntityIds.insert(picked->_id);
                        }                       
                    } else {
                        // Do nothing if we clicked the air with shift held down
                    }
                } else {
                    if (picked) {
                        _selectedEntityIds.clear();
                        _selectedEntityIds.insert(picked->_id);
                    } else {
                        _selectedEntityIds.clear();
                    }
                }
            }
            break;
        }
        case ClickMode::Move: {            
            static Vec3 sClickedPosOnXZPlane;
            if (inputManager.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
                sSelectedPositionsBeforeMove.clear();
                ne::Entity* picked = PickEntity(
                    *_g->_neEntityManager, mouseX, mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera);
                if (picked) {
                    if (_selectedEntityIds.find(picked->_id) != _selectedEntityIds.end()) {
                        if (ProjectScreenPointToXZPlane(
                                mouseX, mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &sClickedPosOnXZPlane)) {                                                    
                            for (ne::EntityId selectedId : _selectedEntityIds) {
                                if (ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(selectedId)) {
                                    sSelectedPositionsBeforeMove.emplace(selectedId, selectedEntity->_transform.GetPos());
                                }
                            }
                        } else {
                            printf("WEIRD!!!! CLICKED POINT THAT DID NOT PROJECT TO XZ PLANE!\n");
                        }
                        
                    }
                }
            } else if (!sSelectedPositionsBeforeMove.empty()) {
                if (inputManager.IsKeyReleasedThisFrame(InputManager::MouseButton::Left)) {
                    sSelectedPositionsBeforeMove.clear();
                } else {
                    // we're moving things!!!
                    Vec3 mousePosOnXZPlane;
                    if (ProjectScreenPointToXZPlane(
                            mouseX, mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &mousePosOnXZPlane)) {
                        Vec3 offset = mousePosOnXZPlane - sClickedPosOnXZPlane;
                        for (auto const& idPosPair : sSelectedPositionsBeforeMove) {
                            if (ne::Entity* entity = _g->_neEntityManager->GetEntity(idPosPair.first)) {
                                Vec3 newPos = idPosPair.second + offset;
                                entity->_transform.SetTranslation(newPos);
                            }
                        }
                    } else {
                        printf("WEIRD!!! MOUSE MOTION POINT DID NOT PROJECT TO XZ PLANE!\n");
                    }
                }
            }
            break;
        }
    }

    // TODO if we add WSAD motion of objects back in, we liked a speed
    // multiplier of 3.0.
}

void Editor::Update(float dt) {
    if (!_g->_editMode) {
        return;
    }

    InputManager const& inputManager = *_g->_inputManager;
    
    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::I)) {
        _visible = !_visible;
    }

    static bool sMultiEnemyEditorVisible = false;
    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::M)) {
        sMultiEnemyEditorVisible = !sMultiEnemyEditorVisible;
    }

    HandleEntitySelectAndMove();

    // Use scroll input to move debug camera around.
    double scrollX = 0.0, scrollY = 0.0;
    _g->_inputManager->GetMouseScroll(scrollX, scrollY);
    if (scrollX != 0.0 || scrollY != 0.0) {
        Vec3 camMotionVec = gScrollFactorForDebugCameraMove * Vec3(-scrollX, 0.f, -scrollY);
        _g->_scene->_camera._transform.Translate(camMotionVec);
    }

    // Draw axes
    for (ne::EntityId selectedId : _selectedEntityIds) {
        ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(selectedId);
        if (selectedEntity == nullptr) {            
            continue;
        }
        renderer::ColorModelInstance& axesModel = _g->_scene->DrawMesh();
        axesModel._mesh = _axesMesh;
        axesModel._topLayer = true;
        // Factor out scale from transform
        Mat3 rotScale = selectedEntity->_transform.GetMat3();
        for (int i = 0; i < 3; ++i) {
            rotScale.GetCol(i).Normalize();
        }
        axesModel._transform.SetTopLeftMat3(rotScale);
        axesModel._transform.SetTranslation(selectedEntity->_transform.GetPos());
    }

    if (_visible) {
        DrawWindow();
    }

    if (sMultiEnemyEditorVisible) {
        DrawMultiEnemyWindow();
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
        pt.DeleteData();
    }

    // Entities  
    if (ImGui::BeginListBox("Entities")) {
        for (auto entityIdIter = _entityIds.begin(); entityIdIter != _entityIds.end(); /*no inc*/) {
            ne::EntityId& entityId = *entityIdIter;
            ne::Entity* entity = _g->_neEntityManager->GetEntity(entityId);

            // Clean up any missing entities
            if (entity == nullptr) {
                entityIdIter = _entityIds.erase(entityIdIter);
                continue;
            }
            
            ImGui::PushID(entityId._id);
            bool selected = _selectedEntityIds.find(entityId) != _selectedEntityIds.end();
            bool clicked = ImGui::Selectable(entity->_name.c_str(), selected);
            if (clicked) {
                if (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                    ImGui::IsKeyDown(ImGuiKey_RightShift)) {
                    int numErased = _selectedEntityIds.erase(entityId);
                    if (numErased == 0) {
                        _selectedEntityIds.insert(entityId);
                    }
                } else {
                    _selectedEntityIds = {entityId};
                }                
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
        // place entity at center of view
        Vec3 newPos;
        bool projectSuccess = ProjectScreenPointToXZPlane(_g->_windowWidth / 2, _g->_windowHeight / 2, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &newPos);
        if (!projectSuccess) {
            printf("Editor.AddEntity: Failed to project center to XZ plane!\n");
        }
        entity->_transform.SetTranslation(newPos);
        entity->Init(*_g);
    }
    
    // Duplicate entities
    if (ImGui::Button("Duplicate Entity")) {
        for (ne::EntityId selectedId : _selectedEntityIds) {
            if (!_g->_neEntityManager->GetEntity(selectedId)) {
                // NOTE: In the case that GetEntity() returns non-null, we DO
                // NOT STORE it because we're about to add an entity to the
                // manager. pointers are NOT STABLE after addition.
                continue;
            }

            ne::Entity* newEntity = _g->_neEntityManager->AddEntity(selectedId._type);
            assert(newEntity != nullptr);

            ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(selectedId);
            assert(selectedEntity != nullptr);

            {
                serial::Ptree pt = serial::Ptree::MakeNew();
                selectedEntity->Save(pt);
                newEntity->Load(pt);
                pt.DeleteData();
            }
            newEntity->_name += " (copy)";
            _entityIds.push_back(newEntity->_id);
            newEntity->Init(*_g);
        }
    }

    if (ImGui::Button("Remove Entity")) {
        for (ne::EntityId selectedId : _selectedEntityIds) {
            if (!_g->_neEntityManager->TagForDestroy(selectedId)) {
                printf("WEIRD: Remove Entity couldn't find the entity!\n");
            }
        }
    }

    if (_selectedEntityIds.size() == 1) {
        if (ne::Entity* selectedEntity = _g->_neEntityManager->GetEntity(*_selectedEntityIds.begin())) {
            if (selectedEntity->ImGui(*_g) == ne::Entity::ImGuiResult::NeedsInit) {
                selectedEntity->Destroy(*_g);
                selectedEntity->Init(*_g);
            }
        }
    } else {
        if (_selectedEntityIds.empty()) {
            ImGui::Text("(No selected entities)");
        } else {
            ImGui::Text("(Editing multiple entities unsupported)");
        }
    }
        
    ImGui::End();
}

void Editor::DrawMultiEnemyWindow() {
    ImGui::Begin("Multi Enemy Edit");

    static std::vector<TypingEnemyEntity*> sSelectedEnemies;
    sSelectedEnemies.clear();
    sSelectedEnemies.reserve(_selectedEntityIds.size());
    for (ne::EntityId eId : _selectedEntityIds) {
        if (eId._type == ne::EntityType::TypingEnemy) {
            if (ne::Entity* entity = _g->_neEntityManager->GetEntity(eId)) {
                sSelectedEnemies.push_back(static_cast<TypingEnemyEntity*>(entity));
            }
        }
    }
    TypingEnemyEntity::MultiSelectImGui(*_g, sSelectedEnemies);
    
    ImGui::End();
}
