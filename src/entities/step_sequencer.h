#pragma once

#include <queue>
#include <optional>

#include "enums/audio_SynthParamType.h"
#include "new_entity.h"
#include "beat_time_event.h"

struct StepSequencerEntity : ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::StepSequencer; }
    static ne::EntityType StaticType() { return ne::EntityType::StepSequencer; }

    static constexpr int kNumParamTracks = 4;

    struct SynthParamValue {
        float _value = 0.f;
        bool _active = false;
        void Save(serial::Ptree pt) const;
        void Load(serial::Ptree pt);
        bool ImGui();
    };
        
    struct SeqStep {
        SeqStep() {}
        static constexpr int kNumNotes = 4;
        std::array<int,kNumNotes> _midiNote = {-1, -1, -1, -1};
        float _velocity = 1.f;
        // params set the new value just for this step, and then it reverts to the patch value.        
        std::array<SynthParamValue, kNumParamTracks> _params;
        
        void Save(serial::Ptree pt) const;
        void Load(serial::Ptree pt);
    };
    struct SeqStepChange {
        SeqStep _step;
        bool _changeNote = true;
        bool _changeVelocity = true;
        bool _temporary = true;
    };

    static bool SeqImGui(char const* label, bool drumKit, std::vector<SeqStep>& sequence);
    
    // Serialized
    struct ParamTrackType {
        bool _active = false;
        audio::SynthParamType _type;
    };
    std::array<ParamTrackType, kNumParamTracks> _paramTrackTypes;
    std::vector<SeqStep> _initialMidiSequenceDoNotChange;
    bool _enableLateChanges = true;
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

    static int constexpr kChangeQueueSize = 3;

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
    double _lastPlayedNoteTime = 0.0;
    bool _primePorta = false;

    enum StepSaveType { Temporary, Permanent };
    void SetNextSeqStep(GameManager& g, SeqStep step, StepSaveType saveType, bool changeNote, bool changeVelocity);
    void SetNextSeqStepVelocity(GameManager& g, float v, StepSaveType saveType);
    void SetAllVelocitiesPermanent(float newVelocity);
    void SetAllStepsPermanent(SeqStep const& newStep);
    // TODO can we do this with a move instead of copying sequence?
    void SetSequencePermanent(std::vector<SeqStep> const& newSequence);
    void SetSequencePermanentWithStartOffset(std::vector<SeqStep> const& newSequence);

    virtual void InitDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override;
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
