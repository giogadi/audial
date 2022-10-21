#pragma once

class SoundBank;

namespace audio {

struct Event;

void EventDrawImGuiNoTime(Event& event, SoundBank const& soundBank);

}