#pragma once

#include <queue>
#include <optional>

#include "new_entity.h"
#include "beat_time_event.h"

struct StepSequencerEntity : ne::Entity {
    struct SeqStep {
        SeqStep() {}
        static constexpr int kNumNotes = 4;
        std::array<int,kNumNotes> _midiNote = {-1, -1, -1, -1};
        float _velocity = 1.f;
    };
    struct SeqStepChange {
        SeqStep _step;
        bool _changeNote = true;
        bool _changeVelocity = true;
        bool _temporary = true;
    };

    static bool SeqImGui(char const* label, bool drumKit, std::vector<SeqStep>& sequence);
    
    // Serialized
    std::vector<SeqStep> _initialMidiSequenceDoNotChange;
    double _stepBeatLength = 0.25;
    std::vector<int> _channels;
    bool _isSynth = true;  // false is drumkit
    double _noteLength = 0.25;
    double _initialLoopStartBeatTime = 4.0;
    bool _startMute = false;
    bool _editorMute = false;
    int _initMaxNumVoices = -1;
    float _initGain = 1.f;
    bool _quantizeTempStepChanges = true;

    static int constexpr kChangeQueueSize = 2;

    // non-serialized
    bool _mute = false;
    int _currentIx = 0;
    double _loopStartBeatTime = 0.0;    
    std::vector<SeqStep> _permanentSequence;
    std::vector<SeqStep> _tempSequence;
    SeqStepChange _changeQueue[kChangeQueueSize];
    int _changeQueueHeadIx = 0;  // points to first element
    int _changeQueueTailIx = 0;  // points to ix one past last element (cyclical)
    int _changeQueueCount = 0;
    int _maxNumVoices = -1;
    float _gain = 1.f;
    bool _seqNeedsReset = true;

    enum StepSaveType { Temporary, Permanent };
    void SetNextSeqStep(GameManager& g, SeqStep step, StepSaveType saveType);
    void SetNextSeqStepVelocity(GameManager& g, float v, StepSaveType saveType);
    void SetAllVelocitiesPermanent(float newVelocity);
    void SetAllStepsPermanent(SeqStep const& newStep);
    // TODO can we do this with a move instead of copying sequence?
    void SetSequencePermanent(std::vector<SeqStep> const& newSequence);
    void SetSequencePermanentWithStartOffset(std::vector<SeqStep> const& newSequence);

    virtual void InitDerived(GameManager& g) override;
    virtual void Update(GameManager& g, float dt) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    static void WriteSeqStep(SeqStep const& step, bool writeNoteName, std::ostream& output);
    static bool TryReadSeqStep(std::istream& input, SeqStep& step);

    // Assumes input contains _only_ the sequence, nothing else.
    static void LoadSequenceFromInput(
        std::istream& input, std::vector<SeqStep>& sequence);    

private:
    void EnqueueChange(SeqStepChange const& change);
    void PlayStep(GameManager& g, SeqStep const& seqStep);
};
