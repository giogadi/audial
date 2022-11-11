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
#include "math_util.h"

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
    SendEvents(g, _events);
}

void EnemyEntity::SendEvents(
    GameManager& g, std::vector<BeatTimeEvent> const& events) {
    for (BeatTimeEvent const& b_e : events) {
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, /*slack=*/0.0);
        g._audioContext->AddEvent(e);
    }    
}

bool EnemyEntity::IsActive(GameManager& g) const {
    if (g._editMode) {
        return true;
    }
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

int EnemyEntity::GetLaneIxFromCurrentPos() {
    float const minX = -kLaneWidth * (kNumLanes / 2);
    float const xOffset = _transform._m03 - minX;
    int laneIx = math_util::Clamp((int)(xOffset / kLaneWidth), 0, kNumLanes - 1);
    return laneIx;
}

void EnemyEntity::SendEventsFromLaneNoteTable(GameManager& g) {
    int const laneIx = GetLaneIxFromCurrentPos();
    
    assert(_laneNotesTable != nullptr);
    assert(laneIx < kNumLanes);
    assert(laneIx >= 0);
    auto const& notes = (*_laneNotesTable)[laneIx];
    for (int noteIx = 0, n = notes.size(); noteIx < n; ++noteIx) {
        int midiNote = notes[noteIx];
        if (midiNote < 0) { continue; }
        BeatTimeEvent b_e;
        b_e._e.type = audio::EventType::NoteOn;
        b_e._e.channel = _channel;
        b_e._e.midiNote = midiNote;
        b_e._beatTime = 0.0;
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, /*slack=*/0.0);
        g._audioContext->AddEvent(e);

        b_e._e.type = audio::EventType::NoteOff;
        b_e._beatTime = _noteLength;
        e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, /*slack=*/0.0);
        g._audioContext->AddEvent(e);
    }
}

void EnemyEntity::OnShot(GameManager& g) {
    if (_laneNoteBehavior == LaneNoteBehavior::Table) {
        SendEventsFromLaneNoteTable(g);
    } else {
        SendEvents(g, _events);
    }    
    _shotBeatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (!g._editMode) {
        if (_hp > 0) {
            --_hp;
            if (_hp <= 0) {
                switch (_onHitBehavior) {
                    case OnHitBehavior::Default: {
                        g._neEntityManager->TagForDestroy(_id);
                        break;
                    }
                    case OnHitBehavior::MultiPhase: {
                        --_numHpBars;
                        if (_numHpBars > 0) {
                            _hp = 3;  // HACK OMG HACK
                            // SendEvents(g, _phaseEvents);
                        } else {
                            g._neEntityManager->TagForDestroy(_id);
                            // SendEvents(g, _deathEvents);
                        }
                        break;
                    }
                }
            }
        }
    }
}

namespace {
    std::array<int, 22> gMinorScaleNotes = {
        0,   2,  3,  5,  7,  8, 10,
        12, 14, 15, 17, 19, 20, 22,
        24, 26, 27, 29, 31, 32, 34,
        36
    };
}

void EnemyEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        ne::Entity::Update(g, dt);
        return;
    }
    
    // If beatTime < activeBeatTime - predelay, don't draw this entity, we done.
    // for beatTime in [activeBeatTime - predelay, activeBeatTime], animate from spawn position to desired position.
    bool active = false;
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
        active = true; 
    }

    float const minX = -kLaneWidth * (kNumLanes / 2);
    int const currentLaneIx = GetLaneIxFromCurrentPos();

    if (active) {
        switch (_behavior) {
            case Behavior::None:
                break;
            case Behavior::Down: {
                _transform._m23 += _downSpeed * dt;
                break;
            }
            case Behavior::Zigging: {
                _zigPhaseTime += dt;
                double const kPhaseTimeInBeats = 1.0;
                double const phaseTimeInSecs = kPhaseTimeInBeats * 60.0 / g._beatClock->GetBpm();
                if (_zigPhaseTime > phaseTimeInSecs) {
                    _zigPhaseTime -= phaseTimeInSecs;
                    _zigMoving = !_zigMoving;
                    if (_zigMoving) {
                        // pick a new target
                        _zigSource = _transform.GetPos();              
                        int newLaneIx = (currentLaneIx + 5) % kNumLanes;
                        float newX = minX + (0.5f * kLaneWidth) + (newLaneIx * kLaneWidth);
                        float newZ = _zigSource._z + 2.f;
                        _zigTarget.Set(newX, 0.f, newZ);                        
                    }
                }
                if (_zigMoving) {
                    float param = SmoothStep(_zigPhaseTime / phaseTimeInSecs);
                    _transform.SetTranslation(_zigSource + (_zigTarget - _zigSource) * param);
                }
                break;
            }
        }
    }

    // Despawn if below a certain point
    float constexpr kDespawnZ = 7.f;
    if (_transform._m23 >= kDespawnZ) {
        g._neEntityManager->TagForDestroy(_id);
    }

    // Update shot button and get current lane ix
    switch (currentLaneIx) {
        case 0: _shootButton = InputManager::Key::A; break;
        case 1: _shootButton = InputManager::Key::S; break;
        case 2: _shootButton = InputManager::Key::D; break;
        case 3: _shootButton = InputManager::Key::F; break;
        case 4: _shootButton = InputManager::Key::H; break;
        case 5: _shootButton = InputManager::Key::J; break;
        case 6: _shootButton = InputManager::Key::K; break;
        case 7: _shootButton = InputManager::Key::L; break;
        default: assert(false); break;
    }

    // update shoot button and midi note from lateral position
    if (_laneNoteBehavior == LaneNoteBehavior::Minor) {                
        assert(_events.size() == 2 * _laneNoteOffsets.size());
        for (int ii = 0; ii < _laneNoteOffsets.size(); ++ii) {
            int const laneNoteOffset = _laneNoteOffsets[ii];
            int minorScaleIndex = currentLaneIx + laneNoteOffset;
            assert(minorScaleIndex >= 0);
            assert(minorScaleIndex < gMinorScaleNotes.size());
            int midiNote = _laneRootNote + gMinorScaleNotes[minorScaleIndex];
            assert(_events[2*ii]._e.type == audio::EventType::NoteOn);
            assert(_events[2*ii+1]._e.type == audio::EventType::NoteOff);
            _events[2*ii]._e.midiNote = midiNote;
            _events[2*ii+1]._e.midiNote = midiNote;
        }
    }

    // This is all done in the OnShot() function
    // if (_laneNoteBehavior == LaneNoteBehavior::Table) {}

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
