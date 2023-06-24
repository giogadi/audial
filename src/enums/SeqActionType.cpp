#include "src/enums/SeqActionType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, SeqActionType> const gStringToSeqActionType = {
    
    { "SpawnAutomator", SeqActionType::SpawnAutomator },
    
    { "RemoveEntity", SeqActionType::RemoveEntity },
    
    { "ChangeStepSequencer", SeqActionType::ChangeStepSequencer },
    
    { "SetAllSteps", SeqActionType::SetAllSteps },
    
    { "SetStepSequence", SeqActionType::SetStepSequence },
    
    { "SetStepSequencerMute", SeqActionType::SetStepSequencerMute },
    
    { "NoteOnOff", SeqActionType::NoteOnOff },
    
    { "BeatTimeEvent", SeqActionType::BeatTimeEvent },
    
    { "WaypointControl", SeqActionType::WaypointControl },
    
    { "PlayerSetKillZone", SeqActionType::PlayerSetKillZone },
    
    { "PlayerSetSpawnPoint", SeqActionType::PlayerSetSpawnPoint },
    
    { "SetNewFlowSection", SeqActionType::SetNewFlowSection },
    
    { "AddToIntVariable", SeqActionType::AddToIntVariable },
    
    { "CameraControl", SeqActionType::CameraControl },
    
    { "SpawnEnemy", SeqActionType::SpawnEnemy },
    
    { "SetEntityActive", SeqActionType::SetEntityActive },
    
    { "ChangeStepSeqMaxVoices", SeqActionType::ChangeStepSeqMaxVoices },
    
    { "ChangePatch", SeqActionType::ChangePatch },
    
    { "VfxPulse", SeqActionType::VfxPulse },
    
    { "Trigger", SeqActionType::Trigger }
    
};

} // end namespace

char const* gSeqActionTypeStrings[] = {
	
    "SpawnAutomator",
    
    "RemoveEntity",
    
    "ChangeStepSequencer",
    
    "SetAllSteps",
    
    "SetStepSequence",
    
    "SetStepSequencerMute",
    
    "NoteOnOff",
    
    "BeatTimeEvent",
    
    "WaypointControl",
    
    "PlayerSetKillZone",
    
    "PlayerSetSpawnPoint",
    
    "SetNewFlowSection",
    
    "AddToIntVariable",
    
    "CameraControl",
    
    "SpawnEnemy",
    
    "SetEntityActive",
    
    "ChangeStepSeqMaxVoices",
    
    "ChangePatch",
    
    "VfxPulse",
    
    "Trigger"
    
};

char const* SeqActionTypeToString(SeqActionType e) {
	return gSeqActionTypeStrings[static_cast<int>(e)];
}

SeqActionType StringToSeqActionType(char const* s) {
    auto iter = gStringToSeqActionType.find(s);
    if (iter != gStringToSeqActionType.end()) {
    	return gStringToSeqActionType.at(s);
    }
    printf("ERROR StringToSeqActionType: unrecognized value \"%s\"\n", s);
    return static_cast<SeqActionType>(0);
}

bool SeqActionTypeImGui(char const* label, SeqActionType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gSeqActionTypeStrings, static_cast<int>(SeqActionType::Count));
    if (changed) {
        *v = static_cast<SeqActionType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(SeqActionType e) {
     return SeqActionTypeToString(e);
}

void StringToEnum(char const* s, SeqActionType& e) {
     e = StringToSeqActionType(s);
}