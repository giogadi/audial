#pragma once



enum class CollisionLayer : int {
    
    None,
    
    Solid,
    
    BodyAttack,
    
    Count
};
char const* CollisionLayerToString(CollisionLayer e);
CollisionLayer StringToCollisionLayer(char const* s);

