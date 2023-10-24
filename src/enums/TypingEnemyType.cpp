#include "src/enums/TypingEnemyType.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



namespace {

std::unordered_map<std::string, TypingEnemyType> const gStringToTypingEnemyType = {
    
    { "Pull", TypingEnemyType::Pull },
    
    { "Push", TypingEnemyType::Push }
    
};

} // end namespace

char const* gTypingEnemyTypeStrings[] = {
	
    "Pull",
    
    "Push"
    
};

char const* TypingEnemyTypeToString(TypingEnemyType e) {
	return gTypingEnemyTypeStrings[static_cast<int>(e)];
}

TypingEnemyType StringToTypingEnemyType(char const* s) {
    auto iter = gStringToTypingEnemyType.find(s);
    if (iter != gStringToTypingEnemyType.end()) {
    	return gStringToTypingEnemyType.at(s);
    }
    printf("ERROR StringToTypingEnemyType: unrecognized value \"%s\"\n", s);
    return static_cast<TypingEnemyType>(0);
}

bool TypingEnemyTypeImGui(char const* label, TypingEnemyType* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gTypingEnemyTypeStrings, static_cast<int>(TypingEnemyType::Count));
    if (changed) {
        *v = static_cast<TypingEnemyType>(selectedIx);
    }
    return changed;
}


char const* EnumToString(TypingEnemyType e) {
     return TypingEnemyTypeToString(e);
}

void StringToEnum(char const* s, TypingEnemyType& e) {
     e = StringToTypingEnemyType(s);
}