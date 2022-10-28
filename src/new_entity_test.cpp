#include <cstdio>

#include "new_entity.h"

using namespace ne;

int main() {
    EntityManager mgr;
    EntityId base;
    {
        Entity* e = mgr.AddEntity(EntityType::Base);
        e->_transform.SetTranslation(Vec3(1.f, 2.f, 3.f));
        base = e->_id;
    }

    EntityId light, light2;
    {
        LightEntity* e = (LightEntity*) mgr.AddEntity(EntityType::Light);
        e->_transform.SetTranslation(Vec3(-1.f, -2.f, -3.f));
        e->_ambient.Set(0.1f, 0.1f, 0.1f);
        e->_diffuse.Set(0.4f, 0.4f, 0.4f);
        light = e->_id;

        e = (LightEntity*) mgr.AddEntity(EntityType::Light);
        e->_transform.SetTranslation(Vec3(0.f, 0.f, 0.f));
        e->_ambient.Set(0.f, 0.f, 0.f);
        e->_diffuse.Set(0.2f, 0.2f, 0.2f);
        light2 = e->_id;
    }

    Entity* pBase = mgr.GetEntity(base);
    Vec3 const& trans = pBase->_transform.GetPos();
    pBase->DebugPrint();

    LightEntity* pLight = (LightEntity*)mgr.GetEntity(light);
    pLight->DebugPrint();

    printf("***********\n");

    EntityManager::AllIterator iter = mgr.GetAllIterator();
    // {
    //     Entity* e = iter.GetEntity();
    //     e->DebugPrint();

    //     iter.Next();
    //     e = iter.GetEntity();
    //     e->DebugPrint();

    //     iter.Next();
    //     assert(iter.Finished());
    // }
    for (; !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        e->DebugPrint();
    }

    printf("************\n");

    assert(mgr.RemoveEntity(base));
    assert(mgr.RemoveEntity(light));

    iter = mgr.GetAllIterator();
    for (; !iter.Finished(); iter.Next()) {
        Entity* e = iter.GetEntity();
        e->DebugPrint();
    }

    printf("************\n");

    pLight = (LightEntity*)mgr.GetEntity(light2);
    pLight->DebugPrint();

    assert(mgr.GetEntity(base) == nullptr);
    assert(mgr.GetEntity(light) == nullptr);

    return 0;
}