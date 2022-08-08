#pragma once

#include <array>
#include <iostream>

#include <GLFW/glfw3.h>

class InputManager {
public:
    enum class Key : int {
        W,S,A,D,E,Q,J,Y,H,K,Escape,Space,Right,NumKeys
    };
    enum class MouseButton : int {
        Left,Right,Middle,Count
    };

    InputManager(GLFWwindow* window)
        : _window(window) {
        _keyStates.fill(false);
        _keyNewStates.fill(false);
        _mouseButtonStates.fill(false);
        _mouseButtonNewStates.fill(false);
    }

    void Update() {
        for (int i = 0; i < (int)Key::NumKeys; ++i) {
            Key k = (Key) i;
            bool pressed = glfwGetKey(_window, MapToGlfw(k));
            _keyNewStates[i] = (pressed != _keyStates[i]);
            _keyStates[i] = pressed;
        }
        for (int i = 0; i < (int)MouseButton::Count; ++i) {
            MouseButton k = (MouseButton) i;
            bool pressed = glfwGetMouseButton(_window, MapToGlfw(k));
            _mouseButtonNewStates[i] = (pressed != _mouseButtonStates[i]);
            _mouseButtonStates[i] = pressed;
        }
        glfwGetCursorPos(_window, &_mouseX, &_mouseY);
    }

    bool IsKeyPressed(Key k) const {
        return _keyStates[(int)k];
    }
    bool IsKeyPressedThisFrame(Key k) const {
        return _keyStates[(int)k] && _keyNewStates[(int)k];
    }
    bool IsKeyReleasedThisFrame(Key k) const {
        return !_keyStates[(int)k] && _keyNewStates[(int)k];
    }

    bool IsKeyPressed(MouseButton k) const {
        return _mouseButtonStates[(int)k];
    }
    bool IsKeyPressedThisFrame(MouseButton k) const {
        return _mouseButtonStates[(int)k] && _mouseButtonNewStates[(int)k];
    }
    bool IsKeyReleasedThisFrame(MouseButton k) const {
        return !_mouseButtonStates[(int)k] && _mouseButtonNewStates[(int)k];
    }

    void GetMousePos(double& mouseX, double& mouseY) const {
        mouseX = _mouseX;
        mouseY = _mouseY;
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
            case Key::Y: return GLFW_KEY_Y;
            case Key::H: return GLFW_KEY_H;
            case Key::K: return GLFW_KEY_K;
            case Key::Escape: return GLFW_KEY_ESCAPE;
            case Key::Space: return GLFW_KEY_SPACE;
            case Key::Right: return GLFW_KEY_RIGHT;
            default: {                    
                std::cout << "InputManager: UNRECOGNIZED KEY!" << std::endl;
                return -1;
            }
        }
    }
    int MapToGlfw(MouseButton b) {
        switch (b) {
            case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
            case MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
            case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
            default: {
                std::cout << "InputManager: UNRECOGNIZED MOUSE BUTTON!" << std::endl;
                return -1;
            }
        }
    }

    std::array<bool,static_cast<int>(Key::NumKeys)> _keyStates;
    std::array<bool,static_cast<int>(Key::NumKeys)> _keyNewStates;
    std::array<bool,static_cast<int>(MouseButton::Count)> _mouseButtonStates;
    std::array<bool,static_cast<int>(MouseButton::Count)> _mouseButtonNewStates;
    double _mouseX = 0.0;
    double _mouseY = 0.0;
    GLFWwindow* _window = nullptr;
};