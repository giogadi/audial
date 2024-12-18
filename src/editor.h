#pragma once

#include <vector>
#include <set>

#include "game_manager.h"
#include "new_entity.h"
#include "synth_imgui.h"
#include "input_manager.h"
#include "seq_action.h"

class BoundMeshPNU;

struct Editor {    
    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;

    void Init(GameManager* g);
    void Update(float dt, SynthGuiState& synthGuiState);
    void DrawWindow();
    void DrawMultiEnemyWindow();
    void DrawSeqWindow();
    void HandleEntitySelectAndMove(float dt);
    void HandlePianoInput(SynthGuiState& synthGuiState);
    int GetPianoNotes(int *midiNotes, int maxNotes);
    int IncreasePianoOctave();
    int DecreasePianoOctave();

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

    std::vector<std::unique_ptr<SeqAction>> _heldActions;
    float _heldActionsTimer = -1.f;

private:
    ne::EntityId _requestedNewSelection;
    std::vector<ne::Entity*> _sameEntities;  // only to avoid reallocating
};
