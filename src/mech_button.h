#pragma once

#include "serial.h"
#include "transform.h"
#include "game_manager.h"
#include "imgui_str.h"

struct MechButton {
    struct Props {
        imgui_util::Char key;
        Vec3 offset;
    };
    Props _p;

    struct State {
        bool keyPressed;
        bool keyJustPressed;
        bool keyJustReleased;
        float keyPushFactor;
    };
    State _s;
    
    void ResetProps();
    void ResetState();
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();
    void Init();
    void Update(GameManager& g, float dt);
    void Draw(GameManager& g, Transform const& t);
};

