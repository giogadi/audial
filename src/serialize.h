#pragma once

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/array.hpp"

#include "matrix.h"
#include "component.h"
#include "renderer.h"
#include "components/rigid_body.h"
#include "components/beep_on_hit.h"
#include "components/player_controller.h"
#include "components/player_orbit_controller.h"
#include "components/sequencer.h"
#include "audio_util.h"
#include "enums/CollisionLayer_cereal.h"
#include "components/camera_controller.h"
#include "components/hit_counter.h"
#include "components/orbitable.h"
#include "components/events_on_hit.h"
#include "components/activator.h"
#include "components/damage.h"
#include "beat_time_event.h"
#include "script_action.h"
#include "components/on_destroy_event.h"

template<typename Archive>
void serialize(Archive& ar, Vec3& v) {
    ar(CEREAL_NVP(v._x), CEREAL_NVP(v._y), CEREAL_NVP(v._z));
}

template<typename Archive>
void serialize(Archive& ar, Vec4& v) {
    ar(CEREAL_NVP(v._x), CEREAL_NVP(v._y), CEREAL_NVP(v._z), CEREAL_NVP(v._w));
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
std::string save_minimal(Archive const& ar, ComponentType const& e) {
	return std::string(ComponentTypeToString(e));
}

template<typename Archive>
void load_minimal(Archive const& ar, ComponentType& e, std::string const& v) {
	e = StringToComponentType(v.c_str());
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
    ar(CEREAL_NVP(m._modelId), cereal::make_nvp("color", m._color));
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
    ar(CEREAL_NVP(m._localAabb), CEREAL_NVP(m._static), CEREAL_NVP(m._layer));
}

template<typename Archive>
void serialize(Archive& ar, BeepOnHitComponent& m) {
    ar(cereal::make_nvp("synth_channel", m._synthChannel), cereal::make_nvp("notes", m._midiNotes));
}

template<typename Archive>
void serialize(Archive& ar, PlayerControllerComponent& m) {
}

template<typename Archive>
void serialize(Archive& ar, PlayerOrbitControllerComponent& m) {
}

template<typename Archive>
void serialize(Archive& ar, SequencerComponent& m) {
    ar(cereal::make_nvp("events", m._events));
}

template<typename Archive>
void serialize(Archive& ar, CameraControllerComponent& m) {
    ar(cereal::make_nvp("target_entity_name", m._targetName));
    ar(cereal::make_nvp("tracking_factor", m._trackingFactor));
}

template<typename Archive>
void serialize(Archive& ar, HitCounterComponent& m) {
    ar(cereal::make_nvp("num_hits", m._hitsRemaining));
}

template<typename Archive>
void serialize(Archive& ar, OrbitableComponent& m) {}

template<typename Archive>
void serialize(Archive& ar, EventsOnHitComponent& m) {
    ar(cereal::make_nvp("denom", m._denom));
    ar(cereal::make_nvp("events", m._events));
}

template<typename Archive>
void serialize(Archive& ar, ActivatorComponent& m) {
    ar(cereal::make_nvp("beat_time", m._activationBeatTime));
    ar(cereal::make_nvp("entity_name", m._entityName));
}

template<typename Archive>
void serialize(Archive& ar, DamageComponent& m) {
    ar(cereal::make_nvp("hp", m._hp));
}

template<typename Archive>
void serialize(Archive& ar, BeatTimeEvent& e) {
    ar(cereal::make_nvp("beat_time", e._beatTime));
    ar(cereal::make_nvp("event", e._e));
}

template<typename Archive>
void save(Archive& ar, OnDestroyEventComponent const& e) {
    SaveActions(ar, e._actions);
}

template<typename Archive>
void load(Archive& ar, OnDestroyEventComponent& e) {
    LoadActions(ar, e._actions);
}