#include "entities/enemy.h"

#include <array>
#include <sstream>
#include <cctype>

#include "constants.h"
#include "imgui/imgui.h"
#include "game_manager.h"
#include "sound_bank.h"
#include "serial.h"
#include "audio.h"

namespace {

std::array<char,1024> gAudioEventScriptBuf;
std::array<char, 8> gButtonNameText;

InputManager::Key GetKeyFromString(char const* keyName) {
    if (keyName[0] == '\0') {
        return InputManager::Key::NumKeys;
    }
    if (keyName[1] == '\0') {
        char c = tolower(keyName[0]);
        int keyIx = c - 'a';
        if (keyIx < 0 || keyIx > 25) {
            return InputManager::Key::NumKeys;
        }
        return (InputManager::Key) keyIx;
    }
    // TODO handle space/etc
    return InputManager::Key::NumKeys;
}

// Assume outStr can hold at least 7 chars (not counting null terminator)
void GetStringFromKey(InputManager::Key key, char* outStr) {
    int keyIx = (int)key;
    if (keyIx < 26) {
        outStr[0] = 'a' + keyIx;
        outStr[1] = '\0';
    } else {
        // TODO not supported
    }
}

} // end namespace

// TODO: consider using the text format we made above for this.
void EnemyEntity::SaveDerived(serial::Ptree pt) const {
    char keyStr[8];
    GetStringFromKey(_shootButton, keyStr);
    pt.PutString("shoot_button", keyStr);
    pt.PutDouble("active_beat_time", _activeBeatTime);
    pt.PutFloat("motion_angle_degrees", _motionAngleDegrees);
    pt.PutFloat("motion_speed", _motionSpeed);
    serial::Ptree eventsPt = pt.AddChild("events");
    for (BeatTimeEvent const& b_e : _events) {
        serial::Ptree eventPt = eventsPt.AddChild("beat_event");
        b_e.Save(eventPt);
    }
}

void EnemyEntity::LoadDerived(serial::Ptree pt) {
    std::string keyStr;
    pt.TryGetString("shoot_button", &keyStr);
    _shootButton = GetKeyFromString(keyStr.c_str());
    pt.TryGetDouble("active_beat_time", &_activeBeatTime);
    pt.TryGetFloat("motion_angle_degrees", &_motionAngleDegrees);
    pt.TryGetFloat("motion_speed", &_motionSpeed);    
    serial::Ptree eventsPt = pt.TryGetChild("events");
    if (eventsPt.IsValid()) {
        int numChildren = 0;
        serial::NameTreePair* children = eventsPt.GetChildren(&numChildren);
        for (int i = 0; i < numChildren; ++i) {
            _events.emplace_back();
            _events.back().Load(children[i]._pt);
        }
        delete[] children;
    }    
}

ne::Entity::ImGuiResult EnemyEntity::ImGuiDerived(GameManager& g) {
    GetStringFromKey(_shootButton, gButtonNameText.data());
    bool changed = ImGui::InputText("Shoot button", gButtonNameText.data(), gButtonNameText.size());
    if (changed) {
        _shootButton = GetKeyFromString(gButtonNameText.data());
    }
    ImGui::InputDouble("Active beat time", &_activeBeatTime);
    ImGui::InputDouble("Event start denom", &_eventStartDenom);
    ImGui::InputFloat("Motion angle (deg)", &_motionAngleDegrees);
    ImGui::InputFloat("Motion speed", &_motionSpeed);
    ImGui::InputTextMultiline("Audio events", gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    if (ImGui::Button("Apply Script to Entity")) {
        ReadBeatEventsFromScript(_events, *g._soundBank, gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    }
    if (ImGui::Button("Apply Entity to Script")) {
        WriteBeatEventsToScript(_events, *g._soundBank, gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    }
    return ImGuiResult::Done;
}

void EnemyEntity::OnEditPick(GameManager& g) {
    SendEvents(g);
}

void EnemyEntity::SendEvents(GameManager& g) {
    for (BeatTimeEvent const& b_e : _events) {
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, /*slack=*/0.0);
        g._audioContext->AddEvent(e);
    }    
}

bool EnemyEntity::IsActive(GameManager& g) const {
    return g._beatClock->GetBeatTime() >= _activeBeatTimeAbsolute;
}

void EnemyEntity::OnShot(GameManager& g) {
    SendEvents(g);
    if (!g._editMode) {
        g._neEntityManager->TagForDestroy(_id);
    }    
}

void EnemyEntity::Init(GameManager& g) {
    ne::Entity::Init(g);
    _activeBeatTimeAbsolute = g._beatClock->GetDownBeatTime() + _activeBeatTime;
}

void EnemyEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        ne::Entity::Update(g, dt);
        return;
    }
    if (!IsActive(g)) {
        return;
    }
    float angleRad = _motionAngleDegrees * kDeg2Rad;
    Vec3 velocity(cos(angleRad), 0.f, -sin(angleRad));
    velocity *= _motionSpeed;

    Vec3 p = _transform.GetPos();
    p += velocity * dt;
    _transform.SetTranslation(p);
    ne::Entity::Update(g, dt);  // draw mesh    
}