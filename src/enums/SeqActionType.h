#pragma once



enum class SeqActionType : int {
    
    SpawnAutomator,
    
    RemoveEntity,
    
    ChangeStepSequencer,
    
    SetAllSteps,
    
    SetStepSequence,
    
    SetStepSequencerMute,
    
    NoteOnOff,
    
    BeatTimeEvent,
    
    WaypointControl,
    
    PlayerSetKillZone,
    
    PlayerSetSpawnPoint,
    
    SetNewFlowSection,
    
    AddToIntVariable,
    
    CameraControl,
    
    SpawnEnemy,
    
    Count
};
extern char const* gSeqActionTypeStrings[];
char const* SeqActionTypeToString(SeqActionType e);
SeqActionType StringToSeqActionType(char const* s);

bool SeqActionTypeImGui(char const* label, SeqActionType* v);
