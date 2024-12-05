#pragma once

#include <array>

struct GLFWwindow;

class InputManager {
public:
    enum class Key : int {
        A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
        Escape,Space,Up,Down,Left,Right,LeftShift,RightShift,LeftAlt,RightAlt,LeftCtrl,RightCtrl,Tab,Backspace,Delete,
        NUM_0,NUM_1,NUM_2,NUM_3,NUM_4,NUM_5,NUM_6,NUM_7,NUM_8,NUM_9,
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

    void Update(bool enabled, float dt);

    static Key CharToKey(char c);

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
    float GetKeyDownTime(MouseButton k) const {
        return _mouseButtonDownTimes[(int)k];
    }
    void GetMouseClickPos(double &x, double &y) const {
        x = _mouseClickX;
        y = _mouseClickY;
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
    bool IsCtrlPressed() const {
        return IsKeyPressed(Key::LeftCtrl) || IsKeyPressed(Key::RightCtrl);
    }

    // returns -1 if no numbers pressed. returns the least number if multiple are pressed
    int GetNumPressedThisFrame() const;

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
    std::array<float, static_cast<int>(MouseButton::Count)> _mouseButtonDownTimes;
    std::array<bool,static_cast<int>(ControllerButton::Count)> _controllerButtonStates;
    std::array<bool,static_cast<int>(ControllerButton::Count)> _controllerButtonNewStates;
    int _controllerId = -1;
    double _mouseX = 0.0;
    double _mouseY = 0.0;
    double _mouseClickX = 0.0;
    double _mouseClickY = 0.0;
    double _mouseMotionX = 0.0;
    double _mouseMotionY = 0.0;
    bool _haveScrollInputThisFrame = false;
    double _mouseScrollX = 0.0;
    double _mouseScrollY = 0.0;
    GLFWwindow* _window = nullptr;
    bool _usingController = false;
};
