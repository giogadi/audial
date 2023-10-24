#pragma once



enum class TypingEnemyType : int {
    
    Pull,
    
    Push,
    
    Count
};
extern char const* gTypingEnemyTypeStrings[];
char const* TypingEnemyTypeToString(TypingEnemyType e);
TypingEnemyType StringToTypingEnemyType(char const* s);

bool TypingEnemyTypeImGui(char const* label, TypingEnemyType* v);


char const* EnumToString(TypingEnemyType e);
void StringToEnum(char const* s, TypingEnemyType& e);