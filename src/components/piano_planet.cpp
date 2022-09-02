#include "piano_planet.h"

#include "imgui/imgui.h"

#include "math_util.h"
#include "game_manager.h"
#include "input_manager.h"
#include "entity_manager.h"
#include "components/model_instance.h"
#include "audio.h"

bool PianoPlanetComponent::ConnectComponents(EntityId id, Entity& e, GameManager& g) {
    _g = &g;
    _entityId = id;
    return true;
}

void PianoPlanetComponent::Update(float const dt) {
    double motionX, motionY;
    _g->_inputManager->GetMouseMotion(motionX, motionY);

    // if (scrollX == 0.0 && scrollY == 0.0) {
    //     return;
    // }

    // float constexpr kScreenUnitToValueUnit = 4.f * (1.f / 600.f);
    float const screenUnitToValueUnit = 1.f / _valueRangeInPixels;

    _currentValue += motionY * screenUnitToValueUnit;
    _currentValue = math_util::Clamp(_currentValue, 0.f, 1.f);

    if (Entity* entity = _g->_entityManager->GetEntity(_entityId)) {
        // change the model's color based on the current value.
        std::shared_ptr<ModelComponent> pModel = entity->FindComponentOfType<ModelComponent>().lock();
        if (pModel != nullptr) {
            pModel->_color = Vec4(_currentValue, _currentValue, _currentValue, 1.f);
        }

        // Play a note based on the current value.
        int numNotes = _midiNotes.size();
        if (numNotes > 0) {
            int noteIndex = std::min(static_cast<int>(numNotes * _currentValue), numNotes - 1);
            assert(noteIndex >= 0 && noteIndex < numNotes);
            int noteToPlay = _midiNotes[noteIndex];
            if (noteToPlay != _currentNote) {
                audio::Event e;
                e.type = audio::EventType::AllNotesOff;
                e.channel = 1;
                _g->_audioContext->AddEvent(e);

                e.type = audio::EventType::NoteOn;
                e.midiNote = noteToPlay;
                _g->_audioContext->AddEvent(e);

                _currentNote = noteToPlay;
            }
        }
    }
}

bool PianoPlanetComponent::DrawImGui() {
    ImGui::InputScalar("Pixel Range##", ImGuiDataType_Float, &_valueRangeInPixels);

    if (ImGui::Button("Add Note##")) {
        _midiNotes.push_back(-1);
    }
    if (ImGui::Button("Remove Note##")) {
        _midiNotes.pop_back();
    }
    for (int i = 0, n = _midiNotes.size(); i < n; ++i) {
        ImGui::PushID(i);
        ImGui::InputScalar("Note##", ImGuiDataType_S32, &_midiNotes[i]);
        ImGui::PopID();
    }

    return false;
}

void PianoPlanetComponent::Save(serial::Ptree pt) const {
    pt.PutFloat("pixel_range", _valueRangeInPixels);
    serial::Ptree notesPt = pt.AddChild("notes");
    for (int n : _midiNotes) {
        notesPt.PutInt("note", n);
    }
}

void PianoPlanetComponent::Load(serial::Ptree pt) {
    _valueRangeInPixels = pt.GetFloat("pixel_range");
    
    serial::Ptree notesPt = pt.GetChild("notes");
    int numChildren;
    serial::NameTreePair* children = notesPt.GetChildren(&numChildren);
    _midiNotes.clear();
    _midiNotes.reserve(numChildren);
    for (int i = 0; i < numChildren; ++i) {
        _midiNotes.push_back(children[i]._pt.GetIntValue());
    }
    delete[] children;
}