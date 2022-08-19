#include "area_recorder.h"

void AreaRecorderComponent::Update(float dt) {

}

bool AreaRecorderComponent::DrawImGui() {
    return false;
}

void AreaRecorderComponent::Save(serial::Ptree pt) const {

}

void AreaRecorderComponent::Load(serial::Ptree pt) {
    
}

void AreaRecorderComponent::Record(audio::Event const& e) {
    printf("UNIMPLEMENTED\n");
}