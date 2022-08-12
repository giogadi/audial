#include "input_manager.h"

#include <iostream>

#include <GLFW/glfw3.h>

InputManager* InputManager::_gInputManager = nullptr;

// static
void InputManager::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    assert(_gInputManager != nullptr);
    _gInputManager->_haveScrollInputThisFrame = true;
    _gInputManager->_mouseScrollX = xOffset;
    _gInputManager->_mouseScrollY = yOffset;
}

InputManager::InputManager(GLFWwindow* window)
    : _window(window) {
    assert(_gInputManager == nullptr);
    _gInputManager = this;

    _keyStates.fill(false);
    _keyNewStates.fill(false);
    _mouseButtonStates.fill(false);
    _mouseButtonNewStates.fill(false);

    glfwSetScrollCallback(window, &InputManager::ScrollCallback);
}

void InputManager::Update() {
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

    if (!_haveScrollInputThisFrame) {
        _mouseScrollX = 0.0;
        _mouseScrollY = 0.0;
    }
    _haveScrollInputThisFrame = false;
}

int InputManager::MapToGlfw(Key k) {
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
int InputManager::MapToGlfw(MouseButton b) {
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