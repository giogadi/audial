#pragma once

#include <vector>

#include "new_entity.h"
#include "beat_time_event.h"

struct TypingEnemyEntity : public ne::Entity {
    struct Action {
        enum class Type { Seq, Note };
        Action() {
            _type = Type::Seq;
            _seqMidiNote = -1;
            _seqVelocity = 1.f;
        }
        
        Type _type = Type::Seq;
        union {
            struct {
                ne::EntityId _seqId;
                int _seqMidiNote;
                float _seqVelocity;
            };
            struct {
                int _noteChannel;
                int _noteMidiNote;
                double _noteLength;
                float _velocity;
            };
        };
    };
    
    // serialized
    std::string _text;
    double _eventStartDenom = 0.125;
    std::vector<BeatTimeEvent> _hitEvents;
    std::vector<BeatTimeEvent> _deathEvents;
    // Each time enemy gets hit, we switch to the next action in this list.
    std::vector<Action> _hitActions;
    double _activeBeatTime = -1.0;
    double _inactiveBeatTime = -1.0;
    
    // non-serialized
    int _numHits = 0;
    Vec3 _velocity;

    bool IsActive(GameManager& g) const;
    void OnHit(GameManager& g);
    
    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    /* virtual void OnEditPick(GameManager& g) {} */
    /* virtual void DebugPrint(); */

    /* virtual void SaveDerived(serial::Ptree pt) const {}; */
    /* virtual void LoadDerived(serial::Ptree pt) {}; */
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) { return ImGuiResult::Done; } */
};