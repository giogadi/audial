#pragma once

#include "new_entity.h"
#include "editor_id.h"

struct EnemySpawnerEntity : public ne::BaseEntity {
	virtual ne::EntityType Type() override { return ne::EntityType::EnemySpawner; }
	static ne::EntityType StaticType() { return ne::EntityType::EnemySpawner; }

    struct Props {
        std::vector<EditorId> enemyTemplates;
        float spawnCooldown = 0.35f;
    };
    Props _p;

    struct State {
        float timeLeft = 0.5f;
        int currentIndex = 0;
        std::vector<ne::EntityId> enemyTemplates;
    };
    State _s;

    // virtual void OnEditPick(GameManager& g) override;

    virtual void InitDerived(GameManager& g) override;
    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override;

    // virtual void Draw(GameManager& g, float dt) override;

    EnemySpawnerEntity() = default;
    EnemySpawnerEntity(EnemySpawnerEntity const&) = delete;
    EnemySpawnerEntity(EnemySpawnerEntity&&) = default;
    EnemySpawnerEntity& operator=(EnemySpawnerEntity&&) = default;
};
