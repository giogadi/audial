#pragma once

#include <array>

struct GLFWwindow;

class InputManager {
public:
    enum class Key : int {
        A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
        Escape,Space,Left,Right,LeftShift,RightShift,LeftAlt,RightAlt,Tab,Backspace,
        Enter,
        NumKeys
    };
    enum class MouseButton : int {
        Left,Right,Middle,Count
    };
    enum class ControllerButton : int {
        ButtonTop,ButtonBottom,ButtonLeft,ButtonRight,
        PadUp,PadDown,PadLeft,PadRight,
        BumperLeft,BumperRight,
        TriggerLeft,TriggerRight,
        Count
    };

    InputManager(GLFWwindow* window);

    void Update(bool enabled);

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

    bool IsKeyPressed(ControllerButton k) const {
        return _controllerButtonStates[(int)k];
    }
    bool IsKeyPressedThisFrame(ControllerButton k) const {
        return _controllerButtonStates[(int)k] && _controllerButtonNewStates[(int)k];
    }
    bool IsKeyReleasedThisFrame(ControllerButton k) const {
        return !_controllerButtonStates[(int)k] && _controllerButtonNewStates[(int)k];
    }

    bool IsShiftPressed() const {
        return IsKeyPressed(Key::LeftShift) || IsKeyPressed(Key::RightShift);
    }
    bool IsAltPressed() const {
        return IsKeyPressed(Key::LeftAlt) || IsKeyPressed(Key::RightAlt);
    }

    // SCREEN COORDINATES, (0,0) is top-left corner of window
    void GetMousePos(double& mouseX, double& mouseY) const {
        mouseX = _mouseX;
        mouseY = _mouseY;
    }

    void GetMouseMotion(double& motionX, double& motionY) const {
        motionX = _mouseMotionX;
        motionY = _mouseMotionY;
    }

    // Units: window pixels (with potential subpixel accuracy)
    // +x: touchpad scroll with fingers moving RIGHT
    // +y: touchpad scroll with fingers moving DOWN
    void GetMouseScroll(double& scrollX, double& scrollY) const {
        scrollX = _mouseScrollX;
        scrollY = _mouseScrollY;
    }

    bool IsUsingController() const {
        return _usingController;
    }

private:
    int MapToGlfw(Key k);
    int MapToGlfw(MouseButton b);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

    static InputManager* _gInputManager;

    std::array<bool,static_cast<int>(Key::NumKeys)> _keyStates;
    std::array<bool,static_cast<int>(Key::NumKeys)> _keyNewStates;
    std::array<bool,static_cast<int>(MouseButton::Count)> _mouseButtonStates;
    std::array<bool,static_cast<int>(MouseButton::Count)> _mouseButtonNewStates;
    std::array<bool,static_cast<int>(ControllerButton::Count)> _controllerButtonStates;
    std::array<bool,static_cast<int>(ControllerButton::Count)> _controllerButtonNewStates;
    int _controllerId = -1;
    double _mouseX = 0.0;
    double _mouseY = 0.0;
    double _mouseMotionX = 0.0;
    double _mouseMotionY = 0.0;
    bool _haveScrollInputThisFrame = false;
    double _mouseScrollX = 0.0;
    double _mouseScrollY = 0.0;
    GLFWwindow* _window = nullptr;
    bool _usingController = false;
};
