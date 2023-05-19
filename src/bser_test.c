#include "stdio.h"
#include "stdint.h"
#include "assert.h"
#include "string.h"

#define M_ARRAY_LEN(V) (sizeof(V)/sizeof((V)[0]))

struct BufferStream {
    char data[UINT16_MAX];
    uint16_t pos;
};

void BufferStream_write(void* src, size_t numBytes, struct BufferStream* stream) {
    assert((size_t)stream->pos + numBytes <= M_ARRAY_LEN(stream->data));
    memcpy((char*)(stream->data) + stream->pos, src, numBytes);
    stream->pos += numBytes;
}

void BufferStream_read(void* dst, size_t numBytes, struct BufferStream* stream) {
    assert((size_t)stream->pos + numBytes <= M_ARRAY_LEN(stream->data));
    memcpy(dst, (char*)(stream->data) + stream->pos, numBytes);
    stream->pos += numBytes;
}

enum Foo_Members {
    Foo_Members_myFloat,
    Foo_Members_myInt, 
    Foo_Members_myString,
    Foo_Members_COUNT
};
enum Foo_Member_Ids {
    Foo_Members_Ids_myFloat = 0,
    // reserved 1
    Foo_Members_Ids_myInt = 2,
    Foo_Members_Ids_myString = 3
};
struct Foo {
    float myFloat;
    int myInt;
    char myString[16];
};
uint16_t const gFooMemberIds[] = {
    0, 2, 3
};

// Do we need to know in advance how big this'll be? _can_ we know this? whatever, keep it constant for now.
void Foo_Save(struct Foo* me, FILE* f) {
    // TODO: do we need to write the type of this struct here?

    uint16_t memberCount = Foo_Members_COUNT;
    fwrite(&memberCount, sizeof(memberCount), 1, f);
    assert(M_ARRAY_LEN(gFooMemberIds) == Foo_Members_COUNT);

    //
    // myFloat
    //
    uint16_t memberId = gFooMemberIds[Foo_Members_myFloat];
    fwrite(&memberId, sizeof(memberId), 1, f);
    assert(sizeof(me->myFloat) <= UINT16_MAX);
    uint16_t memberSize = sizeof(me->myFloat);
    fwrite(&memberSize, sizeof(memberSize), 1, f);   
    
    // Float time. FIGURE OUT HOW TO DO THIS PORTABLY
    fwrite(&me->myFloat, memberSize, 1, f);

    //
    // myInt
    //
    memberId = gFooMemberIds[Foo_Members_myInt];
    fwrite(&memberId, sizeof(memberId), 1, f);
    assert(sizeof(me->myInt) <= UINT16_MAX);
    memberSize = sizeof(me->myFloat);
    fwrite(&memberSize, sizeof(memberSize), 1, f);
    fwrite(&me->myInt, memberSize, 1, f);   

    //
    // myString
    //
    memberId = gFooMemberIds[Foo_Members_myString];
    fwrite(&memberId, sizeof(memberId), 1, f);
    assert(sizeof(me->myString) <= UINT16_MAX);
    memberSize = sizeof(me->myString);
    fwrite(&memberSize, sizeof(memberSize), 1, f);
    fwrite(&me->myString, memberSize, 1, f);
}

void Foo_SaveBuf(struct Foo* me, struct BufferStream* f) {
    // TODO: do we need to write the type of this struct here?

    uint16_t memberCount = Foo_Members_COUNT;
    BufferStream_write(&memberCount, sizeof(memberCount), f);
    assert(M_ARRAY_LEN(gFooMemberIds) == Foo_Members_COUNT);

    //
    // myFloat
    //
    uint16_t memberId = gFooMemberIds[Foo_Members_myFloat];
    BufferStream_write(&memberId, sizeof(memberId), f);
    assert(sizeof(me->myFloat) <= UINT16_MAX);
    uint16_t memberSize = sizeof(me->myFloat);
    BufferStream_write(&memberSize, sizeof(memberSize), f);
    
    // Float time. FIGURE OUT HOW TO DO THIS PORTABLY
    BufferStream_write(&me->myFloat, memberSize, f);

    //
    // myInt
    //
    memberId = gFooMemberIds[Foo_Members_myInt];
    BufferStream_write(&memberId, sizeof(memberId), f);
    assert(sizeof(me->myInt) <= UINT16_MAX);
    memberSize = sizeof(me->myFloat);
    BufferStream_write(&memberSize, sizeof(memberSize), f);
    BufferStream_write(&me->myInt, memberSize, f);

    //
    // myString
    //
    memberId = gFooMemberIds[Foo_Members_myString];
    BufferStream_write(&memberId, sizeof(memberId), f);
    assert(sizeof(me->myString) <= UINT16_MAX);
    memberSize = sizeof(me->myString);
    BufferStream_write(&memberSize, sizeof(memberSize), f);
    BufferStream_write(&me->myString, memberSize, f);
}

void Foo_Load(struct Foo* me, FILE* f) {
    // TODO: zero out the struct?
    
    uint16_t memberCount;
    fread(&memberCount, sizeof(memberCount), 1, f);    

    for (uint16_t ii = 0; ii < memberCount; ++ii) {
        uint16_t memberId;
        fread(&memberId, sizeof(memberId), 1, f);
        uint16_t memberSize;
        fread(&memberSize, sizeof(memberSize), 1, f);
        switch(memberId) {
            case Foo_Members_Ids_myFloat:
                fread(&me->myFloat, sizeof(me->myFloat), 1, f);
                break;
            case Foo_Members_Ids_myInt:
                fread(&me->myInt, sizeof(me->myInt), 1, f);
                break;
            case Foo_Members_Ids_myString:
                fread(me->myString, sizeof(me->myString), 1, f);
                break;
            default:
                printf("unrecognized member id: %u\n", memberId);
                fseek(f, memberSize, SEEK_CUR);
                break;
        }
    }
}

enum Bar_Members {
    Bar_Members_myFoo,
    Bar_Members_myString,
    Bar_Members_COUNT
};
enum Bar_Member_Ids {
    Bar_Members_Ids_myFoo = 1,
    Bar_Members_Ids_myString = 2
};
struct Bar {
    struct Foo myFoo;
    char myString[16];
};
uint16_t const gBarMemberIds[] = {
    1, 2
};

void Bar_Save(struct Bar* me, FILE* f) {
    // TODO: do we need to write the type of this struct here?

    uint16_t memberCount = Bar_Members_COUNT;
    fwrite(&memberCount, sizeof(memberCount), 1, f);
    assert(M_ARRAY_LEN(gBarMemberIds) == Bar_Members_COUNT);

    //
    // myFoo
    //
    uint16_t memberId = gBarMemberIds[Bar_Members_myFoo];
    fwrite(&memberId, sizeof(memberId), 1, f);
    struct BufferStream buf = {};
    Foo_SaveBuf(&me->myFoo, &buf);
    fwrite(&buf.pos, sizeof(buf.pos), 1, f);
    fwrite(buf.data, buf.pos, 1, f);       

    //
    // myString
    //
    memberId = gBarMemberIds[Bar_Members_myString];
    fwrite(&memberId, sizeof(memberId), 1, f);
    assert(sizeof(me->myString) <= UINT16_MAX);
    uint16_t memberSize = sizeof(me->myString);
    fwrite(&memberSize, sizeof(memberSize), 1, f);
    fwrite(&me->myString, memberSize, 1, f);
}

void Bar_Load(struct Bar* me, FILE* f) {
    // TODO: zero out the struct?
    
    uint16_t memberCount;
    fread(&memberCount, sizeof(memberCount), 1, f);    

    for (uint16_t ii = 0; ii < memberCount; ++ii) {
        uint16_t memberId;
        fread(&memberId, sizeof(memberId), 1, f);
        uint16_t memberSize;
        fread(&memberSize, sizeof(memberSize), 1, f);
        switch(memberId) {
            case Bar_Members_Ids_myFoo:
                // NOT A POD!!!
                Foo_Load(&me->myFoo, f);
                break;
            case Bar_Members_Ids_myString:
                fread(me->myString, sizeof(me->myString), 1, f);
                break;
            default:
                printf("unrecognized member id: %u\n", memberId);
                fseek(f, memberSize, SEEK_CUR);
                break;
        }
    }
}

int main() {
    /* struct Foo foo = {}; */
    /* foo.myInt = 4; */
    /* foo.myFloat = 3.14f; */
    /* strcpy(foo.myString, "howdy"); */

    /* FILE* f = fopen("bser_test.dat", "wb"); */
    /* Foo_Save(&foo, f); */
    /* fclose(f); */

    /* f = fopen("bser_test.dat", "rb"); */
    /* struct Foo newFoo; */
    /* Foo_Load(&newFoo, f); */
    /* fclose(f); */

    struct Bar bar = {
        .myFoo = {
            .myInt = 4,
            .myFloat = 3.14f,
        },        
    };
    strcpy(bar.myFoo.myString, "howdy");
    strcpy(bar.myString, "pardner");

    FILE* f = fopen("bser_test.dat", "wb");
    Bar_Save(&bar, f);
    fclose(f);

    f = fopen("bser_test.dat", "rb");
    struct Bar newBar = newBar;
    Bar_Load(&newBar, f);
    fclose(f);
    
    return 0;
}
