#include <cstdio>
#include <cassert>

#include "version_id_list.h"
#include "version_id_list_simple.h"

struct Foo {
    int x, y, z;
    float* bar = nullptr;
};

// struct Foo {
//     int x, y, z;
//     float* bar = nullptr;
//     Foo() {
//         bar = new float[14];
//         for (int i = 0; i < 14; ++i) {
//             bar[i] = -1.;
//         }
//     }
//     Foo(Foo const& rhs) = delete;
//     Foo& operator=(Foo const& rhs) = delete;
//     Foo(Foo&& rhs) {
//         // x = rhs.x;
//         // y = rhs.y;
//         // z = rhs.z;
//         // bar = rhs.bar;
//         // rhs.bar = nullptr;
//         *this = std::move(rhs);
//     }
//     Foo& operator=(Foo&& rhs) {
//         if (bar != nullptr) {
//             delete[] bar;
//         }
//         x = rhs.x;
//         y = rhs.y;
//         z = rhs.z;
//         bar = rhs.bar;
//         rhs.bar = nullptr;
//         return *this;
//     }
//     ~Foo() {
//         delete[] bar;
//     }
// };

// struct Foo {
//     int x, y, z;
//     std::unique_ptr<float[]> bar;
//     Foo() {
//         bar = std::make_unique<float[]>(14);
//         for (int i = 0; i < 14; ++i) {
//             bar[i] = -1.;
//         }
//     }
//     Foo(Foo const& rhs) = delete;
//     Foo& operator=(Foo const& rhs) = delete;
//     Foo(Foo&& rhs) {
//         x = rhs.x;
//         y = rhs.y;
//         z = rhs.z;
//         bar = std::move(rhs.bar);
//     }
//     Foo& operator=(Foo&& rhs) {
//         x = rhs.x;
//         y = rhs.y;
//         z = rhs.z;
//         bar = std::move(rhs.bar);
//         return *this;
//     }
// };

int main() {
    int const N = 200;

    VersionIdList pool(sizeof(Foo), N*2);

    VersionId ids[N];
    for (int i = 0; i < N; ++i) {
        Foo* newItem;
        ids[i] = pool.AddItem((void**)&newItem);
        newItem->x = i;
        newItem->y = i;
        newItem->z = i;
        newItem->bar = new float[14];
        for (int i = 0; i < 14; ++i) {
            newItem->bar[i] = i;
        }
    }

    for (int i = 0; i < N/2; ++i) {
        Foo* f = (Foo*) pool.GetItem(ids[i]);
        delete[] f->bar;
        assert(pool.RemoveItem(ids[i]));
    }

    for (int i = 0; i < N/2; ++i) {
        assert(pool.GetItem(ids[i]) == nullptr);
    }
    for (int i = N/2; i < N; ++i) {
        Foo* newItem = (Foo*) pool.GetItem(ids[i]);
        printf("%d %d %d\n", newItem->x, newItem->y, newItem->z);
    }

    printf("====================\n");

    for (int i = 0; i < N/2; ++i) {
        Foo* newItem;
        ids[i] = pool.AddItem((void**)&newItem);
        newItem->x = i;
        newItem->y = i;
        newItem->z = i;
        newItem->bar = new float[14];
        for (int i = 0; i < 14; ++i) {
            newItem->bar[i] = i;
        }
    }

    for (int i = 0; i < pool.GetCount(); ++i) {
        Foo* newItem = (Foo*) pool.GetItemAtIndex(i);
        printf("%d %d %d\n", newItem->x, newItem->y, newItem->z);
    }

    printf("====================\n");

    for (int i = 0; i < N; ++i) {
        Foo* newItem = (Foo*) pool.GetItem(ids[i]);
        printf("(%d %d) %d %d %d\n", ids[i].GetIndex(), ids[i].GetVersion(), newItem->x, newItem->y, newItem->z);
    }

    for (int i = 0; i < pool.GetCount(); ++i) {
        Foo* f = (Foo*) pool.GetItemAtIndex(i);
        delete[] f->bar;
    }

    return 0;
}

// int main() {
//     TVersionIdList<Foo> pool(100);

//     VersionId ids[50];
//     for (int i = 0; i < 50; ++i) {
//         Foo* newItem;
//         ids[i] = pool.AddItem(&newItem);
//         newItem->x = i;
//         newItem->y = i;
//         newItem->z = i;
//     }

//     for (int i = 0; i < 25; ++i) {
//         assert(pool.RemoveItem(ids[i]));
//     }

//     for (int i = 0; i < 25; ++i) {
//         assert(pool.GetItem(ids[i]) == nullptr);
//     }
//     for (int i = 25; i < 50; ++i) {
//         Foo* newItem = pool.GetItem(ids[i]);
//         printf("%d %d %d\n", newItem->x, newItem->y, newItem->z);
//     }

//     printf("====================\n");

//     for (int i = 0; i < 25; ++i) {
//         Foo* newItem;
//         ids[i] = pool.AddItem(&newItem);
//         newItem->x = 50 + i;
//         newItem->y = 50 + i;
//         newItem->z = 50 + i;
//     }

//     for (int i = 0; i < pool.GetCount(); ++i) {
//         Foo* newItem = pool.GetItemAtIndex(i);
//         printf("%d %d %d\n", newItem->x, newItem->y, newItem->z);
//     }

//     printf("====================\n");

//     for (int i = 0; i < 50; ++i) {
//         Foo* newItem = pool.GetItem(ids[i]);
//         printf("(%d %d) %d %d %d\n", ids[i].GetIndex(), ids[i].GetVersion(), newItem->x, newItem->y, newItem->z);
//     }

//     return 0;
// }

// int main() {
//     int const N = 2;

//     // std::vector<Foo> foos;
//     // foos.reserve(N);
//     // for (int i = 0; i < N; ++i) {
//     //     foos.emplace_back();
//     //     Foo* newItem = &foos.back();
//     //     newItem->x = i;
//     //     newItem->y = i;
//     //     newItem->z = i;
//     //     newItem->bar[0] = (float) i;
//     // }

//     TVersionIdList<Foo> pool;

//     VersionId ids[N];
//     for (int i = 0; i < N; ++i) {
//         Foo* newItem;
//         ids[i] = pool.AddItem(&newItem);
//         newItem->x = i;
//         newItem->y = i;
//         newItem->z = i;
//         newItem->bar[0] = (float) i;
//     }

//     for (int i = 0; i < N/2; ++i) {
//         assert(pool.RemoveItem(ids[i]));
//     }

//     for (int i = 0; i < N/2; ++i) {
//         assert(pool.GetItem(ids[i]) == nullptr);
//     }
//     for (int i = N/2; i < N; ++i) {
//         Foo* newItem = pool.GetItem(ids[i]);
//         printf("%d %d %d\n", newItem->x, newItem->y, newItem->z);
//     }

//     printf("====================\n");

//     for (int i = 0; i < N/2; ++i) {
//         Foo* newItem;
//         ids[i] = pool.AddItem(&newItem);
//         newItem->x = N + i;
//         newItem->y = N + i;
//         newItem->z = N + i;
//     }

//     for (int i = 0; i < pool.GetCount(); ++i) {
//         Foo* newItem = pool.GetItemAtIndex(i);
//         printf("%d %d %d\n", newItem->x, newItem->y, newItem->z);
//     }

//     printf("====================\n");

//     for (int i = 0; i < N; ++i) {
//         Foo* newItem = pool.GetItem(ids[i]);
//         printf("(%d %d) %d %d %d\n", ids[i].GetIndex(), ids[i].GetVersion(), newItem->x, newItem->y, newItem->z);
//     }

//     return 0;
// }