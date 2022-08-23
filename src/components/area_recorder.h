#pragma once

#include <vector>

#include "component.h"
#include "aabb.h"
#include "audio_util.h"
#include "beat_time_event.h"

namespace audio {
    struct Event;
}

class AreaRecorderComponent : public Component {
public:
    virtual ComponentType Type() const override { return ComponentType::AreaRecorder; };

    virtual ~AreaRecorderComponent() override {}
    virtual void Update(float dt) override;
    // virtual void Destroy() {};
    // virtual void EditDestroy() { Destroy(); };
    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;
    
    // If true, request that we try reconnecting the entity's components.
    virtual bool DrawImGui() override;
    // virtual void OnEditPick() {}
    // virtual void EditModeUpdate(float dt) {}

    virtual void Save(serial::Ptree pt) const override;
    virtual void Load(serial::Ptree pt) override;

    void Record(audio::Event const& e);

    // Serialize
    Aabb _localAabb;

    // Non-serialize
    std::vector<audio::Event> _recordedEvents;
    bool _recording = false;
    int _currentBeatNum = 0;
    double _currentRecordingMeasureStart = -1.0;
    int _currentPlaybackIndex = 0;
    std::vector<BeatTimeEvent> _sequence;
    GameManager* _g = nullptr;
};