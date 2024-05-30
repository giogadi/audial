#pragma once

#include <vector>

#include "new_entity.h"
#include "seq_action.h"
#include "input_manager.h"
#include "enums/HitResponseType.h"

struct FlowPlayerEntity;

struct HitResponse {
    HitResponseType _type = HitResponseType::Pull;
    float _speed = -1.f; // if <0, use player's default dash velocity
    float _pushAngleDeg = -1.f; // if <0, just use angle-to-player
    bool _stopOnPass = true;
    bool _stopAfterTimer = true;
    void Load(serial::Ptree pt);
    void Save(serial::Ptree pt) const;
    bool ImGui(char const* label);
};

struct HitResult {
    HitResponse _response;
    std::vector<std::unique_ptr<SeqAction>> _heldActions;
};

struct TypingEnemyEntity : public ne::Entity {
    virtual ne::EntityType Type() override { return ne::EntityType::TypingEnemy; }
    static ne::EntityType StaticType() { return ne::EntityType::TypingEnemy; }
    
    enum class HitBehavior { SingleAction, AllActions };
    
    struct Props {
        HitResponse _hitResponse;

        bool _useLastHitResponse = false;
        HitResponse _lastHitResponse;
        
        std::string _keyText;
        std::string _buttons;
        std::vector<std::unique_ptr<SeqAction>> _hitActions;
        std::vector<std::unique_ptr<SeqAction>> _allHitActions;
        std::vector<std::unique_ptr<SeqAction>> _offCooldownActions;
        std::vector<std::unique_ptr<SeqAction>> _comboEndActions;
        HitBehavior _hitBehavior = HitBehavior::SingleAction; // TODO
        bool _destroyAfterTyped = true;
        double _flowCooldownBeatTime = -1.0;
        bool _resetCooldownOnAnyHit = false;
        EditorId _activeRegionEditorId;
        bool _lockPlayerOnEnterRegion = false;
        bool _showBeatsLeft = false;
        float _cooldownQuantizeDenom = 0.f;
        bool _initHittable = true;
        double _timedHittableTime = -1.0;
    };
    Props _p;
    
    // non-serialized
    struct State {
        int _numHits = 0;
        Vec3 _velocity; // spatial, not audio
        Vec4 _currentColor;
        bool _useHitActionsOnInitHack = false;
        double _timeOfLastHit = -1.0;
        double _flowCooldownStartBeatTime = -1.0;
        bool _hittable = true;
        ne::EntityId _activeRegionId;
        float _bumpTimer = -1.f;
        Vec3 _bumpDir;
    };
    State _s;

    HitResult OnHit(GameManager& g);
    void DoHitActions(GameManager& g, std::vector<std::unique_ptr<SeqAction>>& clones);
    void OnHitOther(GameManager& g);
    void DoComboEndActions(GameManager& g);
    InputManager::Key GetNextKey() const;
    InputManager::ControllerButton GetNextButton() const;
    bool CanHit(FlowPlayerEntity const& player, GameManager& g) const;
    void SetHittable(bool hittable);
    void Bump(Vec3 const& bumpDir);

    
    virtual void InitDerived(GameManager& g) override;
    virtual void UpdateDerived(GameManager& g, float dt) override;
    virtual void Draw(GameManager& g, float dt) override;
    /* virtual void Destroy(GameManager& g) {} */
    virtual void OnEditPick(GameManager& g) override;
    /* virtual void DebugPrint(); */

    virtual void SaveDerived(serial::Ptree pt) const override;
    virtual void LoadDerived(serial::Ptree pt) override;
    virtual ImGuiResult ImGuiDerived(GameManager& g) override;

    TypingEnemyEntity() = default;
    TypingEnemyEntity(TypingEnemyEntity const&) = delete;
    TypingEnemyEntity(TypingEnemyEntity&&) = default;
    TypingEnemyEntity& operator=(TypingEnemyEntity&&) = default;

    static void MultiSelectImGui(GameManager& g, std::vector<TypingEnemyEntity*>& enemies);
};
