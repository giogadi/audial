#include "property.h"
#include "serial.h"
#include "util.h"

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

enum class EntityType { Base, Derived1, Derived2 };
char const* gEntityTypeStrings[] = {
    "Base", "Derived1", "Derived2"
};

struct BaseEntity {
    virtual EntityType Type() const { return EntityType::Base; }
    virtual ~BaseEntity() {}
    virtual void Print() const {
        printf("Base: %f\n", x);
    }
    Bar bar;
    float x;
};
template<typename Serializer>
struct PropSerial<Serializer, BaseEntity> {
    static void Serialize(Serializer s) {
        s.Node(MakeProperty("bar", &BaseEntity::bar));
        s.Leaf(MakeProperty("x", &BaseEntity::x));
    }
};

struct Derived1Entity : public BaseEntity {
    virtual EntityType Type() const override { return EntityType::Derived1; }
    virtual void Print() const override {
        printf("Derived1: %d\n", y);
    }
    int y;
};
template<typename Serializer>
struct PropSerial<Serializer, Derived1Entity> {
    static void Serialize(Serializer s) {
        s.Leaf(MakeProperty("y", &Derived1Entity::y));
    }
};

struct Derived2Entity : public BaseEntity {
    virtual EntityType Type() const override { return EntityType::Derived2; }
    virtual void Print() const override {
        printf("Derived2: %f\n", z);
    }
    float z;
};
template<typename Serializer>
struct PropSerial<Serializer, Derived2Entity> {
    static void Serialize(Serializer s) {
        s.Leaf(MakeProperty("z", &Derived2Entity::z));
    }
};

#include "property_util.h"

void SaveEntity(BaseEntity& e, serial::Ptree pt) {
    EntityType type = e.Type();
    serial::Ptree childPt = pt.AddChild(gEntityTypeStrings[static_cast<int>(type)]);
    PropertySaveFunctor<BaseEntity> baseSaveFn = PropertySaveFunctor_Make(e, childPt);
    PropSerial<decltype(baseSaveFn), BaseEntity>::Serialize(baseSaveFn);
    switch (type) {
        case EntityType::Base: {
            break;
        }
        case EntityType::Derived1: {
            Derived1Entity& derived = static_cast<Derived1Entity&>(e);
            PropertySaveFunctor<Derived1Entity> saveFn = PropertySaveFunctor_Make(derived, childPt);
            PropSerial<decltype(saveFn), Derived1Entity>::Serialize(saveFn);
            break;
        }
        case EntityType::Derived2: {
            Derived2Entity& derived = static_cast<Derived2Entity&>(e);
            PropertySaveFunctor<Derived2Entity> saveFn = PropertySaveFunctor_Make(derived, childPt);
            PropSerial<decltype(saveFn), Derived2Entity>::Serialize(saveFn);
            break;
        }
    }
}

void LoadEntity(BaseEntity& e, serial::Ptree pt) {
    EntityType type = e.Type();
    PropertyLoadFunctor<BaseEntity> baseLoadFn = PropertyLoadFunctor_Make(e, pt);
    PropSerial<decltype(baseLoadFn), BaseEntity>::Serialize(baseLoadFn);
    switch (type) {
        case EntityType::Base: {
            break;
        }
        case EntityType::Derived1: {
            Derived1Entity& derived = static_cast<Derived1Entity&>(e);
            PropertyLoadFunctor<Derived1Entity> loadFn = PropertyLoadFunctor_Make(derived, pt);
            PropSerial<decltype(loadFn), Derived1Entity>::Serialize(loadFn);
            break;
        }
        case EntityType::Derived2: {
            Derived2Entity& derived = static_cast<Derived2Entity&>(e);
            PropertyLoadFunctor<Derived2Entity> loadFn = PropertyLoadFunctor_Make(derived, pt);
            PropSerial<decltype(loadFn), Derived2Entity>::Serialize(loadFn);
            break;
        }
    }
}



int main() {
    {
        serial::Ptree pt = serial::Ptree::MakeNew();

        Foo foo { 5, 3.f };
    
        PropertySaveFunctor saveFn = PropertySaveFunctor_Make(foo, pt);
        PropSerial<decltype(saveFn), Foo>::Serialize(saveFn); 
 
    
        pt.DeleteData();
    }

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

    {
        BaseEntity* base = new BaseEntity;
        base->bar.foo.x = 0;
        base->bar.foo.y = 1.f;
        base->bar.z = 2;
        base->x = 2.5f;

        Derived1Entity* derived1 = new Derived1Entity;
        derived1->bar = base->bar;
        derived1->x = base->x;
        derived1->y = 3;

        Derived2Entity* derived2 = new Derived2Entity;
        derived2->bar = base->bar;
        derived2->x = base->x;
        derived2->z = 4.f;

        BaseEntity* entities[] = { base, derived1, derived2 };
        
        serial::Ptree pt = serial::Ptree::MakeNew();
        
        for (int ii = 0; ii < M_ARRAY_LEN(entities); ++ii) {
            BaseEntity* e = entities[ii];
            SaveEntity(*e, pt); 
        } 

        int numChildren;
        serial::NameTreePair* children = pt.GetChildren(&numChildren);  
        assert(numChildren == 3);
        BaseEntity* entities2[3];
        for (int ii = 0; ii < 3; ++ii) {
            int foundEntityTypeIx = -1; 
            for (int entityTypeIx = 0; entityTypeIx < M_ARRAY_LEN(gEntityTypeStrings); ++entityTypeIx) {
                if (strcmp(children[ii]._name, gEntityTypeStrings[entityTypeIx]) == 0) {
                    foundEntityTypeIx = entityTypeIx;
                    break;
                }
            }
            assert(foundEntityTypeIx >= 0);
            EntityType type = static_cast<EntityType>(foundEntityTypeIx);
            BaseEntity* e = nullptr;
            switch (type) {
                case EntityType::Base: {
                    e = new BaseEntity;
                    break;
                }
                case EntityType::Derived1: {
                    e = new Derived1Entity;
                    break;
                }
                case EntityType::Derived2: {
                    e = new Derived2Entity;
                    break;
                }
            }

            LoadEntity(*e, children[ii]._pt);
            entities2[ii] = e;
        }
        delete[] children; 

        for (int ii = 0; ii < 3; ++ii) {
            entities2[ii]->Print();
        }

        pt.DeleteData();
    }


    return 0;
}
