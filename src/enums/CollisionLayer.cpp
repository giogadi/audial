#include "src/enums/CollisionLayer.h"

#include <unordered_map>
#include <string>
#include <cstdio>

#include "imgui/imgui.h"



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
    auto iter = gStringToCollisionLayer.find(s);
    if (iter != gStringToCollisionLayer.end()) {
    	return gStringToCollisionLayer.at(s);
    }
    printf("ERROR StringToCollisionLayer: unrecognized value \"%s\"\n", s);
    return static_cast<CollisionLayer>(0);
}

char const* EnumToString(CollisionLayer e) {
     return CollisionLayerToString(e);
}

void StringToEnum(char const* s, CollisionLayer& e) {
     e = StringToCollisionLayer(s);
}

bool CollisionLayerImGui(char const* label, CollisionLayer* v) {
    int selectedIx = static_cast<int>(*v);
    bool changed = ImGui::Combo(label, &selectedIx, gCollisionLayerStrings, static_cast<int>(CollisionLayer::Count));
    if (changed) {
        *v = static_cast<CollisionLayer>(selectedIx);
    }
    return changed;
}

