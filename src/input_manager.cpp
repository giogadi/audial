#include "input_manager.h"

#include <iostream>
#include <cctype>

#include <GLFW/glfw3.h>
#include "imgui/backends/imgui_impl_glfw.h"

InputManager* InputManager::_gInputManager = nullptr;

// static
void InputManager::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    assert(_gInputManager != nullptr);
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
    _gInputManager->_haveScrollInputThisFrame = true;
    _gInputManager->_mouseScrollX = xOffset;
    _gInputManager->_mouseScrollY = yOffset;
}

InputManager::Key InputManager::CharToKey(char c) {
    c = std::tolower(c);
    int charIx = c - 'a';
    if (charIx < 0 || charIx > static_cast<int>(InputManager::Key::Z)) {
        printf("WARNING: char \'%c\' not in InputManager!\n", c);
        return InputManager::Key::NumKeys;
    }
    InputManager::Key key = static_cast<InputManager::Key>(charIx);
    return key;
}

InputManager::InputManager(GLFWwindow* window)
    : _window(window) {
    assert(_gInputManager == nullptr);
    _gInputManager = this;

    _keyStates.fill(false);
    _keyNewStates.fill(false);
    _mouseButtonStates.fill(false);
    _mouseButtonNewStates.fill(false);
    _mouseButtonDownTimes.fill(0.f);
    _controllerButtonStates.fill(false);
    _controllerButtonNewStates.fill(false);

    glfwSetScrollCallback(window, &InputManager::ScrollCallback);

    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; ++i) {
        int result = glfwJoystickIsGamepad(i);
        if (result == GLFW_TRUE) {
            _controllerId = i;
            break;
        }
    }
}

void InputManager::Update(bool enabled, float const dt) {
    bool hasKeyInput = false;
    bool hasControllerInput = false;

    double newMouseX;
    double newMouseY;
    glfwGetCursorPos(_window, &newMouseX, &newMouseY);
    _mouseMotionX = newMouseX - _mouseX;
    _mouseMotionY = newMouseY - _mouseY;
    _mouseX = newMouseX;
    _mouseY = newMouseY;

    for (int i = 0; i < (int)Key::NumKeys; ++i) {
        Key k = (Key) i;
        bool pressed = enabled && glfwGetKey(_window, MapToGlfw(k));
        if (pressed) {
            hasKeyInput = true;
        }
        _keyNewStates[i] = (pressed != _keyStates[i]);
        _keyStates[i] = pressed;
    }
    for (int i = 0; i < (int)MouseButton::Count; ++i) {
        MouseButton k = (MouseButton) i;
        bool pressed = enabled && glfwGetMouseButton(_window, MapToGlfw(k));
        _mouseButtonNewStates[i] = (pressed != _mouseButtonStates[i]);
        if (pressed) {
            if (_mouseButtonNewStates[i]) {
                _mouseButtonDownTimes[i] = 0.f;
                _mouseClickX = _mouseX;
                _mouseClickY = _mouseY;
            } else {
                _mouseButtonDownTimes[i] += dt;
            }
        }
        _mouseButtonStates[i] = pressed;
    }
    GLFWgamepadstate padState{};    
    if (_controllerId >= GLFW_JOYSTICK_1) {
        if (!glfwGetGamepadState(_controllerId, &padState)) {
            printf("Failed to get pad state. maybe disconnected?\n");
            _controllerId = -1;
        }
    }
    float constexpr kTriggerThreshold = 0.f;
    for (int i = 0; i < (int)ControllerButton::Count; ++i) {
        ControllerButton b = (ControllerButton) i;
        bool pressed = false;
        if (_controllerId >= GLFW_JOYSTICK_1) {
            switch (b) {
                case InputManager::ControllerButton::PadUp: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP]; break;
                case InputManager::ControllerButton::PadDown: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]; break;
                case InputManager::ControllerButton::PadLeft: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]; break;
                case InputManager::ControllerButton::PadRight: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]; break;
                case InputManager::ControllerButton::ButtonTop: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE]; break;
                case InputManager::ControllerButton::ButtonBottom: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_CROSS]; break;
                case InputManager::ControllerButton::ButtonLeft: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_SQUARE]; break;
                case InputManager::ControllerButton::ButtonRight: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE]; break;
                case InputManager::ControllerButton::BumperLeft: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER]; break;
                case InputManager::ControllerButton::BumperRight: pressed = padState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]; break;
                case InputManager::ControllerButton::TriggerLeft: pressed = padState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] >= kTriggerThreshold; break;
                case InputManager::ControllerButton::TriggerRight: {
                    pressed = padState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] >= kTriggerThreshold;
                    break;
                }
                default: {
                    printf("InputManager: UNRECOGNIZED CONTROLLER BUTTON %d\n", (int)b);
                }
            }
        }
        if (pressed) {
            hasControllerInput = true;
        }
        _controllerButtonNewStates[i] = (pressed != _controllerButtonStates[i]);
        _controllerButtonStates[i] = pressed;
    }

    if (_usingController && hasKeyInput) {
        _usingController = false;
    } else if (!_usingController && hasControllerInput) {
        _usingController = true;
    }    

    if (!enabled || !_haveScrollInputThisFrame) {
        _mouseScrollX = 0.0;
        _mouseScrollY = 0.0;
    }
    _haveScrollInputThisFrame = false;
}

int InputManager::MapToGlfw(Key k) {
    switch (k) {
        case Key::A: return GLFW_KEY_A;
        case Key::B: return GLFW_KEY_B;
        case Key::C: return GLFW_KEY_C;
        case Key::D: return GLFW_KEY_D;
        case Key::E: return GLFW_KEY_E;
        case Key::F: return GLFW_KEY_F;
        case Key::G: return GLFW_KEY_G;
        case Key::H: return GLFW_KEY_H;
        case Key::I: return GLFW_KEY_I;
        case Key::J: return GLFW_KEY_J;
        case Key::K: return GLFW_KEY_K;
        case Key::L: return GLFW_KEY_L;
        case Key::M: return GLFW_KEY_M;
        case Key::N: return GLFW_KEY_N;
        case Key::O: return GLFW_KEY_O;
        case Key::P: return GLFW_KEY_P;
        case Key::Q: return GLFW_KEY_Q;
        case Key::R: return GLFW_KEY_R;
        case Key::S: return GLFW_KEY_S;
        case Key::T: return GLFW_KEY_T;
        case Key::U: return GLFW_KEY_U;
        case Key::V: return GLFW_KEY_V;
        case Key::W: return GLFW_KEY_W;
        case Key::X: return GLFW_KEY_X;
        case Key::Y: return GLFW_KEY_Y;
        case Key::Z: return GLFW_KEY_Z;
        case Key::Escape: return GLFW_KEY_ESCAPE;
        case Key::Space: return GLFW_KEY_SPACE;
        case Key::Left: return GLFW_KEY_LEFT;
        case Key::Right: return GLFW_KEY_RIGHT;
        case Key::LeftShift: return GLFW_KEY_LEFT_SHIFT;
        case Key::RightShift: return GLFW_KEY_RIGHT_SHIFT;
        case Key::LeftAlt: return GLFW_KEY_LEFT_ALT;
        case Key::RightAlt: return GLFW_KEY_RIGHT_ALT;
        case Key::LeftCtrl: return GLFW_KEY_LEFT_CONTROL;
        case Key::RightCtrl: return GLFW_KEY_RIGHT_CONTROL;
        case Key::Tab: return GLFW_KEY_TAB;
        case Key::Backspace: return GLFW_KEY_BACKSPACE;
        case Key::NUM_0: return GLFW_KEY_0;
        case Key::NUM_1: return GLFW_KEY_1;
        case Key::NUM_2: return GLFW_KEY_2;
        case Key::NUM_3: return GLFW_KEY_3;
        case Key::NUM_4: return GLFW_KEY_4;
        case Key::NUM_5: return GLFW_KEY_5;
        case Key::NUM_6: return GLFW_KEY_6;
        case Key::NUM_7: return GLFW_KEY_7;
        case Key::NUM_8: return GLFW_KEY_8;
        case Key::NUM_9: return GLFW_KEY_9;
        case Key::Enter: return GLFW_KEY_ENTER;
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

int InputManager::GetNumPressedThisFrame() const {
    for (int i = 0; i <= 9; ++i) {
        InputManager::Key k = static_cast<InputManager::Key>(static_cast<int>(InputManager::Key::NUM_0) + i);
        if (IsKeyPressedThisFrame(k)) {
            return i;
        }
    }
    return -1;
}
