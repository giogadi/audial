#pragma once

#include <istream>

#include "new_entity.h"
#include "beat_time_event.h"

/* struct SeqAction { */
/*     // virtual void Init(GameManager& g) {}; */
/*     enum class Result { Continue, Done }; */
/*     enum class Type { */
/*         AutomateParam */
/*     }; */
/*     virtual Type GetType() const = 0; */
/*     virtual Result Execute(GameManager& g) = 0; */
/*     virtual void Load(std::istream& input) {}; */
/*     // virtual void Save(std::ostream& output) const {}; */
/* }; */

/* struct AutomateParamSeqAction : public SeqAction { */
/*     Type GetType() const override { return Type::AutomateParam; } */
/*     Result Execute(GameManager& g) override; */
/*     void Load(std::istream& input) override; */
/*     // void Save(std::ostream& output) const override; */
    
/*     //serialized */
/*     double _initialValue = 0.0; */
/*     int _lengthInFrames = 0; */

/*     //non-serialized */
/*     int _frameIx = 0; */
/*     double _currentValue = 0.0; */
/* }; */

struct SeqAction {
    enum class Type {
        SpawnAutomator
    };
    virtual Type GetType() const = 0;
    virtual void Execute(GameManager& g) = 0;
    virtual void Load(std::istream& input) {}

    virtual ~SeqAction() {}
};

struct SpawnAutomatorSeqAction : public SeqAction {
    float _startVelocity = 0.f;
    float _endVelocity = 1.f;
    double _desiredAutomateTime = 1.0;
    std::string _seqEntityName;
    
    virtual Type GetType() const override { return Type::SpawnAutomator; }
    virtual void Execute(GameManager& g) override;
    virtual void Load(std::istream& input) override;
};

struct ActionSequencerEntity : ne::Entity {
    struct BeatTimeAction {
        BeatTimeAction(BeatTimeAction const& rhs) = delete;
        BeatTimeAction(BeatTimeAction&&) = default;
        BeatTimeAction& operator=(BeatTimeAction&&) = default;
        BeatTimeAction() {}
        std::unique_ptr<SeqAction> _pAction;
        double _beatTime = 0.0;
    };
    ActionSequencerEntity() {}
    ActionSequencerEntity(ActionSequencerEntity&&) = default;
    ActionSequencerEntity& operator=(ActionSequencerEntity&&) = default;
    ActionSequencerEntity(ActionSequencerEntity const& rhs) = delete;

    // serialized
    std::string _seqFilename;

    // non-serialized
    std::vector<BeatTimeAction> _actions;
    int _currentIx = 0;
    // double _currentLoopStartBeatTime = -1.0;

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) override; */    
};
