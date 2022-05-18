#pragma once

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
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

// template<typename Archive>
// void serialize(Archive& ar, audio::Event& e) {
//     ar(cereal::make_nvp("type",e.type));
//     ar(cereal::make_nvp("channel",e.channel));
//     ar(cereal::make_nvp("tick_time",e.timeInTicks));
//     if (e.type == audio::EventType::SynthParam) {
//         ar(cereal::make_nvp("param"))
//     }
// }

template<typename Archive>
void serialize(Archive& ar, SequencerComponent& m) {
    // TODO: add the events in here
}