#pragma once

#include "seq_action.h"
#include "entities/typing_enemy.h"

// ONLY SAFE TO EXECUTE ONE TIME!!!!
struct SpawnEnemySeqAction : public SeqAction {
    virtual SeqActionType Type() const override { return SeqActionType::SpawnEnemy; }
    // Serialized        
    TypingEnemyEntity _enemy;
    
    bool _done = false;
    virtual void ExecuteDerived(GameManager& g) override;
    virtual void LoadDerived(LoadInputs const& loadInputs, std::istream& input) override;

    static bool _sNoteTablesLoaded;
};
