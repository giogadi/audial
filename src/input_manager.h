#pragma once

#include <array>
#include <iostream>

#include <GLFW/glfw3.h>

class InputManager {
public:
    enum class Key : int {
        W,S,A,D,E,Q,J,Escape,Space,Right,NumKeys
    };

    InputManager(GLFWwindow* window)
        : _window(window) {
        _keyStates.fill(false);
        _newStates.fill(false);
    }

    void Update() {
        for (int i = 0; i < (int)Key::NumKeys; ++i) {
            Key k = (Key) i;
            bool pressed = glfwGetKey(_window, MapToGlfw(k));
            _newStates[i] = (pressed != _keyStates[i]);
            _keyStates[i] = pressed;
        }
    }

    bool IsKeyPressed(Key k) const {
        return _keyStates[(int)k];
    }
    bool IsKeyPressedThisFrame(Key k) const {
        return _keyStates[(int)k] && _newStates[(int)k];
    }
    bool IsKeyReleasedThisFrame(Key k) const {
        return !_keyStates[(int)k] && _newStates[(int)k];
    }

private:
    int MapToGlfw(Key k) {
        switch (k) {
            case Key::W: return GLFW_KEY_W;
            case Key::S: return GLFW_KEY_S;
            case Key::A: return GLFW_KEY_A;
            case Key::D: return GLFW_KEY_D;
            case Key::E: return GLFW_KEY_E;
            case Key::Q: return GLFW_KEY_Q;
            case Key::J: return GLFW_KEY_J;
            case Key::Escape: return GLFW_KEY_ESCAPE;
            case Key::Space: return GLFW_KEY_SPACE;
            case Key::Right: return GLFW_KEY_RIGHT;
            default: {                    
                std::cout << "InputManager: UNRECOGNIZED KEY!" << std::endl;
                return -1;
            }
        }
    }

    std::array<bool,static_cast<int>(Key::NumKeys)> _keyStates;
    std::array<bool,static_cast<int>(Key::NumKeys)> _newStates;
    GLFWwindow* _window = nullptr;
};