#pragma once

#include "new_entity.h"
#include "enums/MechType.h"
#include "seq_action.h"

struct MechEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::Mech; }
    static ne::EntityType StaticType() { return ne::EntityType::Mech; }
    
    struct SpawnerProps {

    };
    struct PusherProps {
        float angleDeg;
    };
    struct SinkProps {

    };
    struct Props {
        MechType type = MechType::Spawner;
        char key = 'a';
        double quantize = 0.0;
        std::vector<std::unique_ptr<SeqAction>> actions;
        std::vector<std::unique_ptr<SeqAction>> stopActions;

        union {
            SpawnerProps spawner;
            PusherProps pusher;
            SinkProps sink;
        };
    };

    Props _p;

    void InitTypeSpecificProps();
    void InitTypeSpecificState();
    
    struct SpawnerState {

    };
    struct PusherState {
    };
    struct SinkState {

    };

    struct State {
        // For imgui shenanigans
        char keyBuf[2];

        double actionBeatTime = -1.0;
        ne::EntityId resource;

        union {
            SpawnerState spawner;
            PusherState pusher;
            SinkState sink;
        };
    };
    State _s;
       
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Draw(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    MechEntity() = default;
    MechEntity(MechEntity const&) = delete;
    MechEntity(MechEntity&&) = default;
    MechEntity& operator=(MechEntity&&) = default;
};
