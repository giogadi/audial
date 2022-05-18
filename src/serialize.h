#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"

#include "matrix.h"
#include "component.h"
#include "renderer.h"
#include "components/rigid_body.h"
#include "components/beep_on_hit.h"
#include "components/player_controller.h"
#include "components/sequencer.h"
#include "audio_util.h"

template<typename Archive>
void serialize(Archive& ar, Vec3& v) {
    ar(CEREAL_NVP(v._x), CEREAL_NVP(v._y), CEREAL_NVP(v._z));
}

template<typename Archive>
void serialize(Archive& ar, Mat4& m) {
    ar(CEREAL_NVP(m._m00));
    ar(CEREAL_NVP(m._m10));
    ar(CEREAL_NVP(m._m20));
    ar(CEREAL_NVP(m._m30));

    ar(CEREAL_NVP(m._m01));
    ar(CEREAL_NVP(m._m11));
    ar(CEREAL_NVP(m._m21));
    ar(CEREAL_NVP(m._m31));

    ar(CEREAL_NVP(m._m02));
    ar(CEREAL_NVP(m._m12));
    ar(CEREAL_NVP(m._m22));
    ar(CEREAL_NVP(m._m32));

    ar(CEREAL_NVP(m._m03));
    ar(CEREAL_NVP(m._m13));
    ar(CEREAL_NVP(m._m23));
    ar(CEREAL_NVP(m._m33));
}

template<typename Archive>
void serialize(Archive& ar, TransformComponent& t) {
    ar(CEREAL_NVP(t._transform));
}

template<typename Archive>
void serialize(Archive& ar, VelocityComponent& v) {
    ar(CEREAL_NVP(v._linear), CEREAL_NVP(v._angularY));
}

template<typename Archive>
void serialize(Archive& ar, ModelComponent& m) {
    ar(CEREAL_NVP(m._modelId));
}

template<typename Archive>
void serialize(Archive& ar, LightComponent& m) {
    ar(CEREAL_NVP(m._ambient), CEREAL_NVP(m._diffuse));
}

template<typename Archive>
void serialize(Archive& ar, CameraComponent& m) {
    
}

template<typename Archive>
void serialize(Archive& ar, Aabb& m) {
    ar(CEREAL_NVP(m._min), CEREAL_NVP(m._max));
}

template<typename Archive>
void serialize(Archive& ar, RigidBodyComponent& m) {
    ar(CEREAL_NVP(m._localAabb), CEREAL_NVP(m._static));
}

template<typename Archive>
void serialize(Archive& ar, BeepOnHitComponent& m) {
    // TODO: can we have an empty serialize?
}

template<typename Archive>
void serialize(Archive& ar, PlayerControllerComponent& m) {
    // TODO: can we have an empty serialize?
}

namespace audio {
template<typename Archive>
void save(Archive& ar, Event const& e) {
    ar(cereal::make_nvp("type",std::string(EventTypeToString(e.type))));
    ar(cereal::make_nvp("channel",e.channel));
    ar(cereal::make_nvp("tick_time",e.timeInTicks));
    if (e.type == EventType::SynthParam) {
        ar(cereal::make_nvp("synth_param", std::string(SynthParamTypeToString(e.param))));
        ar(cereal::make_nvp("value", e.newParamValue));
    } else {
        ar(cereal::make_nvp("midi_note", e.midiNote));
    }
}

template<typename Archive>
void load(Archive& ar, Event& e) {
    std::string eventTypeName;
    ar(eventTypeName);
    e.type = StringToEventType(eventTypeName.c_str());
    ar(cereal::make_nvp("channel",e.channel));
    ar(cereal::make_nvp("tick_time",e.timeInTicks));
    if (e.type == EventType::SynthParam) {
        std::string synthParamTypeName;
        ar(synthParamTypeName);
        e.param = StringToSynthParamType(synthParamTypeName.c_str());        
        ar(cereal::make_nvp("value", e.newParamValue));
    } else {
        ar(cereal::make_nvp("midi_note", e.midiNote));
    }
}
} // namespace audio

template<typename Archive>
void serialize(Archive& ar, SequencerComponent& m) {
    ar(cereal::make_nvp("events", m._events));
}