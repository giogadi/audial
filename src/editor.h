#pragma once

#include <vector>
#include <set>

#include "game_manager.h"
#include "new_entity.h"
#include "synth_imgui.h"

class BoundMeshPNU;

struct Editor {
    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;

    void Init(GameManager* g);
    void Update(float dt, SynthGuiState& synthGuiState);
    void DrawWindow();
    void DrawMultiEnemyWindow();
    void HandleEntitySelectAndMove(float dt);
    void HandlePianoInput(SynthGuiState& synthGuiState);

    bool IsEntitySelected(ne::EntityId entityId);

    ne::Entity* ImGuiEntitySelector(char const* buttonLabel, char const* popupLabel);

    void SelectEntity(ne::Entity& e);

    GameManager* _g = nullptr;
    bool _entityWindowVisible = false;
    std::set<ne::EntityId> _selectedEntityIds;
    int _selectedEntityTypeIx = 0;
    BoundMeshPNU const* _axesMesh = nullptr;
    std::string _saveFilename;
    std::vector<ne::EntityId> _entityIds;
    bool _enableFlowSectionFilter = false;
    int _flowSectionFilterId = -1;
    bool _showControllerInputs = false;  // if false, show keyboard inputs
    int64_t _nextEditorId = -1;

private:
    ne::EntityId _requestedNewSelection;
};
