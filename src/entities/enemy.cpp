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
#include "rng.h"

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
}

void EnemyEntity::LoadDerived(serial::Ptree pt) {
    std::string keyStr;
    pt.TryGetString("shoot_button", &keyStr);
    _shootButton = GetKeyFromString(keyStr.c_str());
    pt.TryGetDouble("active_beat_time", &_activeBeatTime);
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
    // if (ImGui::Button("Apply Script to Entity")) {
    //     ReadBeatEventsFromScript(_events, *g._soundBank, gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    // }
    // if (ImGui::Button("Apply Entity to Script")) {
    //     WriteBeatEventsToScript(_events, *g._soundBank, gAudioEventScriptBuf.data(), gAudioEventScriptBuf.size());
    // }
    return ImGuiResult::Done;
}

void EnemyEntity::OnEditPick(GameManager& g) {
    if (_laneNoteBehavior == LaneNoteBehavior::Table) {
        assert(_laneNotesTable != nullptr);
        SendEventsFromLaneNoteTable(g, *_laneNotesTable);
    } else if (_laneNoteBehavior == LaneNoteBehavior::Events) {
        SendEventsFromEventsList(g);
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
    _hp = _initialHp;
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

static float GetCenterXOfLane(int laneIx) {
    float constexpr minX = -kLaneWidth * (kNumLanes / 2);
    float centerX = minX + (0.5f * kLaneWidth) + (laneIx * kLaneWidth);
    return centerX;
}

int EnemyEntity::GetLaneIxFromCurrentPos() {
    float constexpr minX = -kLaneWidth * (kNumLanes / 2);
    float const xOffset = _transform._m03 - minX;
    int laneIx = math_util::Clamp((int)(xOffset / kLaneWidth), 0, kNumLanes - 1);
    return laneIx;
}

static double constexpr kSlack = 0.0625*2;

void EnemyEntity::SendEventsFromLaneNoteTable(
    GameManager& g, LaneNotesTable const& laneNotesTable) {
    int const laneIx = GetLaneIxFromCurrentPos();
    
    assert(laneIx < kNumLanes);
    assert(laneIx >= 0);
    auto const& notes = laneNotesTable._table[laneIx];
    for (int noteIx = 0, n = notes.size(); noteIx < n; ++noteIx) {
        int midiNote = notes[noteIx];
        if (midiNote < 0) { continue; }
        BeatTimeEvent b_e;
        b_e._e.type = audio::EventType::NoteOn;
        b_e._e.channel = laneNotesTable._channel;
        b_e._e.midiNote = midiNote;
        b_e._beatTime = 0.0;
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, kSlack);
        g._audioContext->AddEvent(e);

        b_e._e.type = audio::EventType::NoteOff;
        b_e._beatTime = laneNotesTable._noteLength;
        e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, kSlack);
        g._audioContext->AddEvent(e);
    }
}

void EnemyEntity::SendEventsFromEventsList(GameManager& g) {    
    for (BeatTimeEvent const& b_e : _events) {
        audio::Event e = GetEventAtBeatOffsetFromNextDenom(_eventStartDenom, b_e, *g._beatClock, kSlack);
        g._audioContext->AddEvent(e);
    }
}

void EnemyEntity::OnShot(GameManager& g) {
    if (_laneNoteBehavior == LaneNoteBehavior::Table) {
        assert(_laneNotesTable != nullptr);
        SendEventsFromLaneNoteTable(g, *_laneNotesTable);   
    } else if (_laneNoteBehavior == LaneNoteBehavior::Events) {
        SendEventsFromEventsList(g);
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
                            _hp = _initialHp;
                            if (_phaseNotesTable) {
                                SendEventsFromLaneNoteTable(g, *_phaseNotesTable);
                                if (_behavior == Behavior::MoveOnPhase) {
                                    _motionSource = _transform.GetPos();
                                    int currentLaneIx = GetLaneIxFromCurrentPos();
                                    //currentLaneIx  = (currentLaneIx + 4) % kNumLanes;
                                    currentLaneIx = rng::GetInt(0, kNumLanes-1);
                                    float newX = GetCenterXOfLane(currentLaneIx);
                                    float newZ = _motionSource._z + 1.f;
                                    _motionTarget.Set(newX, 0.f, newZ);
                                    _motionStartBeatTime = g._beatClock->GetBeatTimeFromEpoch();
                                    _motionEndBeatTime = g._beatClock->GetNextDownBeatTime(_motionStartBeatTime) + 2.0;
                                }
                            } else {
                                printf("WARNING!! No phase note table!\n");
                            }
                        } else {
                            g._neEntityManager->TagForDestroy(_id);
                            if (_deathNotesTable) {
                                SendEventsFromLaneNoteTable(g, *_deathNotesTable);
                            } else {
                                printf("WARNING!! No death note table!\n");
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void EnemyEntity::Update(GameManager& g, float dt) {
    if (g._editMode) {
        ne::Entity::Update(g, dt);
        return;
    }
    
    // If beatTime < activeBeatTime - predelay, don't draw this entity, we done.
    // for beatTime in [activeBeatTime - predelay, activeBeatTime], animate from spawn position to desired position.
    bool active = false;
    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    if (beatTime < _activeBeatTime - gSpawnPredelayBeatTime ||
        beatTime >= _inactiveBeatTime) {
        return;
    }
    bool useModelColor = false;
    if (beatTime < _activeBeatTime) {
        float param = (_activeBeatTime - beatTime) / gSpawnPredelayBeatTime;  // 0: active beat time; 1: predelay start
        _transform._m13 = _desiredSpawnY + param * gSpawnYOffset;
        useModelColor = false;
    } else if (beatTime > _inactiveBeatTime - gSpawnPredelayBeatTime) {
        float param = (_inactiveBeatTime - beatTime) / gSpawnPredelayBeatTime;  // 0: inactive beat time; 1: predelay start
        _transform._m13 = _desiredSpawnY + (1.0-param) * gSpawnYOffset;
        useModelColor = false;
    } else {        
        useModelColor = true;
        _transform._m13 = _desiredSpawnY;
        active = true; 
    }

    float const minX = -kLaneWidth * (kNumLanes / 2);
    int const currentLaneIx = GetLaneIxFromCurrentPos();

    if (active) {
        switch (_behavior) {
            case Behavior::None:
                break;
            case Behavior::ConstVel: {
                _transform._m03 += _constVelX * dt;
                _transform._m23 += _constVelZ * dt;
                break;
            }
            case Behavior::Zigging: {
                if (_motionStartBeatTime >= 0.0) {
                    
                } else {
                    double constexpr kWaitBeatTime = 1.0;
                    double constexpr kMoveBeatTime = 1.0;
                    if (_motionEndBeatTime < 0.0) {
                        _motionEndBeatTime = g._beatClock->GetNextBeatDenomTime(beatTime, 4.0) - kWaitBeatTime;
                        _motionEndBeatTime = std::max(_motionEndBeatTime, 0.0);
                    }
                    double waitEndTime = _motionEndBeatTime + kWaitBeatTime;
                    if (beatTime >= waitEndTime) {
                        _motionStartBeatTime = waitEndTime;
                        _motionEndBeatTime = _motionStartBeatTime + kMoveBeatTime;
                        _motionSource = _transform.GetPos();
                        int newLaneIx = (currentLaneIx + 5) % kNumLanes;
                        float newX = minX + (0.5f * kLaneWidth) + (newLaneIx * kLaneWidth);
                        float newZ = _motionSource._z + 2.f;
                        _motionTarget.Set(newX, 0.f, newZ);
                    }
                }
                break;
            }
            case Behavior::MoveOnPhase: {
                // handled during on-shot?
            }
        }
    }

    // Beat motion
    if (_motionStartBeatTime >= 0.0) {
        double timeSinceStart = beatTime - _motionStartBeatTime;
        assert(timeSinceStart >= 0.0);
        double totalMotionTime = _motionEndBeatTime - _motionStartBeatTime;
        assert(totalMotionTime >= 0.0);
        double param = timeSinceStart / totalMotionTime;
        param = std::min(param, 1.0);
        param = SmoothStep(param);
        Vec3 newPos = _motionSource + (_motionTarget - _motionSource) * param;
        _transform.SetTranslation(newPos);
        if (beatTime >= _motionEndBeatTime) {
            _motionStartBeatTime = -1.0;
        }
    }

    // Despawn if beyond screen bounds
    float constexpr kDespawnMinZ = -13.f;
    float constexpr kDespawnMaxZ = 7.f;
    float constexpr kDespawnMaxX = 10.f;
    if (_transform._m23 > kDespawnMaxZ ||
        _transform._m23 < kDespawnMinZ ||
        std::abs(_transform._m03) > kDespawnMaxX) {
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
        if (useModelColor) {
            g._scene->DrawMesh(_model, renderTrans, _modelColor);
        } else {
            g._scene->DrawMesh(_model, renderTrans, Vec4(0.3f, 0.3f, 0.3f, 1.f));
        }
    }
}
