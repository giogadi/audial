#include "int_variable.h"

#include <sstream>

#include "seq_action.h"
#include "serial_vector_util.h"
#include "imgui_vector_util.h"
#include "renderer.h"
#include "math_util.h"
#include "synth_patch_bank.h"
#include "audio.h"

void IntVariableEntity::InitDerived(GameManager& g) {
    _currentValue = _initialValue;
    _beatTimeOfLastAdd = -1.0;

    _startPatch = nullptr;
    _endPatch = nullptr;
    if (!_startPatchName.empty()) {
        _startPatch = g._synthPatchBank->GetPatch(_startPatchName);
    }
    if (!_endPatchName.empty()) {
        _endPatch = g._synthPatchBank->GetPatch(_endPatchName);
    }

    for (auto const& pAction : _actions) {
        pAction->Init(g);
    }

    _g = &g;
}

void IntVariableEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("initial_value", _initialValue);

    pt.PutBool("draw_counter", _drawCounter);
    pt.PutBool("draw_horizontal", _drawHorizontal);
    pt.PutFloat("cell_width", _cellWidth);
    pt.PutFloat("cell_spacing", _cellSpacing);

    pt.PutInt("synth_patch_channel", _synthPatchChannel);
    pt.PutString("start_patch", _startPatchName.c_str());
    pt.PutString("end_patch", _endPatchName.c_str());

    SeqAction::SaveActionsInChildNode(pt, "actions", _actions);
}

void IntVariableEntity::LoadDerived(serial::Ptree pt) {
    _initialValue = pt.GetInt("initial_value");

    _drawCounter = false;
    _drawHorizontal = false;
    _cellWidth = 0.25f;
    _cellSpacing = 0.25f;
    pt.TryGetBool("draw_counter", &_drawCounter);    
    pt.TryGetBool("draw_horizontal", &_drawHorizontal);
    pt.TryGetFloat("cell_width", &_cellWidth);
    pt.TryGetFloat("cell_spacing", &_cellSpacing);

    _synthPatchChannel = -1;
    _startPatchName.clear();
    _endPatchName.clear();
    pt.TryGetInt("synth_patch_channel", &_synthPatchChannel);    
    pt.TryGetString("start_patch", &_startPatchName);    
    pt.TryGetString("end_patch", &_endPatchName);

    SeqAction::LoadActionsFromChildNode(pt, "actions", _actions);
}

ne::Entity::ImGuiResult IntVariableEntity::ImGuiDerived(GameManager& g) {
    bool changed = ImGui::InputInt("Initial value", &_initialValue);

    ImGui::Checkbox("Draw", &_drawCounter);
    ImGui::Checkbox("Draw horiz", &_drawHorizontal);
    ImGui::InputFloat("Cell width", &_cellWidth, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::InputFloat("Cell spacing", &_cellSpacing, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);

    SeqAction::ImGui("Actions on zero", _actions);

    ImGui::InputInt("Synth patch channel", &_synthPatchChannel);
    imgui_util::InputText<64>("Start patch", &_startPatchName);
    imgui_util::InputText<64>("End patch", &_endPatchName);

    if (changed) {
        return ne::Entity::ImGuiResult::NeedsInit;
    } else {
        return ne::Entity::ImGuiResult::Done;
    }    
}

void IntVariableEntity::Draw(GameManager& g, float dt) {
    if (_drawCounter) {
        DrawCounter(g);
    }
}

void IntVariableEntity::AddToVariable(int amount) {
    bool wasZero = _currentValue == 0;
    _currentValue += amount;
    _beatTimeOfLastAdd = _g->_beatClock->GetBeatTimeFromEpoch();

    if (_startPatch != nullptr && _endPatch != nullptr && _synthPatchChannel >= 0) {
        float const blendFactor = 1.f - math_util::Clamp((static_cast<float>(_currentValue) / static_cast<float>(_initialValue)), 0.f, 1.f);
        double constexpr kBlendBeatTime = 0.0;
        double blendSecs = _g->_beatClock->BeatTimeToSecs(kBlendBeatTime);
        audio::Event e;
        e.type = audio::EventType::SynthParam;
        e.channel = _synthPatchChannel;
        e.paramChangeTimeSecs = blendSecs;
        for (int paramIx = 0, n = (int)audio::SynthParamType::Count; paramIx < n; ++paramIx) {
            audio::SynthParamType paramType = (audio::SynthParamType)paramIx;
            float startV = _startPatch->Get(paramType);
            float endV = _endPatch->Get(paramType);
            float v = startV + blendFactor * (endV - startV);
            e.param = paramType;
            e.newParamValue = v;
            _g->_audioContext->AddEvent(e);
        }
    }    

    if (!wasZero && _currentValue == 0) {
        for (auto const& pAction : _actions) {
            pAction->Execute(*_g);
        }
    }
}

void IntVariableEntity::Reset() {
    _currentValue = _initialValue;

    double constexpr kBlendBeatTime = 0.0;
    auto blendTimeSecs = _g->_beatClock->BeatTimeToSecs(kBlendBeatTime);
    audio::Event e;
    e.type = audio::EventType::SynthParam;
    e.channel = _synthPatchChannel;
    e.paramChangeTimeSecs = blendTimeSecs;
    for (int paramIx = 0, n = (int)audio::SynthParamType::Count; paramIx < n; ++paramIx) {
        audio::SynthParamType paramType = (audio::SynthParamType)paramIx;
        float v = _startPatch->Get(paramType);
        e.param = paramType;
        e.newParamValue = v;
        _g->_audioContext->AddEvent(e);
    }
}

void IntVariableEntity::DrawCounter(GameManager& g) {
    int numSteps = _initialValue;
    float totalWidth = numSteps * _cellWidth + (_cellSpacing * (numSteps - 1));
    Vec3 direction;
    if (_drawHorizontal) {
        direction.Set(1.f, 0.f, 0.f);
    } else {
        direction.Set(0.f, 0.f, 1.f);
    }
    Transform t;
    t.SetScale(Vec3(_cellWidth, _cellWidth, _cellWidth));
    Vec3 pos = _transform.Pos();
    pos -= 0.5f * totalWidth * direction;    
    pos += 0.5f * _cellWidth * direction;    
    int numOn = _initialValue - _currentValue;
    Vec4 constexpr kNoHitColor = Vec4(0.245098054f, 0.933390915f, 1.f, 1.f);
    Vec4 constexpr kHitColor = Vec4(0.933390915f, 0.245098054f, 1.f, 1.f);

    double const beatTime = g._beatClock->GetBeatTimeFromEpoch();
    double beatTimeSinceLastAdd = std::numeric_limits<double>::max();
    double constexpr kAnimTime = 0.5f;
    // float constexpr kNewlyActiveExtraScale = 4.f;
    float constexpr kNewlyActiveExtraScale = 2.f;
    float newlyActiveScaleFactor = 1.f;
    float alpha = _modelColor._w;
    // if (_currentValue == 0) {
    //     alpha = 0.f;
    // }
    if (_beatTimeOfLastAdd >= 0.0) {
        beatTimeSinceLastAdd = beatTime - _beatTimeOfLastAdd;
        if (beatTimeSinceLastAdd <= kAnimTime) {
            float factor = math_util::Clamp(beatTimeSinceLastAdd / kAnimTime, 0.0, 1.0);
            float scaleFactor = math_util::SmoothUpAndDown(factor);
            newlyActiveScaleFactor += scaleFactor * kNewlyActiveExtraScale;

            // if (_currentValue == 0) {
            //     alpha = _modelColor._w + factor * (0.f - _modelColor._w);
            // }
        } else {
            _beatTimeOfLastAdd = -1.0;
        }
    }
    for (int i = 0; i < numSteps; ++i) {
        Vec4 color;
        if (i < numOn) {
            color = kHitColor;
            t.SetScale(Vec3(_cellWidth, _cellWidth, _cellWidth) * newlyActiveScaleFactor);
        } else {
            color = kNoHitColor;
            t.SetScale(Vec3(_cellWidth, _cellWidth, _cellWidth));
        }
        color._w = alpha;

        t.SetTranslation(pos);
        pos += (_cellWidth + _cellSpacing) * direction;
    }
}
