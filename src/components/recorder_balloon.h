#pragma once

#include "component.h"

class ModelComponent;
class TransformComponent;
class SequencerComponent;

class RecorderBalloonComponent : public Component {
public:
    virtual ~RecorderBalloonComponent() override {}

    virtual ComponentType Type() const override {
        return ComponentType::RecorderBalloon;
    };

    virtual void Update(float dt) override;

    // virtual void Destroy() override;
    // virtual void EditDestroy() override;

    virtual bool ConnectComponents(EntityId id, Entity& e, GameManager& g) override;   

    // If true, request that we try reconnecting the entity's components.
    // virtual bool DrawImGui() override;

    // virtual void OnEditPick() {}
    // virtual void EditModeUpdate(float dt) {}

    // virtual void Save(boost::property_tree::ptree& pt) const override;
    // virtual void Load(boost::property_tree::ptree const& pt) override;

    void OnHit(EntityId other);

    void MaybeCommitSequence();

    // serialized
    int _numDesiredHits = 1;
    double _denom = 0.25;

    // non-serialized
    bool _wasHit = false;
    std::weak_ptr<TransformComponent> _transform;
    std::weak_ptr<ModelComponent> _model;    
    std::weak_ptr<SequencerComponent> _sequencer;
    GameManager* _g = nullptr;

    int _numHits = 0;
    struct RecordInfo {
        bool _hit = false;
        int _measureNumber = -1;
    };
    std::vector<RecordInfo> _recordedNotes;
};