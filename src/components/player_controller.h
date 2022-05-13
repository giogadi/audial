#pragma once

#include <limits>
#include <functional>

#include "component.h"

#include "beep_on_hit.h"

#include "glm/gtc/constants.hpp"
#include "glm/gtx/norm.hpp"

class VelocityPhysicsComponent : public Component {
public:
    VelocityPhysicsComponent(TransformComponent* t, float radius, HitManager* hitMgr)
        : _transform(t)
        , _linear(0.f)
        , _radius(radius)
        , _hitMgr(hitMgr) {
    }

    void SetOnHitCallback(std::function<void()> f) {
        _onHitFn = f;
    }

    // virtual void Update(float dt) override {
    //     _timeSinceLastHit += dt;
    //     if (glm::length2(_linear) < 0.01f*0.01f) {
    //         _linear = glm::vec3(0.f);            
    //         return;
    //     }
    //     glm::vec3 newPos = _transform->GetPos() + dt * _linear;
    //     if (/*_timeSinceLastHit > 0.2f*/ true) {
    //         HitSphere hitSphere;
    //         hitSphere._center = newPos;
    //         hitSphere._radius = _radius;
    //         std::vector<HitSphere> hits;
    //         _hitMgr->HitAndStore(hitSphere, &hits);
    //         if (!hits.empty()) {
    //             HitSphere const& other = hits.front();
    //             _timeSinceLastHit = 0.f;
    //             glm::vec3 dirAwayFromHit = glm::normalize(_transform->GetPos() - other._center);
    //             newPos = other._center + dirAwayFromHit * (_radius + other._radius + 0.1f);
    //             // _linear = -_linear;
    //             _linear = glm::vec3(0.f);
    //         }
    //     }

    //     _transform->SetPos(newPos);        
    // }

    virtual void Update(float dt) override {
        _timeSinceLastHit += dt;
        if (glm::length2(_linear) < 0.01f*0.01f) {
            _linear = glm::vec3(0.f);            
            return;
        }
        glm::vec3 newPos = _transform->GetPos() + dt * _linear;
        if (/*_timeSinceLastHit > 0.2f*/ true) {
            Hitbox hitbox;
            hitbox._min = newPos - glm::vec3(_radius);
            hitbox._max = newPos + glm::vec3(_radius);
            std::vector<Hitbox> hits;
            _hitMgr->HitAndStore(hitbox, &hits);
            if (!hits.empty()) {
                _timeSinceLastHit = 0.f;
                // find minimum penetration axis. ASSUME OVERLAP!!!
                Hitbox const& other = hits.front();
                glm::vec3 diff1 = hitbox._min - other._max;
                glm::vec3 diff2 = hitbox._max - other._min;
                float minDepth = std::numeric_limits<float>::max();
                int minDepthIdx = -1;
                bool minDepth1 = false;
                for (int i = 0; i < 3; ++i) {
                    float d1 = std::abs(diff1[i]);
                    if (d1 < minDepth) {
                        minDepth = d1;
                        minDepthIdx = i;
                        minDepth1 = true;
                    }

                    float d2 = std::abs(diff2[i]);
                    if (d2 < minDepth) {
                        minDepth = d2;
                        minDepthIdx = i;
                        minDepth1 = false;
                    }
                }

                if (minDepth1) {
                    // This means we need to scooch along the POSITIVE minimum axis.
                    newPos[minDepthIdx] += minDepth + 0.001f;
                } else {
                    // This means we need to scooch along the NEGATIVE minimum axis.
                    newPos[minDepthIdx] -= minDepth + 0.001f;
                }
                // TODO: THIS IS ONLY CORRECT IF WE HAVEN'T OVERSHOT THE CENTER
                _linear = -_linear;
                if (_onHitFn) {
                    _onHitFn();
                }
            }
            

            // HitSphere hitSphere;
            // hitSphere._center = newPos;
            // hitSphere._radius = _radius;
            // std::vector<HitSphere> hits;
            // _hitMgr->HitAndStore(hitSphere, &hits);
            // if (!hits.empty()) {
            //     HitSphere const& other = hits.front();
            //     _timeSinceLastHit = 0.f;
            //     glm::vec3 dirAwayFromHit = glm::normalize(_transform->GetPos() - other._center);
            //     newPos = other._center + dirAwayFromHit * (_radius + other._radius + 0.1f);
            //     // _linear = -_linear;
            //     _linear = glm::vec3(0.f);
            // }
        }

        _transform->SetPos(newPos);        
    }

    TransformComponent* _transform = nullptr;
    glm::vec3 _linear;
    float _radius = 0.f;
    HitManager* _hitMgr = nullptr;
    float _timeSinceLastHit = 100.f;
    std::function<void()> _onHitFn;
    
};

class PlayerControllerComponent : public Component {
public:
    PlayerControllerComponent(TransformComponent* t, VelocityPhysicsComponent* v, InputManager const* input, HitManager* hit) 
        : _transform(t)
        , _velocity(v)
        , _input(input)
        , _hitMgr(hit) {}
        
    virtual ~PlayerControllerComponent() {}

    virtual void Update(float dt) override {
        bool evalStateMachine = true;
        State prevState = _state;
        while (evalStateMachine) {
            bool newState = _state != prevState;
            prevState = _state;
            switch (_state) {
                case State::Idle:
                    evalStateMachine = UpdateIdleState(dt, newState);
                    break;
                case State::Attacking:
                    evalStateMachine = UpdateAttackState(dt, newState);
                    break;
            }
        }
    }

    // Returns true if we need to re-evaluate state machine
    bool UpdateIdleState(float dt, bool newState) {
        // Check for attack transition
        if (_input->IsKeyPressedThisFrame(InputManager::Key::J) &&
            glm::length2(_velocity->_linear) > 0.1f*0.1f) {
            _state = State::Attacking;
            return true;
        }

        glm::vec3 inputVec(0.0f);
        if (_input->IsKeyPressed(InputManager::Key::W)) {
            inputVec.z -= 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::S)) {
            inputVec.z += 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::A)) {
            inputVec.x -= 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::D)) {
            inputVec.x += 1.0f;
        }

        if (inputVec.x == 0.f && inputVec.y == 0.f && inputVec.z == 0.f) {
            _velocity->_linear = glm::vec3(0.f);
            return false;
        }

        _velocity->_linear = glm::normalize(inputVec) * kIdleSpeed;
        
        return false;
    }

    bool UpdateAttackState(float dt, bool newState) {
        // triple the speed for a brief time, then decelerate.
        if (newState) {
            _stateTimer = 0.f;
            _attackDir = glm::normalize(_velocity->_linear);
            _velocity->_linear = _attackDir * kAttackSpeed;
        }

        {
            float decelAmount = 175.f;
            glm::vec3 decel = -_attackDir * decelAmount;
            glm::vec3 newVel = _velocity->_linear + dt * decel;
            if (glm::dot(newVel, _attackDir) <= 0.5*kIdleSpeed) {
                // _velocity->_linear = glm::vec3(0.f);    
                _state = State::Idle;            
                return true;
            } else {
                _velocity->_linear = newVel;
            }
        }

        _stateTimer += dt;
        _timeSinceLastHit += dt;
        return false;
    }

    void OnHit() {
        _attackDir = -_attackDir;
        float minSpeed = 30.f;
        if (glm::length2(_velocity->_linear) < minSpeed * minSpeed) {
            _velocity->_linear = _attackDir * minSpeed;
        }
    }

    enum class State {
        Idle, Attacking
    };

    static inline float const kIdleSpeed = 5.f;
    static inline float const kAttackSpeed = 40.f;
    
    TransformComponent* _transform = nullptr;
    VelocityPhysicsComponent* _velocity = nullptr;
    InputManager const* _input = nullptr;
    HitManager* _hitMgr = nullptr;
    State _state = State::Idle;
    float _stateTimer = 0.f;
    glm::vec3 _attackDir;
    float _timeSinceLastHit = 100.f;
    // storing here to reduce allocations
    std::vector<HitSphere> _hits;
};