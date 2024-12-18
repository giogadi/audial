#pragma once



enum class CollisionLayer : int {
    
    None,
    
    Solid,
    
    BodyAttack,
    
    Count
};
extern char const* gCollisionLayerStrings[];
char const* CollisionLayerToString(CollisionLayer e);
CollisionLayer StringToCollisionLayer(char const* s);

bool CollisionLayerImGui(char const* label, CollisionLayer* v);


char const* EnumToString(CollisionLayer e);
void StringToEnum(char const* s, CollisionLayer& e);