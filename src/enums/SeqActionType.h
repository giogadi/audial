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
    
    SetEntityActive,
    
    ChangeStepSeqMaxVoices,
    
    ChangePatch,
    
    VfxPulse,
    
    Trigger,
    
    SetEnemyHittable,
    
    Respawn,
    
    AddMotion,
    
    RandomizeText,
    
    SetBpm,
    
    SetMissTrigger,
    
    Count
};
extern char const* gSeqActionTypeStrings[];
char const* SeqActionTypeToString(SeqActionType e);
SeqActionType StringToSeqActionType(char const* s);

bool SeqActionTypeImGui(char const* label, SeqActionType* v);


char const* EnumToString(SeqActionType e);
void StringToEnum(char const* s, SeqActionType& e);