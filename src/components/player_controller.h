#pragma once

#include "component.h"

#include "beep_on_hit.h"

#include "glm/gtc/constants.hpp"

class PlayerControllerComponent : public Component {
public:
    PlayerControllerComponent(TransformComponent* t, VelocityComponent* v, InputManager const* input, HitManager* hit) 
        : _transform(t)
        , _velocity(v)
        , _input(input)
        , _hitMgr(hit) {}
        
    virtual ~PlayerControllerComponent() {}
    virtual void Update(float dt) override {
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
        if (_input->IsKeyPressed(InputManager::Key::Q)) {
            inputVec.y += 1.0f;
        }
        if (_input->IsKeyPressed(InputManager::Key::E)) {
            inputVec.y -= 1.0f;
        }

        float const kSpeed = 5.0f;
        _velocity->_linear = inputVec * kSpeed;

        // Spin attack!
        if (_input->IsKeyPressed(InputManager::Key::J)) {
            _velocity->_angularY = 2.f * glm::pi<float>() * 2.f;
            Hitbox hitbox;
            float extent = glm::one_over_root_two<float>();
            hitbox._min = _transform->GetPos() - glm::vec3(extent);
            hitbox._max = _transform->GetPos() + glm::vec3(extent);
            _hitMgr->Hit(hitbox);
        } else if (std::abs(_velocity->_angularY) > 0.f) {
            _velocity->_angularY = 0.f;
        }
    }
    
    TransformComponent* _transform = nullptr;
    VelocityComponent* _velocity = nullptr;
    InputManager const* _input = nullptr;
    HitManager* _hitMgr = nullptr;
};