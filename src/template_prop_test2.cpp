#include <vector>
#include <cstdio>

struct Foo {
    int x;
    float y;
};

struct Bar {
    Foo foo;
    int z;
    std::vector<float> yy;
};

struct Baz {
    std::vector<Foo> foos;
    float w;
};

template<typename Serializer, typename T>
struct SerializeFn {
    static void Serialize(Serializer s);
};

template<typename Serializer>
struct SerializeFn<Serializer, Foo> {
    static void Serialize(Serializer s) {
        s("x", &Foo::x);
        s("y", &Foo::y);
    }
};

template<typename Serializer>
struct SerializeFn<Serializer, Bar> {
    static void Serialize(Serializer s) {
        s("foo", &Bar::foo);
        s("z", &Bar::z);
        s("yy", &Bar::yy);
    }
};

template<typename Serializer>
struct SerializeFn<Serializer, Baz> {
    static void Serialize(Serializer s) {
        s("foos", &Baz::foos);
        s("w", &Baz::w);
    }
};

template<typename T>
struct IsLeaf {
    static bool const value = false;
};
template<>
struct IsLeaf<int> {
    static bool const value = true;
};
template<>
struct IsLeaf<float> {
    static bool const value = true;
};

void Save(char const* name, int const& x) {
    printf("%s: %d\n", name, x);
}
void Save(char const* name, float const& x) {
    printf("%s: %f\n", name, x);
}

// template <class C, typename T>
// T GetPointerType(T C::*v);

template<typename ObjectType>
struct Saver;

template<typename ObjectType>
struct Saver {
    ObjectType const* object;

    template<typename ValueType>
    void operator()(char const* name, ValueType ObjectType::*pMember, typename std::enable_if<IsLeaf<ValueType>::value>::type* = 0) {
        ValueType const& v = object->*pMember;
        Save(name, v);
    }

    // ptree or whatever
    template<typename ValueType>
    void operator()(char const* name, ValueType ObjectType::*pMember, typename std::enable_if<!IsLeaf<ValueType>::value>::type* = 0) {
        printf("Struct %s\n", name);
        Saver<ValueType> itemSaver;
        itemSaver.object = &(object->*pMember);
        SerializeFn<Saver<ValueType>, ValueType>::Serialize(itemSaver);
    }

    template<typename ValueType>
    void operator()(char const* name, std::vector<ValueType> ObjectType::*pMember, typename std::enable_if<!IsLeaf<ValueType>::value>::type* = 0) {
        printf("vector %s\n", name);
        std::vector<ValueType> const& items = object->*pMember;
        for (ValueType const& item : items) {
            Saver<ValueType> itemSaver;
            itemSaver.object = &item;
            SerializeFn<Saver<ValueType>, ValueType>::Serialize(itemSaver);
        }
    }

    template<typename ValueType>
    void operator()(char const* name, std::vector<ValueType> ObjectType::*pMember, typename std::enable_if<IsLeaf<ValueType>::value>::type* = 0) {
        printf("vector %s\n", name);
        std::vector<ValueType> const& items = object->*pMember;
        for (ValueType const& item : items) {
            Save(name, item);            
        }
    }
};

int main() {
    Foo foo;
    foo.x = 0;
    foo.y = 1.5f;

    Bar bar;
    bar.foo = foo;
    bar.z = 3;
    bar.yy.push_back(378.3f);
    bar.yy.push_back(400.1f);

    {
        printf("********\n");
        Saver<Foo> s;
        s.object = &foo;
        SerializeFn<Saver<Foo>, Foo>::Serialize(s);
    }

    {
        printf("********\n");
        Saver<Bar> s;
        s.object = &bar;
        SerializeFn<Saver<Bar>, Bar>::Serialize(s);
    }

    {
        printf("********\n");
        Baz baz;
        baz.foos.push_back(foo);
        baz.foos.push_back(foo);
        baz.foos.push_back(foo);
        baz.foos.back().x = 99;
        baz.w = 98.9f;

        Saver<Baz> s;
        s.object = &baz;
        SerializeFn<Saver<Baz>, Baz>::Serialize(s);
    }

    return 0;
}