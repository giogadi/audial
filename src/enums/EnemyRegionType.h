#pragma once



enum class EnemyRegionType : int {
    
    None,
    
    TriggerEntity,
    
    BoundedXAxis,
    
    Count
};
extern char const* gEnemyRegionTypeStrings[];
char const* EnemyRegionTypeToString(EnemyRegionType e);
EnemyRegionType StringToEnemyRegionType(char const* s);

bool EnemyRegionTypeImGui(char const* label, EnemyRegionType* v);


char const* EnumToString(EnemyRegionType e);
void StringToEnum(char const* s, EnemyRegionType& e);