#include "entities/typing_slider.h"

#include <imgui/imgui.h>

#include "game_manager.h"
#include "input_manager.h"
#include "imgui_util.h"
#include "renderer.h"

void TypingSliderEntity::SaveDerived(serial::Ptree pt) const {
    pt.PutString("text", _p._text.c_str());
    SeqAction::SaveActionsInChildNode(pt, "actions", _p._actions);
}

void TypingSliderEntity::LoadDerived(serial::Ptree pt) {
    _p = Props();
    pt.TryGetString("text", &_p._text);
    SeqAction::LoadActionsFromChildNode(pt, "actions", _p._actions);
}

ne::BaseEntity::ImGuiResult TypingSliderEntity::ImGuiDerived(GameManager& g) {
    ImGuiResult result = ImGuiResult::Done;
    if (imgui_util::InputText<64>("Text", &_p._text)) {
        result = ImGuiResult::NeedsInit;
    }
    if (SeqAction::ImGui("Actions", _p._actions)) {
        result = ImGuiResult::NeedsInit;
    }
    return result;
}

ne::BaseEntity::ImGuiResult TypingSliderEntity::MultiImGui(GameManager& g, BaseEntity** entities, size_t entityCount) {
    ImGuiResult result = ImGuiResult::Done;
    return result;
}


void TypingSliderEntity::InitDerived(GameManager& g) {
    _s = State();
    for (auto const& pAction : _p._actions) {
        pAction->Init(g);
    }
}

void TypingSliderEntity::UpdateDerived(GameManager& g, float dt) {
    if (g._editMode) {
        return;
    } 

    InputManager::Key currentValueKey = InputManager::Key::NumKeys;
    if (_s._index >= 0 && _s._index < _p._text.length()) {
        currentValueKey = InputManager::CharToKey(_p._text[_s._index]);
    }
    InputManager::Key nextValueKey = InputManager::Key::NumKeys;
    if (_s._index + 1 < _p._text.length()) {
        nextValueKey = InputManager::CharToKey(_p._text[_s._index + 1]);
    }
    InputManager::Key prevValueKey = InputManager::Key::NumKeys;
    if (_s._index - 1 >= 0) {
        prevValueKey = InputManager::CharToKey(_p._text[_s._index - 1]);
    }

    bool pressed = false;
    if (g._inputManager->IsKeyPressedThisFrame(currentValueKey)) {
        pressed = true;
    } else if (nextValueKey != InputManager::Key::NumKeys && g._inputManager->IsKeyPressedThisFrame(nextValueKey)) {
        pressed = true;
        ++_s._index;
    } else if (prevValueKey != InputManager::Key::NumKeys && g._inputManager->IsKeyPressedThisFrame(prevValueKey)) {
        pressed = true;
        --_s._index; 
    }

    if (pressed) {
        for (auto& action : _p._actions) {
            action->Execute(g);
        }
    }
}

void TypingSliderEntity::Draw(GameManager& g, float dt) {
    BaseEntity::Draw(g, dt);

    Transform renderTrans = _transform;

    float constexpr kTextSize = 1.5f;
#if 0
    float constexpr kFadeAlpha = 0.2f;
    Vec4 constexpr kFadeColor(1.f, 1.f, 1.f, kFadeAlpha);
    Vec4 constexpr kActiveColor(1.f, 1.f, 0.f, 1.f);
    Vec4 constexpr kReadyColor(1.f, 1.f, 1.f, 1.f);
#else
    Vec4 constexpr kFadeColor(0.8f, 0.8f, 0.8f, 1.f);
    Vec4 constexpr kReadyColor = kFadeColor;
    Vec4 constexpr kActiveColor(1.f, 1.f, 0.f, 1.f);
#endif
    bool needToAppend = false;
    if (_s._index > 1) {
        std::string_view fadedText = std::string_view(_p._text).substr(0, _s._index - 1);
        if (fadedText.length() > 0) {
            g._scene->DrawTextWorld(fadedText, renderTrans.GetPos(), kTextSize, kFadeColor); 
            needToAppend = true;
        }
    }

    if (_s._index > 0) {
        if (_s._index - 1 < _p._text.size()) {
            char readyLetter[] = "a";
            readyLetter[0] = _p._text[_s._index - 1];
            g._scene->DrawTextWorld(std::string_view(readyLetter, 1), renderTrans.GetPos(), kTextSize, kReadyColor, /*appendToPrevious=*/needToAppend);
            needToAppend = true;
        }
    }

    if (_s._index >= 0 && _s._index < _p._text.length()) {
        char activeLetter[] = "a";
        activeLetter[0] = _p._text[_s._index];
        g._scene->DrawTextWorld(std::string_view(activeLetter, 1), renderTrans.GetPos(), kTextSize, kActiveColor, needToAppend);
        needToAppend = true;
    }

    if (_s._index + 1 < _p._text.size() && _s._index >= 0) {
        char readyLetter[] = "a";
        readyLetter[0] = _p._text[_s._index + 1];
        g._scene->DrawTextWorld(std::string_view(readyLetter, 1), renderTrans.GetPos(), kTextSize, kReadyColor, /*appendToPrevious=*/needToAppend);
        needToAppend = true;
    }

    if (_s._index + 2 < _p._text.length()) {
        std::string_view fadedText = std::string_view(_p._text).substr(_s._index + 2);
        if (fadedText.length() > 0) {
            g._scene->DrawTextWorld(fadedText, renderTrans.GetPos(), kTextSize, kFadeColor, needToAppend);
            needToAppend = true;
        }
    }
}

void TypingSliderEntity::OnEditPick(GameManager& g) {
}
