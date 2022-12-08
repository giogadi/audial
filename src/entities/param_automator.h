#pragma once

#include "new_entity.h"
#include "enums/audio_SynthParamType.h"

struct ParamAutomatorEntity : public ne::Entity {
    // serialized
    float _startValue = 0.f;
    float _endValue = 1.f;
    double _desiredAutomateTime = 1.0;
    bool _synth = false; // if false, we're modulating velocity of a seq (sigh)
    audio::SynthParamType _synthParam = audio::SynthParamType::Gain;
    int _channel = 0;  // only if _synth == true
    std::string _seqEntityName;

    // non-serialized
    ne::EntityId _seqId;
    double _automateStartTime = 0.0;

    virtual void Init(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    /* virtual void SaveDerived(serial::Ptree pt) const override; */
    /* virtual void LoadDerived(serial::Ptree pt) override; */
    /* virtual ImGuiResult ImGuiDerived(GameManager& g) override; */
};
