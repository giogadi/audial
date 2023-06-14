#include "change_patch.h"

#include "imgui/imgui.h"
#include "audio.h"

extern GameManager gGameManager;

void ChangePatchSeqAction::LoadDerived(serial::Ptree pt) {
    _channelIx = pt.GetInt("channel");
    _blendBeatTime = pt.GetDouble("blend_beat_time");
    serial::LoadFromChildOf(pt, "patch", _patch);
}
void ChangePatchSeqAction::SaveDerived(serial::Ptree pt) const {
    pt.PutInt("channel", _channelIx);
    pt.PutDouble("blend_beat_time", _blendBeatTime);
    serial::SaveInNewChildOf(pt, "patch", _patch);
}
bool ChangePatchSeqAction::ImGui() {
    ImGui::InputInt("Channel", &_channelIx);
    ImGui::InputDouble("Blend beat time", &_blendBeatTime);
    if (ImGui::Button("Import current patch")) {
        auto const& synths = gGameManager._audioContext->_state.synths;
        if (_channelIx >= 0 && _channelIx < synths.size()) {
            synth::Patch const& currentPatch = synths[_channelIx].patch;
            _patch = currentPatch;
        }
    }
    if (ImGui::Button("Apply current patch")) {
        audio::Event e;
        e.channel = _channelIx;
        e.timeInTicks = 0;
        e.type = audio::EventType::SynthParam;
        e.paramChangeTime = 0;
        for (int ii = 0; ii < (int)audio::SynthParamType::Count; ++ii) {
            auto type = (audio::SynthParamType) ii;
            e.param = type;
            e.newParamValue = _patch.Get(type);
            gGameManager._audioContext->AddEvent(e);
        }
    }
    _patch.ImGui();
    return false;
}
void ChangePatchSeqAction::ExecuteDerived(GameManager& g) {
    if (g._editMode) {
        return;
    }
    auto blendTickTime = g._beatClock->BeatTimeToTickTime(_blendBeatTime);
    audio::Event e;
    e.type = audio::EventType::SynthParam;
    e.channel = _channelIx;
    e.paramChangeTime = blendTickTime;
    for (int paramIx = 0, n = (int)audio::SynthParamType::Count; paramIx < n; ++paramIx) {
        audio::SynthParamType paramType = (audio::SynthParamType) paramIx;
        float v = _patch.Get(paramType);
        e.param = paramType;
        e.newParamValue = v;
        g._audioContext->AddEvent(e);
    }
}
