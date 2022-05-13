#include <fstream>
#include <iostream>
#include <vector>

#include "glm/glm.hpp"

#include "cereal/archives/json.hpp"
#include <cereal/types/vector.hpp>

enum class EventType {
    NoteOn, NoteOff
};

struct Event {
    EventType type;
    int channel = -1;
    long timeInTicks = 0;
    int midiNote = 0;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(type), CEREAL_NVP(channel), CEREAL_NVP(timeInTicks), CEREAL_NVP(midiNote));
    }
};

struct Foo {
    std::vector<Event> _events;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(_events));
    }
};

struct Bar {
    glm::vec3 v;

    template<typename Archive>
    void save(Archive& ar) const {
        float x = v.x;
        float y = v.y;
        float z = v.z;
        //ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
        ar(CEREAL_NVP(x));
        ar(CEREAL_NVP(y));
        ar(CEREAL_NVP(z));
    }

    template<typename Archive>
    void load(Archive& ar) {
        float x, y, z;
        ar(x, y, z);
        v = glm::vec3(x,y,z);
    }
};

int main() {
    Foo foo;
    Event e;
    e.type = EventType::NoteOn;
    e.channel = 0;
    e.timeInTicks = 500;
    e.midiNote = 69;
    foo._events.push_back(e);

    e.type = EventType::NoteOff;
    e.timeInTicks = 1000;
    foo._events.push_back(e);

    Bar b;
    b.v = glm::vec3(1.f);

    {
        std::ofstream outFile("test.json");
        cereal::JSONOutputArchive archive(outFile);
        archive(e, foo, b);
    }

    return 0;
}