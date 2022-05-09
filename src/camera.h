#pragma once

#include "glm/vec3.hpp"

#include "input_manager.h"

class DebugCamera {
public:
    DebugCamera(InputManager const& input)
        : _pos(0.0f)
        , _input(input) {}
    
    glm::vec3 _pos;

    glm::mat4 GetViewMatrix() const {
        glm::vec3 center = _pos + glm::vec3(0.f, 0.f, -1.f);
        return glm::lookAt(_pos, center, /*up=*/glm::vec3(0.f, 1.f, 0.f));
    }

    void Update(float const dt) {
        glm::vec3 inputVec(0.0f);
        if (_input.IsKeyPressed(InputManager::Key::W)) {
            inputVec.z -= 1.0f;
        }
        if (_input.IsKeyPressed(InputManager::Key::S)) {
            inputVec.z += 1.0f;
        }
        if (_input.IsKeyPressed(InputManager::Key::A)) {
            inputVec.x -= 1.0f;
        }
        if (_input.IsKeyPressed(InputManager::Key::D)) {
            inputVec.x += 1.0f;
        }
        if (_input.IsKeyPressed(InputManager::Key::Q)) {
            inputVec.y += 1.0f;
        }
        if (_input.IsKeyPressed(InputManager::Key::E)) {
            inputVec.y -= 1.0f;
        }

        if (inputVec.x == 0.f && inputVec.y == 0.f && inputVec.z == 0.f) {
            return;
        }

        float const kSpeed = 5.0f;
        glm::vec3 translation = dt * kSpeed * glm::normalize(inputVec);
        _pos += translation;
    }

private:
    float _pitchRad = 0.0f;
    float _yawRad = 0.0f;
    InputManager const& _input;
};