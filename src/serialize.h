#pragma once

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/polymorphic.hpp"

#include "matrix.h"
#include "component.h"

template<typename Archive>
void serialize(Archive& ar, Entity& e) {
    ar(CEREAL_NVP(e._components));
}

template<typename Archive>
void serialize(Archive& ar, EntityManager& e) {
    ar(CEREAL_NVP(e._entities));
}

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
CEREAL_REGISTER_TYPE(TransformComponent);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, TransformComponent);

template<typename Archive>
void serialize(Archive& ar, VelocityComponent& v) {
    ar(CEREAL_NVP(v._linear));
}
CEREAL_REGISTER_TYPE(VelocityComponent);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, VelocityComponent);