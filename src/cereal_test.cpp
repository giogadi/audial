#include <fstream>
#include <iostream>
#include <vector>
#include <memory>

#include "cereal/archives/json.hpp"
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>

struct Foo {
    virtual ~Foo() {}
    virtual void Howdy() = 0;
};

struct Bar : public Foo {
    virtual void Howdy() override {
        std::cout << "bar1 " << _x << " " << _y << " " << _z << std::endl;
    }
    float _x, _y, _z;
};

template<class Archive>
void serialize(Archive & archive,
               Bar & m)
{
  archive( m._x, m._y, m._z );
}

struct Bar2 : public Foo {
    virtual void Howdy() override {
        std::cout << "bar2 " << _a << " " << _b << " " << _c << std::endl;
    }
    float _a, _b, _c;
};

template<class Archive>
void serialize(Archive & archive,
               Bar2 & m)
{
  archive( m._a, m._b, m._c );
}

struct Baz {
    int howdy;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive( howdy);
    }
};

CEREAL_REGISTER_TYPE(Bar);
CEREAL_REGISTER_TYPE(Bar2);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Foo, Bar);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Foo, Bar2);

int main() {
    std::shared_ptr<Bar> barShared = std::make_shared<Bar>();
    barShared->_x = 0.f;
    barShared->_y = 1.f;
    barShared->_z = 2.f;

    std::shared_ptr<Bar> barShared2 = barShared;

    std::vector<std::unique_ptr<Foo>> items;
    {
        std::unique_ptr<Bar> bar = std::make_unique<Bar>();
        bar->_x = 0.f;
        bar->_y = 1.f;
        bar->_z = 2.f;        
        items.push_back(std::move(bar));
    }
    {
        std::unique_ptr<Bar2> bar = std::make_unique<Bar2>();
        bar->_a = 5.f;
        bar->_b = 6.f;
        bar->_c = 7.f;
        items.push_back(std::move(bar));
    }

    {
        std::ofstream outFile("test.json");
        cereal::JSONOutputArchive archive(outFile);
        archive(items, CEREAL_NVP(barShared), CEREAL_NVP(barShared2));
    }
    {
        std::ifstream inFile("test.json");
        cereal::JSONInputArchive archive(inFile);
        std::vector<std::unique_ptr<Foo>> testing;
        std::shared_ptr<Bar> b1, b2;
        archive(testing, b1, b2);
        // for (auto const& f : testing) {
        //     f->Howdy();
        // }
        b1->Howdy();
        b2->Howdy();
    }

    return 0;
}