#pragma once

class BeatClock;

namespace audio {

class Event;

void EventDrawImGuiBeatTime(Event& event, BeatClock const& beatClock);

}