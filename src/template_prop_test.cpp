#include "property.h"
#include "serial.h"

template<typename Serializer, typename ObjectType>
void Serialize(Serializer s);

void PropSave(serial::Ptree pt, char const* name, int const& x) {
    pt.PutInt(name, x);
}
void PropSave(serial::Ptree pt, char const* name, float const& x) {
    pt.PutFloat(name, x);
}
void PropLoad(serial::Ptree pt, char const* name, int& x) {
    pt.TryGetInt(name, &x);
}
void PropLoad(serial::Ptree pt, char const* name, float& x) {
    pt.TryGetFloat(name, &x);
}

template<typename Serializer, typename ObjectType>
struct PropSerial {
    void Serialize(Serializer s);
};

struct Foo {
    int x;
    float y;
};

template<typename Serializer>
struct PropSerial<Serializer, Foo> {
    static void Serialize(Serializer s) {
        s.Leaf(MakeProperty("x", &Foo::x));
        s.Leaf(MakeProperty("y", &Foo::y));
    }
};

struct Bar {
    Foo foo;
    int z; 
};
template<typename Serializer>
struct PropSerial<Serializer, Bar> {
    static void Serialize(Serializer s) {
        s.Node(MakeProperty("foo", &Bar::foo));
        s.Leaf(MakeProperty("z", &Bar::z));
    }
};

struct Baz {
    float w;
    Bar bar;
};
template<typename Serializer>
struct PropSerial<Serializer, Baz> {
    static void Serialize(Serializer s) {
        s.Leaf(MakeProperty("w", &Baz::w));
        s.Node(MakeProperty("bar", &Baz::bar));
    }
};


#include "property_util.h"

int main() {
    serial::Ptree pt = serial::Ptree::MakeNew();

    Foo foo { 5, 3.f };
    
    PropertySaveFunctor saveFn = PropertySaveFunctor_Make(foo, pt);
    PropSerial<decltype(saveFn), Foo>::Serialize(saveFn); 
 
    
    pt.DeleteData();

    {
        serial::Ptree pt = serial::Ptree::MakeNew();
        Bar bar { { 1, 2.f }, -4 };
        
        PropertySaveFunctor saveFn = PropertySaveFunctor_Make(bar, pt);
        PropSerial<decltype(saveFn), Bar>::Serialize(saveFn);

        Bar bar2;
        PropertyLoadFunctor loadFn = PropertyLoadFunctor_Make(bar2, pt);
        PropSerial<decltype(loadFn), Bar>::Serialize(loadFn);

        pt.DeleteData();
    }

    {
        serial::Ptree pt = serial::Ptree::MakeNew();
        Baz baz { -1.f, { {3, -2.f}, 6 } };
        
        PropertySaveFunctor saveFn = PropertySaveFunctor_Make(baz, pt);
        PropSerial<decltype(saveFn), Baz>::Serialize(saveFn);

        Baz baz2;
        PropertyLoadFunctor loadFn = PropertyLoadFunctor_Make(baz2, pt);
        PropSerial<decltype(loadFn), Baz>::Serialize(loadFn);

        pt.DeleteData();
    }


    return 0;
}
