#include "src/enums/CollisionLayer.h"

#include <unordered_map>
#include <string>



namespace {

std::unordered_map<std::string, CollisionLayer> const gStringToCollisionLayer = {
    
    { "None", CollisionLayer::None },
    
    { "Solid", CollisionLayer::Solid },
    
    { "BodyAttack", CollisionLayer::BodyAttack }
    
};

} // end namespace

char const* gCollisionLayerStrings[] = {
	
    "None",
    
    "Solid",
    
    "BodyAttack"
    
};

char const* CollisionLayerToString(CollisionLayer e) {
	return gCollisionLayerStrings[static_cast<int>(e)];
}

CollisionLayer StringToCollisionLayer(char const* s) {
	return gStringToCollisionLayer.at(s);
}

