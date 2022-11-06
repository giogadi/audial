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
#include "renderer.h"

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
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    return beatTime >= _activeBeatTime && beatTime < _inactiveBeatTime;
}

static float constexpr gSpawnYOffset = -10.f;
static double constexpr gSpawnPredelayBeatTime = 0.5;

void EnemyEntity::Init(GameManager& g) {
    ne::Entity::Init(g);
    _desiredSpawnY = _transform._m13;
    _modelColor = Vec4(0.3f, 0.3f, 0.3f, 1.f);
}

float SmoothStep(float x) {
    if (x <= 0) { return 0.f; }
    if (x >= 1.f) { return 1.f; }
    float x2 = x*x;
    return 3*x2 - 2*x2*x;
}

// Maps [0..1] to [0..1..0]
float Triangle(float x) {
    return -std::abs(2.f*(x - 0.5f)) + 1.f;
}

float SmoothUpAndDown(float x) {
    if (x < 0.5f) {
        return SmoothStep(2*x);
    }
    return 1.f-SmoothStep(2*(x - 0.5f));
}

void EnemyEntity::OnShot(GameManager& g) {
    SendEvents(g);
    _shotBeatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (!g._editMode) {
        g._neEntityManager->TagForDestroy(_id);
    }
}

void EnemyEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        ne::Entity::Update(g, dt);
        return;
    }
    
    // If beatTime < activeBeatTime - predelay, don't draw this entity, we done.
    // for beatTime in [activeBeatTime - predelay, activeBeatTime], animate from spawn position to desired position.
    double beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (beatTime < _activeBeatTime - gSpawnPredelayBeatTime ||
        beatTime >= _inactiveBeatTime) {
        return;
    }
    if (beatTime < _activeBeatTime) {
        float param = (_activeBeatTime - beatTime) / gSpawnPredelayBeatTime;  // 0: active beat time; 1: predelay start
        _transform._m13 = _desiredSpawnY + param * gSpawnYOffset;
    } else if (beatTime > _inactiveBeatTime - gSpawnPredelayBeatTime) {
        float param = (_inactiveBeatTime - beatTime) / gSpawnPredelayBeatTime;  // 0: inactive beat time; 1: predelay start
        _transform._m13 = _desiredSpawnY + (1.0-param) * gSpawnYOffset;
        _modelColor = Vec4(0.3f, 0.3f, 0.3f, 1.f);
    } else {
        _modelColor = Vec4(1.f, 0.647f, 0.f, 1.f);  // orange
        _transform._m13 = _desiredSpawnY;

        // float constexpr moveSpeed = 2.f;
        // _transform._m23 += moveSpeed * dt;
    }    

    Mat4 renderTrans = _transform;

    if (_shotBeatTime >= 0.0) {
        double timeSinceShot = beatTime - _shotBeatTime;
        float constexpr bounceBeatTime = 0.3f;
        float param = timeSinceShot / bounceBeatTime;
        // param: [0,1]. Want to map this to [0...1...0]
        param = Triangle(param);
        // For some reason, smoothing doesn't look good.
        // param = SmoothStep(param);
        renderTrans._m23 += -param * 1.5f;
        if (timeSinceShot >= bounceBeatTime) {
            _shotBeatTime = -1.0;
        }
    }

    // Want to hit 2pi every 4 beats.
    double beatFrac = fmod(beatTime, 4.0);
    float angle = (beatFrac / 4.0) * 2 * kPi;
    Mat3 rot = Mat3::FromAxisAngle(Vec3(0.f, 1.f, 0.f), angle);
    renderTrans.SetTopLeftMat3(rot);
    
    if (_model != nullptr) {
        g._scene->DrawMesh(_model, renderTrans, _modelColor);
    }
}