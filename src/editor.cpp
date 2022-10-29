#include "editor.h"

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "new_entity.h"
#include "input_manager.h"
#include "beat_clock.h"
#include "renderer.h"

void Editor::Init(GameManager* g) {
    _g = g;
    _axesMesh = _g->_scene->GetMesh("axes");
}

void Editor::Update() {
    if (_g->_inputManager->IsKeyPressedThisFrame(InputManager::Key::I)) {
        _visible = !_visible;
    }
    
    // double mouseX, mouseY;
    // _g->_inputManager->GetMousePos(mouseX, mouseY);
    // if (_g->_inputManager->IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
    //     _selectedEntityId = PickEntity(
    //         *g._entityManager, mouseX, mouseY, g._windowWidth, g._windowHeight, g._scene->_camera);
    //     if (_selectedEntityId.IsValid()) {
    //         Entity& entity = *g._entityManager->GetEntity(_selectedEntityId);
    //         // std::cout << "PICKED " << entity._name << std::endl;
    //         // Call each components' on-pick code.
    //         entity.OnEditPickComponents();
    //     }
    // }

    // UpdateSelectedPositionFromInput(dt, g);

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