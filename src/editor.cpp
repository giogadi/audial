#include "editor.h"

#include <map>
#include <cctype>
#include <sstream>
#include <unordered_set>
#include <ctime>
#include <algorithm>

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
#include "entities/step_sequencer.h"
#include <omni_sequencer.h>
#include <string_ci.h>

extern EditorIdMap *gEditorIdMap;

static float gScrollFactorForDebugCameraMove = 1.f;

static std::vector<ne::EntityId> gControlGroups[10];

static bool sSeqWindowVisible = false;

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
            if (e->_editorId.IsValid()) { // Filter out "runtime" generated objects
                // no init necessary
                _entityIds.push_back(e->_id);
                if (e->_editorId._id >= 0) {
                    _nextEditorId = std::max(_nextEditorId, e->_editorId._id + 1);
                    auto result = editorIds.insert(e->_editorId._id);
                    assert(result.second);
                }
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

enum class InputMode {
    Default,
    DragToSelect,
    Grab,
    Scale,
    PolyDraw,
    Piano,
    EnemyTyping,
    Sequencer,
    Count
};

char const *InputModeName(InputMode mode) {
    switch (mode) {
        case InputMode::Default: return "Default";
        case InputMode::DragToSelect: return "Drag to select";
        case InputMode::Grab: return "Grab";
        case InputMode::Scale: return "Scale";
        case InputMode::PolyDraw: return "Draw poly to select";
        case InputMode::Piano: return "Piano";
        case InputMode::EnemyTyping: return "Enemy typing";
        case InputMode::Sequencer: return "Sequencer";
        case InputMode::Count: assert(false); return "";
    }
    return nullptr;
}

void PickEntity(Editor &editor, ne::BaseEntity* entity) {
    if (entity->Type() == ne::EntityType::TypingEnemy) {
        TypingEnemyEntity* enemy = static_cast<TypingEnemyEntity*>(entity);
        HitResult result = enemy->OnHit(*editor._g, 0);
        if (!editor._heldActions.empty()) {
            for (auto const& a : editor._heldActions) {
                a->ExecuteRelease(*editor._g);
            }
        }
        editor._heldActions = std::move(result._heldActions);
        editor._heldActionsTimer = 0.f;
    } else {
        entity->OnEditPick(*editor._g);
    }
}

InputMode sInputMode = InputMode::Default;
bool sSynthWindowVisible = false;
bool sDemoWindowVisible = false;
bool sMultiEnemyEditorVisible = false;

} // end namespace

void Editor::DrawSeqWindow() {
    ImGui::Begin("Step sequencers");
    for (int entityIx = 0; entityIx < _entityIds.size(); ++entityIx) {
        ne::EntityId const eId = _entityIds[entityIx];
        if (eId._type != ne::EntityType::StepSequencer) {
            continue;
        }
        StepSequencerEntity *e = _g->_neEntityManager->GetEntityAs<StepSequencerEntity>(eId, true, true);
        if (!e) {
            continue;
        }
        ImGui::PushID(entityIx);
        ImGui::Text("%s",e->_name.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Go to")) {
            SelectEntity(*e);
        }
        ImGui::SameLine();
        bool activeChanged = ImGui::Checkbox("Active", &e->_initActive);
        if (activeChanged) {
            if (e->_initActive) {
                _g->_neEntityManager->TagForActivate(eId, /*initOnActivate=*/true);
            } else {
                _g->_neEntityManager->TagForDeactivate(eId);
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Editor mute", &e->_editorMute);
        ImGui::PopID();
    }
    ImGui::End();
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

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::P)) {
        sInputMode = InputMode::Default;
        return;
    }

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

int Editor::GetPianoNotes(int *midiNotes, int maxNotes) {
    int numPressedNotes = 0;
    int const numPianoKeys = M_ARRAY_LEN(sPianoKeys);
    for (int ii = 0; ii < numPianoKeys && numPressedNotes < maxNotes; ++ii) {
        InputManager::Key key = sPianoKeys[ii];
        if (_g->_inputManager->IsKeyPressedThisFrame(key)) {
            midiNotes[numPressedNotes++] = GetMidiNoteFromKey(key, gPianoOctave);
        }        
    }
    return numPressedNotes;
}

int Editor::IncreasePianoOctave() {
    gPianoOctave = std::min(7, gPianoOctave + 1);
    return gPianoOctave;
}

int Editor::DecreasePianoOctave() {
    gPianoOctave = std::max(0, gPianoOctave - 1);
    return gPianoOctave;
}

namespace {
void UpdateDefault(GameManager &g, float const dt, Editor &editor) {
    InputManager &inputManager = *g._inputManager;

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::P)) {
        sInputMode = InputMode::Piano;
        return;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::G) && !editor._selectedEntityIds.empty()) {
        sInputMode = InputMode::Grab;
        return;
    }

    if (inputManager.IsAltPressed() && inputManager.IsKeyPressedThisFrame(InputManager::Key::S) && !inputManager.IsKeyPressed(InputManager::MouseButton::Right)) {
        sInputMode = InputMode::PolyDraw;
        return;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::S) && !editor._selectedEntityIds.empty() && !inputManager.IsAltPressed() && !inputManager.IsKeyPressed(InputManager::MouseButton::Right)) {
        sInputMode = InputMode::Scale;
        return;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::T)) {
        sInputMode = InputMode::EnemyTyping;
        return;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Q) && !inputManager.IsShiftPressed()) {
        sInputMode = InputMode::Sequencer;
        return;
    }

    float constexpr kMinTimeForDrag = 15.f * (1.f / 60.f);
    if (inputManager.IsKeyPressed(InputManager::MouseButton::Left) &&
        inputManager.GetKeyDownTime(InputManager::MouseButton::Left) >= kMinTimeForDrag) {
        sInputMode = InputMode::DragToSelect;
        return;
    }

    // Control groups
    if (inputManager.IsCtrlPressed()) {
        int ctrlGroupIx = inputManager.GetNumPressedThisFrame();
        if (ctrlGroupIx >= 0) {
            std::vector<ne::EntityId>& ctrlGroup = gControlGroups[ctrlGroupIx];
            ctrlGroup.clear();
            ctrlGroup.insert(ctrlGroup.begin(), editor._selectedEntityIds.begin(), editor._selectedEntityIds.end());
        }
    } else {
        int ctrlGroupIx = inputManager.GetNumPressedThisFrame();
        if (ctrlGroupIx >= 0) {
            std::vector<ne::EntityId>& ctrlGroup = gControlGroups[ctrlGroupIx];
            editor._selectedEntityIds.clear();
            editor._selectedEntityIds.insert(ctrlGroup.begin(), ctrlGroup.end());
        }
    }

    if (inputManager.IsShiftPressed() && inputManager.IsKeyPressedThisFrame(InputManager::Key::Q)) {
        sSeqWindowVisible = !sSeqWindowVisible;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Y)) {
        sSynthWindowVisible = !sSynthWindowVisible;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::I)) {
        editor._entityWindowVisible = !editor._entityWindowVisible;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::M)) {
        sMultiEnemyEditorVisible = !sMultiEnemyEditorVisible;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::C)) {
        editor._showControllerInputs = !editor._showControllerInputs;
    }

    if (inputManager.IsShiftPressed() && inputManager.IsKeyPressedThisFrame(InputManager::Key::I)) {
        sDemoWindowVisible = !sDemoWindowVisible;
    }

    // Move camera around with RMB + WSAD.
    if (inputManager.IsKeyPressed(InputManager::MouseButton::Right)) {
        Vec3 inputVec;
        if (inputManager.IsKeyPressed(InputManager::Key::W)) {
            inputVec._z -= 1.f;
        }
        if (inputManager.IsKeyPressed(InputManager::Key::S)) {
            inputVec._z += 1.f;
        }
        if (inputManager.IsKeyPressed(InputManager::Key::A)) {
            inputVec._x -= 1.f;
        }
        if (inputManager.IsKeyPressed(InputManager::Key::D)) {
            inputVec._x += 1.f;
        }
        Vec3 camVel = inputVec * 5.f;
        if (inputManager.IsShiftPressed()) {
            camVel *= 5.f;
        }
        g._scene->_camera._transform.Translate(camVel * dt);
    }

    // Use scroll input to move debug camera around.
    double scrollX = 0.0, scrollY = 0.0;
    inputManager.GetMouseScroll(scrollX, scrollY);
    if (scrollX != 0.0 || scrollY != 0.0) {
        Vec3 camMotionVec = gScrollFactorForDebugCameraMove * Vec3(-scrollX, 0.f, -scrollY);
        g._scene->_camera._transform.Translate(camMotionVec);
    }

    // Click to select entities    
    if (inputManager.IsKeyReleasedThisFrame(InputManager::MouseButton::Left) &&
        inputManager.GetKeyDownTime(InputManager::MouseButton::Left) < kMinTimeForDrag) {
        double clickX, clickY;
        inputManager.GetMouseClickPos(clickX, clickY);

        static std::vector<std::pair<ne::Entity*, float>> entityDistPairs;
        entityDistPairs.clear();
        bool const ignorePickable = inputManager.IsAltPressed();
        PickEntities(*g._neEntityManager, clickX, clickY, g._windowWidth, g._windowHeight, g._scene->_camera, ignorePickable, entityDistPairs);
        if (entityDistPairs.empty()) {
            if (!inputManager.IsShiftPressed()) {
                editor._selectedEntityIds.clear();
            }
        } else {
            ne::Entity* pPicked = entityDistPairs.front().first;;

            // See if the currently selected entity is somewhere in the pick
            // list
            int ixOfSelectedInPicked = -1;
            if (!editor._selectedEntityIds.empty()) {
                for (int ii = 0; ii < entityDistPairs.size(); ++ii) {
                    ne::Entity* pPicked = entityDistPairs[ii].first;
                    if (editor._selectedEntityIds.find(pPicked->_id) != editor._selectedEntityIds.end()) {
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
                auto selectedIter = editor._selectedEntityIds.find(pPicked->_id);
                if (selectedIter == editor._selectedEntityIds.end()) {
                    editor._selectedEntityIds.emplace(pPicked->_id);
                    PickEntity(editor, pPicked);
                } else {
                    editor._selectedEntityIds.erase(selectedIter);
                }
            } else {
                editor._selectedEntityIds.clear();
                editor._selectedEntityIds.emplace(pPicked->_id);
                PickEntity(editor, pPicked);
            }
        }
    }
}

void UpdateDragToSelect(GameManager &g, Editor &editor) {
    InputManager &inputManager = *g._inputManager;

    Vec3 clickWorldPos;
    double clickX, clickY;
    inputManager.GetMouseClickPos(clickX, clickY);
    bool success = geometry::ProjectScreenPointToXZPlane((int)clickX, (int)clickY, g._windowWidth, g._windowHeight, g._scene->_camera, &clickWorldPos);
    if (!success) {
        printf("editor.cpp::UpdateDragToSelect: failed to project click pos? %f %f\n", clickX, clickY);
    }

    double mouseX, mouseY;
    inputManager.GetMousePos(mouseX, mouseY);
    Vec3 mouseWorldPos;
    success = geometry::ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, g._windowWidth, g._windowHeight, g._scene->_camera, &mouseWorldPos);
    if (!success) {
        printf("editor.cpp::UpdateDragToSelect: failed to project mouse pos? %f %f\n", mouseX, mouseY);
    }
    
    Vec3 center = (clickWorldPos + mouseWorldPos) * 0.5f;
    Vec3 scale = center - clickWorldPos;
    scale._x = 2 * std::abs(scale._x);
    scale._y = 1.f;
    scale._z = 2 * std::abs(scale._z);
    Transform t;
    t.SetScale(scale);
    t.SetPos(center);
    g._scene->DrawBoundingBox(t.Mat4Scale(), ToColor4(ColorPreset::White));

    if (!inputManager.IsKeyPressed(InputManager::MouseButton::Left)) {
        // // LOL
        t.SetScale(Vec3(t.Scale()._x, 1000.f, t.Scale()._z));
        bool shiftHeld = inputManager.IsShiftPressed();
        bool altHeld = inputManager.IsAltPressed();
        if (!shiftHeld) {
            editor._selectedEntityIds.clear();
        }
        std::vector<ne::BaseEntity*> selected;
        for (auto iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
            if (ne::BaseEntity* e = iter.GetEntity()) {
                if (!altHeld && !e->_pickable) {
                    continue;
                }
                selected.push_back(e);
            }
        }
        for (auto iter = g._neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
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
                    bool alreadyExists = !editor._selectedEntityIds.emplace(e->_id).second;
                    if (alreadyExists) {
                        editor._selectedEntityIds.erase(e->_id);
                    }
                } else {
                    editor._selectedEntityIds.emplace(e->_id);
                }
            }
        }

        sInputMode = InputMode::Default;
    }
}

void UpdateGrab(GameManager &g, Editor &editor, bool newState) {
    InputManager &inputManager = *g._inputManager;
    double mouseX, mouseY;
    inputManager.GetMousePos(mouseX, mouseY);
    Vec3 mousePosOnXZPlane;
    geometry::ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, g._windowWidth, g._windowHeight, g._scene->_camera, &mousePosOnXZPlane);
    static Vec3 sGrabPos;
    if (newState) {
        // TODO: this will currently happen on the next frame, unless we allow same-frame input mode changes.
        sGrabPos = mousePosOnXZPlane;
    }
    Vec3 grabOffset = mousePosOnXZPlane - sGrabPos;
    for (ne::EntityId selectedId : editor._selectedEntityIds) {
        if (ne::Entity* selectedEntity = g._neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
            Vec3 newPos = selectedEntity->_initTransform.Pos() + grabOffset;
            selectedEntity->_transform.SetPos(newPos);
        }
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Enter) ||
        inputManager.IsKeyPressedThisFrame(InputManager::Key::G) ||
        inputManager.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        // commit transforms
        for (ne::EntityId selectedId : editor._selectedEntityIds) {
            if (ne::Entity* selectedEntity = g._neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                selectedEntity->_initTransform = selectedEntity->_transform;
            }
        }
        sInputMode = InputMode::Default;
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Escape)) {
        // revert transforms
        for (ne::EntityId selectedId : editor._selectedEntityIds) {
            if (ne::Entity* selectedEntity = g._neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                selectedEntity->_transform = selectedEntity->_initTransform;
            }
        }
        sInputMode = InputMode::Default;
    }
}

void UpdateScale(GameManager &g, Editor &editor, bool newState) {
    InputManager &inputManager = *g._inputManager;

    double mouseX, mouseY;
    inputManager.GetMousePos(mouseX, mouseY);
    Vec3 mousePosOnXZPlane;
    geometry::ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, g._windowWidth, g._windowHeight, g._scene->_camera, &mousePosOnXZPlane);

    enum class ScaleAxis {
        XZ, X, Z
    };
    static ScaleAxis sScaleAxis = ScaleAxis::XZ;
    static bool sPositive = true;
    static Vec3 sGrabPos;
    if (newState) {
        sScaleAxis = ScaleAxis::XZ;
        sPositive = true;
        sGrabPos = mousePosOnXZPlane;
    }

    if (sScaleAxis == ScaleAxis::XZ) {
        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::X)) {
            sScaleAxis = ScaleAxis::X;
            sPositive = !inputManager.IsShiftPressed();
        } else if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Z)) {
            sScaleAxis = ScaleAxis::Z;
            sPositive = !inputManager.IsShiftPressed();
        }
    }

    Vec3 grabOffset = mousePosOnXZPlane - sGrabPos;
    switch (sScaleAxis) {
        case ScaleAxis::XZ: break;
        case ScaleAxis::X: {
            grabOffset._y = grabOffset._z = 0.f;
            break;
        }
        case ScaleAxis::Z: {
            grabOffset._x = grabOffset._y = 0.f;
            break;
        }
    }
    for (ne::EntityId selectedId : editor._selectedEntityIds) {
        float constexpr kMinScale = 0.001f;
        if (ne::Entity* selectedEntity = g._neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
            switch (sScaleAxis) {
                case ScaleAxis::XZ: {
                    Vec3 newScale = selectedEntity->_initTransform.Scale() + 2.f * grabOffset;
                    for (int ii = 0; ii < 3; ++ii) {
                        newScale(ii) = std::max(kMinScale, newScale(ii));
                    }
                    selectedEntity->_transform.SetScale(newScale);
                    break;
                }
                case ScaleAxis::X: {
                    float positive = (sPositive ? 1.f : -1.f);
                    float newEdgePos = selectedEntity->_initTransform.Pos()._x + positive * 0.5f * selectedEntity->_initTransform.Scale()._x + grabOffset._x;
                    float newScaleX = 0.5f * selectedEntity->_initTransform.Scale()._x + positive * (newEdgePos - selectedEntity->_initTransform.Pos()._x);
                    newScaleX = std::max(kMinScale, newScaleX);
                    float newPosX = newEdgePos + (-positive * 0.5f * newScaleX);
                    Vec3 newPos = selectedEntity->_transform.Pos();
                    newPos._x = newPosX;
                    selectedEntity->_transform.SetPos(newPos);
                    Vec3 newScale = selectedEntity->_transform.Scale();
                    newScale._x = newScaleX;
                    selectedEntity->_transform.SetScale(newScale);
                    break;
                }
                case ScaleAxis::Z: {
                    float positive = (sPositive ? 1.f : -1.f);
                    float newEdgePos = selectedEntity->_initTransform.Pos()._z + positive * 0.5f * selectedEntity->_initTransform.Scale()._z + grabOffset._z;
                    float newScaleZ = 0.5f * selectedEntity->_initTransform.Scale()._z + positive * (newEdgePos - selectedEntity->_initTransform.Pos()._z);
                    newScaleZ = std::max(kMinScale, newScaleZ);
                    float newPosZ = newEdgePos + (-positive * 0.5f * newScaleZ);
                    Vec3 newPos = selectedEntity->_transform.Pos();
                    newPos._z = newPosZ;
                    selectedEntity->_transform.SetPos(newPos);
                    Vec3 newScale = selectedEntity->_transform.Scale();
                    newScale._z = newScaleZ;
                    selectedEntity->_transform.SetScale(newScale);
                    break;
                }
            }
        }
    }

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Enter) ||
        inputManager.IsKeyPressedThisFrame(InputManager::Key::S) ||
        inputManager.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        // commit transforms
        for (ne::EntityId selectedId : editor._selectedEntityIds) {
            if (ne::Entity* selectedEntity = g._neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                selectedEntity->_initTransform = selectedEntity->_transform;
            }
        }
        sInputMode = InputMode::Default;
    }

    if (inputManager.IsKeyReleasedThisFrame(InputManager::Key::Escape)) {
        // revert transforms
        for (ne::EntityId selectedId : editor._selectedEntityIds) {
            if (ne::Entity* selectedEntity = g._neEntityManager->GetActiveOrInactiveEntity(selectedId)) {
                selectedEntity->_transform = selectedEntity->_initTransform;
            }
        }
        sInputMode = InputMode::Default;
    }
}

void UpdatePolyDraw(GameManager &g, Editor &editor, bool newState) {
    InputManager &inputManager = *g._inputManager;

    int constexpr kMaxPoints = 64;
    static Vec3 sSelectPolyPoints[kMaxPoints];
    static int sSelectPolyPointsCount = 0;
    if (newState) {
        sSelectPolyPointsCount = 0;
    }
    if (inputManager.IsKeyPressedThisFrame(InputManager::MouseButton::Left)) {
        double mouseX, mouseY;
        inputManager.GetMousePos(mouseX, mouseY);
        Vec3 p;
        geometry::ProjectScreenPointToXZPlane((int)mouseX, (int)mouseY, g._windowWidth, g._windowHeight, g._scene->_camera, &p);
        if (sSelectPolyPointsCount < kMaxPoints) {
            sSelectPolyPoints[sSelectPolyPointsCount++] = p;
        }
    }
    for (int ii = 0; ii < sSelectPolyPointsCount - 1; ++ii) {
        g._scene->DrawLine(sSelectPolyPoints[ii], sSelectPolyPoints[ii+1], Vec4(1.f, 1.f, 1.f, 1.f));
    }

    editor._selectedEntityIds.clear();
    Vec4 localBbox[4];
    localBbox[0].Set(0.5f, 0.f, 0.5f, 1.f);
    localBbox[1].Set(0.5, 0.f, -0.5f, 1.f);
    localBbox[2].Set(-0.5f, 0.f, -0.5f, 1.f);
    localBbox[3].Set(-0.5f, 0.f, 0.5f, 1.f);
    Vec4 worldBbox[4];
    auto maybeSelectEntity = [&](ne::Entity* e) {
        Mat4 localToWorld = e->_transform.Mat4Scale();
        for (int ii = 0; ii < 4; ++ii) {
            worldBbox[ii] = localToWorld * localBbox[ii];
            Vec3 queryP = worldBbox[ii].GetXYZ();
            bool inside = geometry::PointInPolygon2D(queryP, sSelectPolyPoints, sSelectPolyPointsCount);
            if (inside) {
                editor._selectedEntityIds.insert(e->_id);
                break;
            }
        }
        };

    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::S) && inputManager.IsAltPressed() && !inputManager.IsKeyPressed(InputManager::MouseButton::Right)) {
        for (auto iter = g._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
            ne::Entity* entity = iter.GetEntity();
            if (entity == nullptr) {
                continue;
            }
            maybeSelectEntity(entity);
        }
        for (auto iter = g._neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
            ne::Entity* entity = iter.GetEntity();
            if (entity == nullptr) {
                continue;
            }
            maybeSelectEntity(entity);
        }
        
        sInputMode = InputMode::Default;
    }
}

void UpdateEnemyTyping(GameManager &g, Editor &editor) {
    InputManager &inputManager = *g._inputManager;
    if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Escape)) {
        sInputMode = InputMode::Default;
        return;
    }

    Mat4 viewProjTransform = g._scene->GetViewProjTransform();
    auto perEntityFn = [&](ne::EntityManager::Iterator iter) {
        TypingEnemyEntity *e = static_cast<TypingEnemyEntity*>(iter.GetEntity());
        if (e == nullptr) {
            return;
        }
        // Check if it's in view of camera
        float screenX, screenY;
        geometry::ProjectWorldPointToScreenSpace(e->_transform.GetPos(), viewProjTransform, g._windowWidth, g._windowHeight, screenX, screenY);
        if (screenX < 0 || screenX > g._windowWidth || screenY < 0 || screenY > g._windowHeight) {
            return;
        }

        TypingEnemyEntity::NextKeys nextKeys = e->GetNextKeys();
        bool hit = false;
        for (int kIx = 0; kIx < nextKeys.size(); ++kIx) {
            InputManager::Key key = nextKeys[kIx];
            if (key == InputManager::Key::NumKeys) {
                break;
            }
            hit = g._inputManager->IsKeyPressedThisFrame(key);
            if (hit) {
                break;
            }
        }
        if (hit) {
            PickEntity(editor, e);
        }
    };
    for (ne::EntityManager::Iterator iter = g._neEntityManager->GetIterator(ne::EntityType::TypingEnemy); !iter.Finished(); iter.Next()) {
        perEntityFn(iter);
    }
    for (ne::EntityManager::Iterator iter = g._neEntityManager->GetInactiveIterator(ne::EntityType::TypingEnemy); !iter.Finished(); iter.Next()) {
        perEntityFn(iter);
    }
}

void UpdateSequencer(GameManager &g, Editor &editor) {
    InputManager &inputManager = *g._inputManager;
    if (inputManager.IsKeyPressed(InputManager::Key::Escape)) {
        sInputMode = InputMode::Default;
        return;
    }
    g._omniSequencer->Gui(g, editor);
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

    static InputMode sPrevMode = InputMode::Default;
    bool newInputMode = sPrevMode != sInputMode;
    sPrevMode = sInputMode;
    switch (sInputMode) {
        case InputMode::Default:
            UpdateDefault(*_g, dt, *this);
            break;
        case InputMode::DragToSelect:
            UpdateDragToSelect(*_g, *this);
            break;
        case InputMode::Grab:
            UpdateGrab(*_g, *this, newInputMode);
            break;
        case InputMode::Scale:
            UpdateScale(*_g, *this, newInputMode);
            break;
        case InputMode::PolyDraw:
            UpdatePolyDraw(*_g, *this, newInputMode);
            break;
        case InputMode::Piano:
            HandlePianoInput(synthGuiState);
            break;
        case InputMode::EnemyTyping:
            UpdateEnemyTyping(*_g, *this);
            break;
        case InputMode::Sequencer:
            UpdateSequencer(*_g, *this);
            break;
        case InputMode::Count:
            break;
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

    if (sSeqWindowVisible) {
        DrawSeqWindow();
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

namespace {
    float TimeElapsed(clock_t t0, clock_t t1) {
        clock_t ticksElapsed = t1 - t0;
        return ((float)ticksElapsed) / CLOCKS_PER_SEC;
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
        clock_t t0 = clock();
        Save(scriptPt);
        clock_t t1 = clock();
        float treeBuildTime = TimeElapsed(t0, t1);
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
        t0 = clock();
        if (pt.WriteToFile(_saveFilename.c_str())) {
            t1 = clock();
            float fileWriteTime = TimeElapsed(t0, t1);
            printf("Saved script to \"%s\". (build time: %f) (write time: %f)\n", _saveFilename.c_str(), treeBuildTime, fileWriteTime);
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
            int const entityIx = std::distance(_entityIds.begin(), entityIdIter);
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
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ||
                    ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
                    int numErased = _selectedEntityIds.erase(entityId);
                    if (numErased == 0) {
                        _selectedEntityIds.insert(entityId);
                    }
                } else if (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                    ImGui::IsKeyDown(ImGuiKey_RightShift)) {
                    if (!_selectedEntityIds.empty()) {
                        ne::EntityId prevSelected = *_selectedEntityIds.begin();
                        // Find index of this dude
                        int prevEntityIx = -1;
                        for (int ii = 0; ii < _entityIds.size(); ++ii) {
                            if (_entityIds[ii] == prevSelected) {
                                prevEntityIx = ii;
                                break;
                            }
                        }
                        if (prevEntityIx < 0) {
                            printf("Editor: Uh, selected entity ID not in _entityIds?\n");
                            prevEntityIx = 0;
                        }
                        int thisEntityIx = entityIx;
                        if (prevEntityIx > thisEntityIx) {
                            // swap
                            int temp = prevEntityIx;
                            prevEntityIx = thisEntityIx;
                            thisEntityIx = temp;
                        }
                        for (int ii = prevEntityIx; ii <= thisEntityIx; ++ii) {
                            _selectedEntityIds.insert(_entityIds[ii]);
                        }
                    } else {
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

    int constexpr kFilterBufLength = 64;
    static char sEntityTypeFilterBuf[kFilterBufLength] = {0};
    ImGui::InputText("Type search", sEntityTypeFilterBuf, kFilterBufLength);
    std::string_view typeFilter(sEntityTypeFilterBuf);
    if (ImGui::BeginListBox("Entity type")) {
        for (int typeIx = 0; typeIx < ne::gkNumEntityTypes; ++typeIx) {
            char const *typeName = ne::gkEntityTypeNames[typeIx];
            if (string_ci::Contains(std::string_view(typeName), typeFilter)) {
                bool selected = typeIx == _selectedEntityTypeIx;
                bool clicked = ImGui::Selectable(typeName, selected);
                if (clicked) {
                    _selectedEntityTypeIx = typeIx;
                }
            }
        }
        ImGui::EndListBox();
    }

    // Add Entities
    if (ImGui::Button("Add Entity")) {
        ne::Entity* entity = _g->_neEntityManager->AddEntity((ne::EntityType)_selectedEntityTypeIx);
        entity->_name = "new_entity";
        entity->_editorId._id = _nextEditorId++;
        _entityIds.push_back(entity->_id);        
        // place entity at center of view
        Vec3 newPos;
        bool projectSuccess = geometry::ProjectScreenPointToXZPlane(_g->_windowWidth / 2, _g->_windowHeight / 2, _g->_windowWidth, _g->_windowHeight, _g->_scene->_camera, &newPos);
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
        EditorIdMap editorIdMap;

        struct Duplicate {
            ne::EntityId src;
            ne::EntityId dst;
            EditorId dstEditorId;
        };
        std::vector<Duplicate> prevNewPairs;
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

            Duplicate& dup = prevNewPairs.emplace_back();
            dup.src = selectedId;
            dup.dst = newEntity->_id;
            dup.dstEditorId = EditorId(_nextEditorId++);

            editorIdMap.emplace(selectedEntity->_editorId._id, dup.dstEditorId._id);
        }

        gEditorIdMap = &editorIdMap;
        for (auto &prevNewPair : prevNewPairs) {
            ne::EntityId prevId = prevNewPair.src;
            ne::EntityId newId = prevNewPair.dst;
            ne::Entity* selectedEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(prevId);
            ne::Entity* newEntity = _g->_neEntityManager->GetActiveOrInactiveEntity(newId);
            {
                serial::Ptree pt = serial::Ptree::MakeNew();
                selectedEntity->Save(pt);
                newEntity->Load(pt);
                pt.DeleteData();
            }

            newEntity->_editorId = prevNewPair.dstEditorId;

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
        gEditorIdMap = nullptr;
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
        _selectedEntityIds.clear();
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
                    if (ne::BaseEntity *e = _g->_neEntityManager->GetActiveOrInactiveEntity(eId)) {
                        if (example->_initActive) {
                            e->_initActive = true;
                            _g->_neEntityManager->TagForActivate(eId, /*initOnActivate=*/true);
                        } else {
                            e->_initActive = false;
                            _g->_neEntityManager->TagForDeactivate(eId);
                        }
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
