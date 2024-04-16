#pragma once

#include "serial.h"
#include "transform.h"
#include "game_manager.h"
#include "imgui_str.h"

struct MechButton {
    imgui_util::Char key;
    Vec3 offset;
    void Save(serial::Ptree pt) const;
    void Load(serial::Ptree pt);
    bool ImGui();
    void Draw(GameManager& g, Transform const& t, float pushFactor, bool keyPressed);
};

