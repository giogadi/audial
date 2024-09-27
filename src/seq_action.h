#pragma once

#include <string>
#include <istream>

#include "game_manager.h"
#include "new_entity.h"
#include "beat_time_event.h"

#include "enums/SeqActionType.h"

struct SeqAction;

struct BeatTimeAction {
    BeatTimeAction(BeatTimeAction const& rhs) = delete;
    BeatTimeAction(BeatTimeAction&&) = default;
    BeatTimeAction& operator=(BeatTimeAction&&) = default;
    BeatTimeAction();
    std::unique_ptr<SeqAction> _pAction;
    double _beatTime = 0.0;

    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();
};

struct SeqAction {

    virtual SeqActionType Type() const = 0;
    
    struct LoadInputs {
        double _beatTimeOffset = 0.0;
        bool _defaultEnemiesSave = false;
        int _sectionId = -1;
        int _sampleRate = -1;
    };

    bool _oneTime = false;
    
    void Init(GameManager& g) {
        _hasExecuted = false;
        InitDerived(g);
        _init = true;
    }
    void Execute(GameManager& g) {
        assert(_init);
        if (!_oneTime || !_hasExecuted) {
            ExecuteDerived(g);
            _hasExecuted = true;
        }
    }
    virtual void ExecuteRelease(GameManager& g) {}

    virtual ~SeqAction() {}

    static void LoadAndInitActions(GameManager& g, std::istream& input, std::vector<BeatTimeAction>& actions);
    static std::unique_ptr<SeqAction> LoadAction(LoadInputs const& loadInputs, std::istream& input);

    virtual bool ImGui() { return false; };
    static bool ImGui(char const* label, std::vector<std::unique_ptr<SeqAction>>& actions);    

    void Save(serial::Ptree pt) const;
    static void SaveActionsInChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>> const& actions);

    static std::unique_ptr<SeqAction> New(SeqActionType actionType);
    static std::unique_ptr<SeqAction> Load(serial::Ptree pt);
    static bool LoadActionsFromChildNode(serial::Ptree pt, char const* childName, std::vector<std::unique_ptr<SeqAction>>& actions);

    static std::unique_ptr<SeqAction> Clone(SeqAction const& action);
    
protected:
    virtual void LoadDerived(
        LoadInputs const& loadInputs, std::istream& input) {
        printf("UNIMPLEMENTED TEXT-BASED LOAD - SeqActionType::%s!\n", SeqActionTypeToString(Type()));
    };
    virtual void LoadDerived(serial::Ptree pt) {
        printf("UNIMPLEMENTED LOAD - SeqActionType::%s!\n", SeqActionTypeToString(Type()));
    }
    virtual void SaveDerived(serial::Ptree pt) const {
        printf("UNIMPLEMENTED SAVE - SeqActionType::%s!\n", SeqActionTypeToString(Type()));
    }
    virtual void InitDerived(GameManager& g) {}    
    virtual void ExecuteDerived(GameManager& g) = 0;
    
private:
    bool _init = false;
    bool _hasExecuted = false;
};
