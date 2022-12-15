#pragma once

#include "seq_action.h"
#include "entities/typing_enemy.h"

// ONLY SAFE TO EXECUTE ONE TIME!!!!
struct SpawnEnemySeqAction : public SeqAction {
    TypingEnemyEntity _enemy;
    bool _done = false;
    virtual Type GetType() const override { return Type::SpawnEnemy; }
    virtual void Execute(GameManager& g) override;
    virtual void Load(GameManager& g, LoadInputs const& loadInputs, std::istream& input) override;

    static bool _sStaticDataLoaded;
};
