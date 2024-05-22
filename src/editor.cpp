#include "editor.h"

#include <map>
#include <cctype>
#include <sstream>
#include <unordered_set>

#include "imgui/imgui.h"
#include "imgui_util.h"
#include "new_entity.h"
#include "input_manager.h"
#include "beat_clock.h"
#include "renderer.h"
#include "entity_picking.h"
#include "entities/typing_enemy.h"
#include "util.h"
#include "string_util.h"
#include "color_presets.h"
#include "geometry.h"
#include "seq_action.h"

static float gScrollFactorForDebugCameraMove = 1.f;

static std::vector<ne::EntityId> gControlGroups[10];

void Editor::Save(serial::Ptree pt) const {
    serial::Ptree groupsPt = pt.AddChild("ctrl_groups");
    std::stringstream ss;
    for (int ctrlGroupIx = 0; ctrlGroupIx <= 9; ++ctrlGroupIx) {
        ss.str("");
        ss << "group" << ctrlGroupIx;
        std::string groupName = ss.str();
        serial::Ptree groupPt = groupsPt.AddChild(groupName.c_str());
        std::vector<ne::EntityId> const& ctrlGroup = gControlGroups[ctrlGroupIx];
        for (ne::EntityId eId : ctrlGroup) {
            if (ne::Entity* e = _g->_neEntityManager->GetActiveOrInactiveEntity(eId)) {
                serial::SaveInNewChildOf(groupPt, "editor_id", e->_editorId);
            }
        }
    }
}

void Editor::Load(serial::Ptree pt) {
    serial::Ptree groupsPt = pt.TryGetChild("ctrl_groups");
    if (!groupsPt.IsValid()) {
        return;
    }
    int numChildrenGroups = 0;
    serial::NameTreePair* childrenGroups = groupsPt.GetChildren(&numChildrenGroups);
    for (int groupIx = 0; groupIx < numChildrenGroups; ++groupIx) {
        int numChildrenEntities = 0;
        serial::Ptree* groupPt = &childrenGroups[groupIx]._pt;
        serial::NameTreePair* childrenEntities = groupPt->GetChildren(&numChildrenEntities);
        auto& ctrlGroup = gControlGroups[groupIx];
        ctrlGroup.clear();
        for (int entityIx = 0; entityIx < numChildrenEntities; ++entityIx) {
            serial::Ptree* eIdPt = &childrenEntities[entityIx]._pt;
            EditorId editorId;
            editorId.Load(*eIdPt);
            if (ne::Entity* entity = _g->_neEntityManager->FindEntityByEditorId(editorId)) {
                ctrlGroup.push_back(entity->_id);
            }
        }
        delete[] childrenEntities;
    }
    delete[] childrenGroups;
}

void Editor::Init(GameManager* g) {
    _g = g;
    _axesMesh = _g->_scene->GetMesh("axes");
    // Collect all our entities, and init the inactive ones (Active ones should
    // already have been init). For example, this sets all dudes' transforms to
    // their initial transforms.
    std::unordered_set<int64_t> editorIds;
    for (ne::EntityManager::AllIterator iter = g->_neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
        if (ne::Entity* e = iter.GetEntity()) {
            // no init necessary
            _entityIds.push_back(e->_id);
            if (e->_editorId._id >= 0) {
                _nextEditorId = std::max(_nextEditorId, e->_editorId._id + 1);
                auto result = editorIds.insert(e->_editorId._id);
                assert(result.second);
            }
        }
    }
    for (ne::EntityManager::AllIterator iter = g->_neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
        if (ne::Entity* e = iter.GetEntity()) {
            e->Init(*g);
            _entityIds.push_back(e->_id);
            if (e->_editorId._id >= 0) {
                _nextEditorId = std::max(_nextEditorId, e->_editorId._id + 1);
                auto result = editorIds.insert(e->_editorId._id);
                assert(result.second);
            }
        }
    }

    // sort entity IDs. This should result in the ordering that they were listed in the input xml.
    std::sort(_entityIds.begin(), _entityIds.end());
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

enum class InputMode {
    Default,
    Piano,
    Count
};

InputMode sInputMode = InputMode::Default;
bool sSynthWindowVisible = false;
bool sDemoWindowVisible = false;
bool sMultiEnemyEditorVisible = false;

} // end namespace

void Editor::PickEntity(ne::BaseEntity* entity) {
    if (entity->Type() == ne::EntityType::TypingEnemy) {
        TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(entity);
        HitResult result = enemy->OnHit(*_g);
        if (!_heldActions.empty()) {
            for (auto const& a : _heldActions) {
                a->ExecuteRelease(*_g);
            }
        }
        _heldActions = std::move(result._heldActions);
        _heldActionsTimer = 0.f;
    } else {
        entity->OnEditPick(*_g);
    }    
}

void Editor::HandleEntitySelectAndMove(float deltaTime) {
    InputManager const& inputManager = *_g->_inputManager;

    static bool sGrabMode = false;
    static Vec3 sGrabPos;

    if (sGrabMode) {
        double mouseX, mouseY;
        inputManager.GetMousePos(mouseX, mouseY);
        Vec3 clickedPosOnXZPlane;
        if (ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &clickedPosOnXZPlane)) {
            Vec3 grabOffset = clickedPosOnXZPlane - sGrabPos;
            for (ne::EntityId selectedId : _selectedEntityIds) {
                if (ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                    Vec3 newPos = selectedEntity->_initTransform.Pos() + grabOffset;
                    selectedEntity->_transform.SetPos(newPos);
                }
            }
        }

        if (inputManager.IsKeyReleasedThisFrame(InputManager::Key::Enter) ||
            inputManager.IsKeyReleasedThisFrame(InputManager::Key::G) ||
            inputManager.IsKeyReleasedThisFrame(InputManager::MouseButton::Left)) {
            // commit transforms
            for (ne::EntityId selectedId : _selectedEntityIds) {
                if (ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                    selectedEntity->_initTransform = selectedEntity->_transform;
                }
            }
            sGrabMode = false;
        }

        if (inputManager.IsKeyReleasedThisFrame(InputManager::Key::Escape)) {
            // revert transforms
            for (ne::EntityId selectedId : _selectedEntityIds) {
                if (ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                    selectedEntity->_transform = selectedEntity->_initTransform;
                }
            }
            sGrabMode = false;
        }
    } else {
        // select mode
        static float sClickHoldTimer = -1.f;
        static double sClickX = -1.f;
        static double sClickY = -1.f;
        static bool dragMode = false;
        if (inputManager.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
            dragMode = false;
            sClickHoldTimer = 0.f;
            inputManager.GetMousePos(sClickX, sClickY);
        }

        bool wasInDragMode = dragMode;
        if (dragMode) {
            Vec3 clickWorldPos;
            if (ProjectScreenPointToXZPlane((int)sClickX, (int)sClickY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &clickWorldPos)) {
                double mouseX, mouseY;
                inputManager.GetMousePos(mouseX, mouseY);
                Vec3 mouseWorldPos;
                if (ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &mouseWorldPos)) {
                    Vec3 center = (clickWorldPos + mouseWorldPos) * 0.5f;
                    Vec3 scale = center - clickWorldPos;
                    scale._x = 2 * std::abs(scale._x);
                    scale._y = 1.f;
                    scale._z = 2 * std::abs(scale._z);
                    Transform t;
                    t.SetScale(scale);
                    t.SetPos(center);
                    _g->_scene->DrawBoundingBox(t.Mat4Scale(), ToColor4(ColorPreset::White));

                    if (!inputManager.IsKeyPressed(InputManager::MouseButton::Left)) {
                        dragMode = false;
                        // // LOL
                        t.SetScale(Vec3(t.Scale()._x, 1000.f, t.Scale()._z));
                        bool shiftHeld = inputManager.IsShiftPressed();
                        bool altHeld = inputManager.IsAltPressed();
                        if (!shiftHeld) {
                            _selectedEntityIds.clear();
                        }
                        std::vector<ne::BaseEntity*> selected;
                        for (auto iter = _g->_neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
                            if (ne::BaseEntity* e = iter.GetEntity()) {
                                if (!altHeld && !e->_pickable) {
                                    continue;
                                }
                                selected.push_back(e);
                            }
                        }
                        for (auto iter = _g->_neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
                            if (ne::BaseEntity* e = iter.GetEntity()) {
                                if (!altHeld && !e->_pickable) {
                                    continue;
                                }
                                selected.push_back(e);
                            }
                        }
                        for (ne::BaseEntity* e : selected) {
                            bool inBox = geometry::IsPointInBoundingBox2d(e->_transform.GetPos(), t);
                            if (inBox) {
                                if (shiftHeld) {
                                    bool alreadyExists = !_selectedEntityIds.emplace(e->_id).second;
                                    if (alreadyExists) {
                                        _selectedEntityIds.erase(e->_id);
                                    }
                                } else {
                                    _selectedEntityIds.emplace(e->_id);
                                }
                            }
                        }                        
                    }
                }
            }
        } else if (inputManager.IsKeyPressed(InputManager::MouseButton::Left)) {
            float constexpr kTimeToDrag = 0.25f;
            if (sClickHoldTimer >= kTimeToDrag) {
                dragMode = true;
            }
            sClickHoldTimer += deltaTime;
        }

        if (!wasInDragMode && inputManager.IsKeyReleasedThisFrame(InputManager::MouseButton::Left)) {
            double mouseX, mouseY;
            // inputManager.GetMousePos(mouseX, mouseY);
            mouseX = sClickX;
            mouseY = sClickY;
            static std::vector<std::pair<ne::Entity*, float>> entityDistPairs;
            entityDistPairs.clear();
            bool const ignorePickable = inputManager.IsAltPressed();
            PickEntities(*_g->_neEntityManager, mouseX, mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, ignorePickable, entityDistPairs);
            if (entityDistPairs.empty()) {
                if (!inputManager.IsShiftPressed()) {
                    _selectedEntityIds.clear();
                }
            } else {
                ne::Entity* pPicked = entityDistPairs.front().first;;
                
                // See if the currently selected entity is somewhere in the pick
                // list
                int ixOfSelectedInPicked = -1;
                if (!_selectedEntityIds.empty()) {
                    for (int ii = 0; ii < entityDistPairs.size(); ++ii) {
                        ne::Entity* pPicked = entityDistPairs[ii].first;
                        if (_selectedEntityIds.find(pPicked->_id) != _selectedEntityIds.end()) {
                            ixOfSelectedInPicked = ii;
                            break;
                        }
                    }
                }

                if (ixOfSelectedInPicked >= 0) {
                    // If no modifiers, just pick this one and select only this
                    // one. If shift is held, remove only this one from the
                    // selection. If alt is held, cycle to the next one.
                    if (inputManager.IsAltPressed()) {
                        int nextIx = (ixOfSelectedInPicked + 1) % entityDistPairs.size();
                        pPicked = entityDistPairs[nextIx].first;
                    } else {
                        pPicked = entityDistPairs[ixOfSelectedInPicked].first;
                    }
                }

                assert(pPicked);
                if (inputManager.IsShiftPressed()) {
                    auto selectedIter = _selectedEntityIds.find(pPicked->_id);
                    if (selectedIter == _selectedEntityIds.end()) {
                        _selectedEntityIds.emplace(pPicked->_id);
                        // pPicked->OnEditPick(*_g);
                        PickEntity(pPicked);
                    } else {
                        _selectedEntityIds.erase(selectedIter);
                    }
                } else {
                    _selectedEntityIds.clear();
                    _selectedEntityIds.emplace(pPicked->_id);
                    // pPicked->OnEditPick(*_g);
                    PickEntity(pPicked);
                }
            }
        }

        if (sInputMode == InputMode::Default && inputManager.IsKeyReleasedThisFrame(InputManager::Key::G) && !_selectedEntityIds.empty()) {
            sGrabMode = true;
            double mouseX, mouseY;
            inputManager.GetMousePos(mouseX, mouseY);
            ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &sGrabPos);
        }
    }
    
}

namespace {
int GetNoteIndexFromKey(InputManager::Key key) {
    switch (key) {
        case InputManager::Key::A: { return 0; }
        case InputManager::Key::W: { return 1; }
        case InputManager::Key::S: { return 2; }
        case InputManager::Key::E: { return 3; }
        case InputManager::Key::D: { return 4; }
        case InputManager::Key::F: { return 5; }
        case InputManager::Key::T: { return 6; }
        case InputManager::Key::G: { return 7; }
        case InputManager::Key::Y: { return 8; }
        case InputManager::Key::H: { return 9; }
        case InputManager::Key::U: { return 10; }
        case InputManager::Key::J: { return 11; }
        case InputManager::Key::K: { return 12; }
        default: return -1;
    }
    return -1;
}

int GetMidiNoteFromKey(InputManager::Key key, int octave) {
    assert(octave >= 0);
    int offset = GetNoteIndexFromKey(key);
    if (offset < 0) {
        return -1;
    }
    int midiNote = 12 * (octave + 1) + offset;
    return midiNote;
}

int gPianoOctave = 3;

InputManager::Key sPianoKeys[] = {
    InputManager::Key::A,
    InputManager::Key::W,
    InputManager::Key::S,
    InputManager::Key::E,
    InputManager::Key::D,
    InputManager::Key::F,
    InputManager::Key::T,
    InputManager::Key::G,
    InputManager::Key::Y,
    InputManager::Key::H,
    InputManager::Key::U,
    InputManager::Key::J,
    InputManager::Key::K
};
}

void Editor::HandlePianoInput(SynthGuiState& synthGuiState) {
    InputManager const& inputManager = *_g->_inputManager;

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Left)) {
        gPianoOctave = std::max(0, gPianoOctave - 1);
    } else if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Right)) {
        gPianoOctave = std::min(7, gPianoOctave + 1);
    }
    
    int const numPianoKeys = M_ARRAY_LEN(sPianoKeys);
    for (int i = 0; i < numPianoKeys; ++i) {
        InputManager::Key key = sPianoKeys[i];
        bool pressed = inputManager.IsKeyPressedThisFrame(key);
        bool released = inputManager.IsKeyReleasedThisFrame(key);
        if (!pressed && !released) {
            continue;
        }
        int midiNote = GetMidiNoteFromKey(key, gPianoOctave);
        if (midiNote < 0) {
            continue;
        }
        audio::Event e;
        e.channel = synthGuiState._currentSynthIx;
        e.midiNote = midiNote;
        if (pressed) {
            e.type = audio::EventType::NoteOn;
        } else if (released) {
            e.type = audio::EventType::NoteOff;
        }
        _g->_audioContext->AddEvent(e);
    }
}

void Editor::Update(float dt, SynthGuiState& synthGuiState) {
    if (!_g->_editMode) {
        return;
    }

    //
    // Handle release actions
    //
    float constexpr kActionReleaseTime = 0.5f;
    if (_heldActionsTimer >= 0.f) {
        if (_heldActionsTimer >= kActionReleaseTime) {
            for (auto& action : _heldActions) {
                action->ExecuteRelease(*_g);
            }
            _heldActions.clear();
            _heldActionsTimer = -1.f;
        } else {
            _heldActionsTimer += dt;
        }
    }

    if (_requestedNewSelection.IsValid()) {
        _selectedEntityIds.clear();
        _selectedEntityIds.insert(_requestedNewSelection);
        _requestedNewSelection = ne::EntityId();
    }
    
    InputManager const& inputManager = *_g->_inputManager;

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::P)) {
        sInputMode = (InputMode) ((((int) sInputMode) + 1) % ((int) InputMode::Count));
    }

    if (sInputMode == InputMode::Default) {
        // Control groups
        if (inputManager.IsCtrlPressed()) {
            int ctrlGroupIx = inputManager.GetNumPressedThisFrame();
            if (ctrlGroupIx >= 0) {
                std::vector<ne::EntityId>& ctrlGroup = gControlGroups[ctrlGroupIx];
                ctrlGroup.clear();
                ctrlGroup.insert(ctrlGroup.begin(), _selectedEntityIds.begin(), _selectedEntityIds.end());
            }
        } else {
            int ctrlGroupIx = inputManager.GetNumPressedThisFrame();
            if (ctrlGroupIx >= 0) {
                std::vector<ne::EntityId>& ctrlGroup = gControlGroups[ctrlGroupIx];
                _selectedEntityIds.clear();
                _selectedEntityIds.insert(ctrlGroup.begin(), ctrlGroup.end());
            }            
        }

        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Y)) {
            sSynthWindowVisible = !sSynthWindowVisible;
        }
        
    
        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::I)) {
            _entityWindowVisible = !_entityWindowVisible;
        }

        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::M)) {
            sMultiEnemyEditorVisible = !sMultiEnemyEditorVisible;
        }

        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::C)) {
            _showControllerInputs = !_showControllerInputs;
        }

        if (inputManager.IsShiftPressed() && inputManager.IsKeyPressedThisFrame(InputManager::Key::I)) {
            sDemoWindowVisible = !sDemoWindowVisible;
        }

        // Move camera around with RMB + WSAD.
        if (_g->_inputManager->IsKeyPressed(InputManager::MouseButton::Right)) {
            Vec3 inputVec;
            if (_g->_inputManager->IsKeyPressed(InputManager::Key::W)) {
                inputVec._z -= 1.f;
            }
            if (_g->_inputManager->IsKeyPressed(InputManager::Key::S)) {
                inputVec._z += 1.f;
            }
            if (_g->_inputManager->IsKeyPressed(InputManager::Key::A)) {
                inputVec._x -= 1.f;
            }
            if (_g->_inputManager->IsKeyPressed(InputManager::Key::D)) {
                inputVec._x += 1.f;
            }
            Vec3 camVel = inputVec * 5.f;
            if (_g->_inputManager->IsShiftPressed()) {
                camVel *= 5.f;
            }
            _g->_scene->_camera._transform.Translate(camVel * dt);
        }
    } else if (sInputMode == InputMode::Piano) {
        HandlePianoInput(synthGuiState);
    }
    
    HandleEntitySelectAndMove(dt);    

    // Use scroll input to move debug camera around.
    double scrollX = 0.0, scrollY = 0.0;
    _g->_inputManager->GetMouseScroll(scrollX, scrollY);
    if (scrollX != 0.0 || scrollY != 0.0) {
        Vec3 camMotionVec = gScrollFactorForDebugCameraMove * Vec3(-scrollX, 0.f, -scrollY);
        _g->_scene->_camera._transform.Translate(camMotionVec);
    }

    // Draw axes
    for (ne::EntityId selectedId : _selectedEntityIds) {
        ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(selectedId);
        if (selectedEntity == nullptr) {            
            continue;
        }
        renderer::ModelInstance& axesModel = _g->_scene->DrawMesh(_axesMesh, selectedEntity->_transform.Mat4NoScale(), Vec4());
        axesModel._topLayer = true;
        axesModel._useMeshColor = true;

        Transform bbTrans = selectedEntity->_transform;
        bbTrans.SetScale(bbTrans.Scale() + Vec3(0.1f, 0.1f, 0.1f));
        _g->_scene->DrawBoundingBox(bbTrans.Mat4Scale(), Vec4(1.f, 1.f, 1.f, 1.f));
    }

    if (_entityWindowVisible) {
        DrawWindow();
    }

    if (sMultiEnemyEditorVisible) {
        DrawMultiEnemyWindow();
    }

    if (sSynthWindowVisible) {
        DrawSynthGuiAndUpdatePatch(synthGuiState, *_g->_audioContext);
    }

    if (sDemoWindowVisible) {
        ImGui::ShowDemoWindow(&sDemoWindowVisible);
    }
}

namespace {
    std::pair<std::string_view, int> SplitIntoNameAndNumberSuffix(std::string const& str) {
        int numIx = -1;
        for (int charIx = str.length() - 1; charIx >= 0; --charIx) {
            if (isdigit(str[charIx])) {
                numIx = charIx;
            } else {
                break;
            }
        }
        if (numIx < 0) {
            return std::make_pair(std::string_view(str), -1);
        }
        std::string_view nameStr(str.c_str(), numIx);
        std::string_view numStr(str.c_str() + numIx);
        int suffixNum = -1;
        if (string_util::MaybeStoi(numStr, suffixNum)) {            
            return std::make_pair(nameStr, suffixNum);
        } else {
            printf("WARNING: bad number suffix starting at %d in %s\n", numIx, str.c_str());
            return std::make_pair(std::string_view(str), -1);
        }
    }
}

void Editor::DrawWindow() {
    ImGui::Begin("Editor");

    imgui_util::InputText<256>("Save filename", &_saveFilename);
    if (ImGui::Button("Save")) {
        serial::Ptree pt = serial::Ptree::MakeNew();
        serial::Ptree scriptPt = pt.AddChild("script");
        scriptPt.PutDouble("bpm", _g->_beatClock->GetBpm());
        scriptPt.PutBool("gamma_correction", _g->_scene->IsGammaCorrectionEnabled());
        Save(scriptPt);
        serial::Ptree entitiesPt = scriptPt.AddChild("new_entities");
        for (ne::EntityId id : _entityIds) {
            bool active = false;
            ne::Entity* entity = _g->_neEntityManager->GetActiveOrInactiveEntity(id, &active);
            if (entity == nullptr) {
                continue;
            }
            assert(id._type == entity->_id._type);
            serial::Ptree entityPt = entitiesPt.AddChild(ne::gkEntityTypeNames[(int)id._type]);
            entity->Save(entityPt);
        }
        if (pt.WriteToFile(_saveFilename.c_str())) {
            printf("Saved script to \"%s\".\n", _saveFilename.c_str());
        } else {
            printf("FAILED to save script to \"%s\".\n", _saveFilename.c_str());
        }
        pt.DeleteData();
    }

    ImGui::Checkbox("Filter by flow section ID", &_enableFlowSectionFilter);
    if (_enableFlowSectionFilter) {
        ImGui::InputInt("Flow section ID", &_flowSectionFilterId);;
    }

    static int64_t sEditorIdFilter = -1;
    ImGui::InputScalar("Editor ID Filter", ImGuiDataType_S64, &sEditorIdFilter);

    static std::string sNameFilter;
    imgui_util::InputText<128>("Name Filter", &sNameFilter);

    static int sTagFilter = -1;
    ImGui::InputInt("Tag Filter", &sTagFilter);

    // Entities  
    if (ImGui::BeginListBox("Entities")) {
        for (auto entityIdIter = _entityIds.begin(); entityIdIter != _entityIds.end(); /*no inc*/) {
            ne::EntityId& entityId = *entityIdIter;
            ne::Entity* entity = _g->_neEntityManager->GetActiveOrInactiveEntity(entityId);

            // Clean up any missing entities
            if (entity == nullptr) {
                entityIdIter = _entityIds.erase(entityIdIter);
                continue;
            }

            if (_enableFlowSectionFilter) {
                if (entity->_flowSectionId != _flowSectionFilterId) {
                    ++entityIdIter;
                    continue;
                }
            }

            if (sEditorIdFilter >= 0) {
                if (entity->_editorId._id != sEditorIdFilter) {
                    ++entityIdIter;
                    continue;
                }
            }

            if (!sNameFilter.empty()) {
                if (!string_util::Contains(entity->_name, sNameFilter)) {
                    ++entityIdIter;
                    continue;
                }
            }

            if (sTagFilter >= 0) {
                if (entity->_tag != sTagFilter) {
                    ++entityIdIter;
                    continue;
                }
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
        entity->_editorId._id = _nextEditorId++;
        _entityIds.push_back(entity->_id);        
        // place entity at center of view
        Vec3 newPos;
        bool projectSuccess = ProjectScreenPointToXZPlane(_g->_windowWidth / 2, _g->_windowHeight / 2, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &newPos);
        if (!projectSuccess) {
            printf("Editor.AddEntity: Failed to project center to XZ plane!\n");
        }
        entity->_initTransform.SetTranslation(newPos);
        entity->Init(*_g);

        _selectedEntityIds.clear();
        _selectedEntityIds.insert(entity->_id);
    }
    
    // Duplicate entities
    if (ImGui::Button("Duplicate Entity")) {
        std::vector<ne::EntityId> newIds;
        newIds.reserve(_selectedEntityIds.size());
        for (ne::EntityId selectedId : _selectedEntityIds) {
            bool selectedEntityActive = true;
            if (!_g->_neEntityManager->GetActiveOrInactiveEntity(selectedId, &selectedEntityActive)) {
                // NOTE: In the case that GetEntity() returns non-null, we DO
                // NOT STORE it because we're about to add an entity to the
                // manager. pointers are NOT STABLE after addition.
                continue;
            }

            ne::Entity* newEntity = _g->_neEntityManager->AddEntity(selectedId._type, selectedEntityActive);
            assert(newEntity != nullptr);

            // NOTE! MUST do this AFTER doing AddEntity() above or else selectedEntity pointer can get invalidated.
            ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(selectedId);
            assert(selectedEntity != nullptr);

            {
                serial::Ptree pt = serial::Ptree::MakeNew();
                selectedEntity->Save(pt);
                newEntity->Load(pt);
                pt.DeleteData();
            }

            newEntity->_editorId._id = _nextEditorId++;

            // Pick new name for duplicate
            {
                auto [nameStr, suffixNum] = SplitIntoNameAndNumberSuffix(newEntity->_name);
                for (ne::EntityManager::AllIterator eIter = _g->_neEntityManager->GetAllIterator(); !eIter.Finished(); eIter.Next()) {
                    if (ne::Entity* entity = eIter.GetEntity()) {
                        auto [otherNameStr, otherSuffixNum] = SplitIntoNameAndNumberSuffix(entity->_name);
                        if (nameStr == otherNameStr) {
                            suffixNum = std::max(suffixNum, otherSuffixNum);
                        }
                    }
                }
                for (ne::EntityManager::AllIterator eIter = _g->_neEntityManager->GetAllInactiveIterator(); !eIter.Finished(); eIter.Next()) {
                    if (ne::Entity* entity = eIter.GetEntity()) {
                        auto [otherNameStr, otherSuffixNum] = SplitIntoNameAndNumberSuffix(entity->_name);
                        if (nameStr == otherNameStr) {
                            suffixNum = std::max(suffixNum, otherSuffixNum);
                        }
                    }
                }
                ++suffixNum;
                std::stringstream ss;
                ss << nameStr << suffixNum;
                newEntity->_name = ss.str();
                // nameStr is invalid here!!!
            }

            _entityIds.push_back(newEntity->_id);
            // TODO: are we cool with Init-ing an inactive dude?
            newEntity->Init(*_g);

            newIds.push_back(newEntity->_id);
        }
        _selectedEntityIds.clear();
        for (ne::EntityId const& id : newIds) {
            _selectedEntityIds.insert(id);
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
        ne::EntityId entityId = *_selectedEntityIds.begin();
        if (ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(entityId)) {
            if (selectedEntity->ImGui(*_g) == ne::Entity::ImGuiResult::NeedsInit) {
                selectedEntity->Destroy(*_g);
                selectedEntity->Init(*_g);
            }
        }
    } else {
        if (_selectedEntityIds.empty()) {
            ImGui::Text("(No selected entities)");
        } else {
            auto GetEntity = [this](ne::EntityId eId) {
                return this->_g->_neEntityManager->GetActiveOrInactiveEntity(eId);    
            };

            // TODO: move this into new_entity.cpp
            ne::EntityId exampleId = *_selectedEntityIds.begin();
            ne::Entity* example = GetEntity(exampleId);
            bool needsInit = false;
            if (ImGui::Button("Force Init")) {
                needsInit = true;
            }
            if (ImGui::InputInt("Section Id", &example->_flowSectionId)) {
                for (ne::EntityId eId : _selectedEntityIds) {
                    ne::Entity* e = GetEntity(eId);
                    e->_flowSectionId = example->_flowSectionId;
                }
            }
            if (ImGui::Checkbox("Init active", &example->_initActive)) {
                for (ne::EntityId eId : _selectedEntityIds) {
                    if (example->_initActive) {
                        _g->_neEntityManager->TagForActivate(eId, /*initOnActivate=*/true);
                    } else {
                        _g->_neEntityManager->TagForDeactivate(eId);
                    }
                } 
            }
            if (imgui_util::ColorEdit4("Model color", &example->_modelColor)) {
                needsInit = true;
                for (ne::EntityId eId : _selectedEntityIds) {
                    ne::Entity* e = GetEntity(eId);
                    e->_modelColor = example->_modelColor;
                }
            }

            bool allSameType = true;
            _sameEntities.clear(); 
            _sameEntities.reserve(_selectedEntityIds.size());
            for (ne::EntityId eId : _selectedEntityIds) {
                if (eId._type != exampleId._type) {
                    allSameType = false;
                    break;
                }
                ne::Entity* e = GetEntity(eId);
                _sameEntities.push_back(e);
            }
            if (allSameType) {
                if (example->MultiImGui(*_g, _sameEntities.data(), _sameEntities.size()) == ne::Entity::ImGuiResult::NeedsInit) {
                    needsInit = true;
                }
            }

            if (needsInit) {
                for (ne::EntityId eId : _selectedEntityIds) {
                    ne::Entity* e = GetEntity(eId);
                    e->Init(*_g);
                }
            }
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
            if (ne::Entity* entity = _g->_neEntityManager->GetActiveOrInactiveEntity(eId)) {
                sSelectedEnemies.push_back(static_cast<TypingEnemyEntity*>(entity));
            }
        }
    }
    TypingEnemyEntity::MultiSelectImGui(*_g, sSelectedEnemies);
    
    ImGui::End();
}

bool Editor::IsEntitySelected(ne::EntityId entityId) {
    auto iter = _selectedEntityIds.find(entityId);
    bool selected = iter != _selectedEntityIds.end();
    return selected;
}

ne::Entity* Editor::ImGuiEntitySelector(char const* buttonLabel, char const* popupLabel) {
    bool selected = false;
    ne::Entity* selectedEntity = nullptr;
    if (ImGui::Button(buttonLabel)) {
        ImGui::OpenPopup(popupLabel);
    }
    if (ImGui::BeginPopup(popupLabel)) {
        static std::string sNameFilter;
        imgui_util::InputText<128>("Search", &sNameFilter);
        if (ImGui::BeginListBox("Entities")) {
            for (auto entityIdIter = _entityIds.begin(); entityIdIter != _entityIds.end(); ++entityIdIter) {
                ne::EntityId entityId = *entityIdIter;
                ne::Entity* entity = _g->_neEntityManager->GetActiveOrInactiveEntity(entityId);
                if (entity == nullptr) {
                    continue;
                }
                if (!sNameFilter.empty()) {
                    if (!string_util::Contains(entity->_name, sNameFilter)) {
                        continue;
                    }
                }
                ImGui::PushID(entityId._id);
                bool clicked = ImGui::Selectable(entity->_name.c_str(), false);
                if (clicked) {
                    selectedEntity = entity;
                    selected = true;
                }
                ImGui::PopID();
            }
            ImGui::EndListBox();
        }
        if (selected) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return selectedEntity;
}

void Editor::SelectEntity(ne::Entity& e) {
    _requestedNewSelection = e._id;
}
