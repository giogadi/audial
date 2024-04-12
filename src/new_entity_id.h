#pragma once

#include <cassert>

namespace ne {

#define M_ENTITY_TYPES \
    X(Base) \
    X(Test) \
    X(Light) \
    X(Camera) \
    X(Sequencer) \
    X(ActionSequencer) \
    X(Mech) \
    X(Resource) \
    X(TypingEnemy)  \
    X(FlowPlayer) \
    X(StepSequencer) \
    X(ParamAutomator) \
    X(FlowWall) \
    X(FlowPickup) \
    X(FlowTrigger) \
    X(IntVariable) \
    X(Vfx) \
    X(Viz)

    enum class EntityType : int {
#   define X(a) a,
        M_ENTITY_TYPES
#   undef X
        Count
    };

    struct EntityId {
        bool IsValid() const { return _id >= 0; }
        int _id = -1;
        EntityType _type;
        bool operator==(EntityId const& rhs) const {
            bool v = _id == rhs._id;
            if (v) {
                assert(_type == rhs._type);
            }
            return v;
        }
        bool operator!=(EntityId const& rhs) const {
            return !(*this == rhs);
        }
        bool operator<(EntityId const& rhs) const {
            return _id < rhs._id;
        }
    };

}
