#include <cstdio>
#include <memory>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "serial.h"
#include "util.h"

struct Property;

typedef void (*PropLoadFn)(void* v, serial::Ptree pt);
typedef void (*PropSaveFn)(void const* v, serial::Ptree pt);
typedef bool (*PropImGuiFn)(char const* name, void* v);
typedef void (*PropCopyFn)(void* dst, void const* src);
struct PropertySpec;
struct PropertySpec {
    size_t size;
    PropLoadFn loadFn;
    PropSaveFn saveFn;
    PropImGuiFn imguiFn;
    PropCopyFn copyFn;
    Property* subProperties;
    size_t numSubProperties;

};

struct Property {
    char const* name;
    PropertySpec const* spec; 
    size_t offset;

    // Array-specific
    size_t arrayCapacity;
    size_t countOffset;
    size_t imguiIxOffset;
};

void PropertyArrayLoad(void* objects, size_t* countOut, size_t capacity, PropertySpec const* subSpec, serial::Ptree pt);
void PropertyArraySave(void const* vIn, size_t count, PropertySpec const* subSpec, serial::Ptree pt);
bool PropertyArrayImGui(void* objects, size_t capacity, size_t* count, int* imguiIndex, char const* name, PropertySpec const* subSpec);

void PropertyLoad(PropertySpec const* spec, void* object, serial::Ptree pt) {
    if (spec->loadFn) {
        spec->loadFn(object, pt);
        return;
    }
    for (int ii = 0; ii < spec->numSubProperties; ++ii) {
        Property const* prop = &spec->subProperties[ii]; 
        serial::Ptree childPt = pt.TryGetChild(prop->name);
        if (childPt.IsValid()) {
            void* v = (char*)object + prop->offset;
            if (prop->arrayCapacity > 0) {
                size_t* count = (size_t*)((char*)object + prop->countOffset);
                PropertyArrayLoad(v, count, prop->arrayCapacity, prop->spec, childPt);
            } else {
                PropertyLoad(prop->spec, v, childPt);
            } 
        }
    }
}

void PropertySave(PropertySpec const* spec, void const* object, serial::Ptree pt) {
    if (spec->saveFn) {
        spec->saveFn(object, pt);
        return;
    }
    for (int ii = 0; ii < spec->numSubProperties; ++ii) {
        Property const* prop = &spec->subProperties[ii];
        serial::Ptree childPt = pt.AddChild(prop->name);
        if (childPt.IsValid()) {
            void const* v = (char const*)object + prop->offset;
            if (prop->arrayCapacity > 0) {
                size_t const* count = (size_t const*)((char const*)object + prop->countOffset);
                PropertyArraySave(v, *count, prop->spec, childPt);
            } else {
                PropertySave(prop->spec, v, childPt);
            }
        }
    }
}

bool PropertyImGui(PropertySpec const* spec, char const* name, void* object) {
    if (spec->imguiFn) {
        return spec->imguiFn(name, object);
    }
    if (ImGui::TreeNode(name)) {
        for (int ii = 0; ii < spec->numSubProperties; ++ii) {
            Property const& prop = spec->subProperties[ii];
            void* v = (char*)object + prop.offset;
            if (prop.arrayCapacity > 0) {
                size_t* count = (size_t*)((char*)object + prop.countOffset);
                int* imguiIndex = (int*)((char*)object + prop.imguiIxOffset);

                PropertyArrayImGui(v, prop.arrayCapacity, count, imguiIndex, prop.name, prop.spec);
            } else {
                PropertyImGui(prop.spec, prop.name, v);
            }
        }
        ImGui::TreePop();
    }
    return false;
}

// TODO: still not sure about these offsets.
void MultiImGuiInternal(void** objects, size_t objOffset, size_t numObjects, PropertySpec const* spec, char const* name) {
    void* firstObject = (char*)objects[0] + objOffset;
    if (spec->imguiFn) {
        bool changed = spec->imguiFn(name, firstObject);

        if (changed) {
            assert(spec->copyFn);
            for (int objIx = 0; objIx < numObjects; ++objIx) {
                void* obj = (char*)objects[objIx] + objOffset;
                spec->copyFn(obj, firstObject);
            }
        }
        
        return;
    }
    if (ImGui::TreeNode(name)) {
        for (int propIx = 0; propIx < spec->numSubProperties; ++propIx) {
            Property const& prop = spec->subProperties[propIx];
            if (prop.arrayCapacity > 0) {
                // TODO
            } else {
                MultiImGuiInternal(objects, objOffset + prop.offset, numObjects, prop.spec, prop.name);
            }
        }
        ImGui::TreePop();
    }
}

void MultiImGui(void** objects, size_t numObjects, PropertySpec const* spec, char const* name) {
    MultiImGuiInternal(objects, 0, numObjects, spec, name);
}

void PropertyIntLoad(void* vIn, serial::Ptree pt) {
    int* v = static_cast<int*>(vIn);
    *v = pt.GetIntValue(); 
}
void PropertyIntSave(void const* vIn, serial::Ptree pt) {
    int const* v = static_cast<int const*>(vIn);
    pt.PutIntValue(*v);
}
bool PropertyIntImGui(char const* name, void* vIn) {
    int* v = static_cast<int*>(vIn);
    return ImGui::InputInt(name, v);
}
void PropertyIntCopy(void* dst, void const* src) {
    int* dstInt = (int*)dst;
    int const* srcInt = (int const*)src;
    *dstInt = *srcInt;
}

PropertySpec gPropertySpecInt = {
    .size = sizeof(int),
    .loadFn = &PropertyIntLoad,
    .saveFn = &PropertyIntSave,
    .imguiFn = &PropertyIntImGui,
    .copyFn = &PropertyIntCopy
};

void PropertyFloatLoad(void* vIn, serial::Ptree pt) {
    float* v = static_cast<float*>(vIn);
    *v = pt.GetFloatValue(); 
}
void PropertyFloatSave(void const* vIn, serial::Ptree pt) {
    float const* v = static_cast<float const*>(vIn);
    pt.PutFloatValue(*v);
}
bool PropertyFloatImGui(char const* name, void* vIn) {
    float* v = static_cast<float*>(vIn);
    return ImGui::InputFloat(name, v);
}
void PropertyFloatCopy(void* dstIn, void const* srcIn) {
    float* dst = (float*)dstIn;
    float const* src = (float const*)srcIn;
    *dst = *src;
}

PropertySpec gPropertySpecFloat = {
    .size = sizeof(float),
    .loadFn = &PropertyFloatLoad,
    .saveFn = &PropertyFloatSave,
    .imguiFn = &PropertyFloatImGui,
    .copyFn = &PropertyFloatCopy
};

void PropertyArrayLoad(void* objects, size_t* countOut, size_t capacity, PropertySpec const* subSpec, serial::Ptree pt) {
    int numChildren = 0;
    serial::NameTreePair* children = pt.GetChildren(&numChildren);
    if (numChildren > capacity) {
        printf("WARNING: Trying to load an array with too many children! (%d > %zu)\n", numChildren, capacity);
        numChildren = capacity;
    }
    for (int ii = 0; ii < numChildren; ++ii) {
        serial::Ptree childPt = children[ii]._pt;
        void* child = (char*)(objects) + subSpec->size * ii;
        PropertyLoad(subSpec, child, childPt);
    }
    *countOut = numChildren;
}
void PropertyArraySave(void const* vIn, size_t count, PropertySpec const* subSpec, serial::Ptree pt) {
    for (int ii = 0; ii < count; ++ii) {
        serial::Ptree childPt = pt.AddChild("item");
        void const* object = (char const*)vIn + subSpec->size * ii;
        PropertySave(subSpec, object, childPt);
    } 
}
bool PropertyArrayImGui(void* objects, size_t capacity, size_t* count, int* imguiIndex, char const* name, PropertySpec const* subSpec) {
    if (ImGui::TreeNode(name)) { 
        if (ImGui::Button("Add")) {
            if (*count < capacity) {
                ++(*count);
            } 
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove")) {
            if (*count > 0 && *imguiIndex > 0 && *imguiIndex < *count) {
                for (int ii = *imguiIndex; ii < *count - 1; ++ii) {
                    // TODO: copyFn?
                    void* dst = (char*)objects + ii * subSpec->size;
                    void* src = (char*)objects + (ii+1) * subSpec->size;
                    memcpy(dst, src, subSpec->size);
                }
                --(*count);
            }
        }
        if (ImGui::BeginListBox(name)) {
            for (int itemIx = 0; itemIx < *count; ++itemIx) {
                char nameBuf[8];
                sprintf(nameBuf, "%d", itemIx); 
                bool selected = itemIx == *imguiIndex;
                bool clicked = ImGui::Selectable(nameBuf, selected);
                if (clicked) {
                    *imguiIndex = itemIx;
                }
            }  
            ImGui::EndListBox();
        }

        if (*imguiIndex >= 0 && *imguiIndex < *count) {
            void* item = (char*)objects + subSpec->size * (*imguiIndex);
            PropertyImGui(subSpec, "howdy", item);
        }

        ImGui::TreePop();
    }

    return false;
}

struct Foo {
    int x;
    float y;
};
Property gPropertiesFoo[] = {
    {
        .name = "x",
        .spec = &gPropertySpecInt,
        .offset = offsetof(Foo, x)
    },
    {
        .name = "y",
        .spec = &gPropertySpecFloat,
        .offset = offsetof(Foo, y)
    }
};
PropertySpec gPropertySpecFoo {
    .size = sizeof(Foo),
    .subProperties = gPropertiesFoo,
    .numSubProperties = M_ARRAY_LEN(gPropertiesFoo)
};

struct Bar {
    Foo foo;
    int z;
};
Property gPropertiesBar[] = {
    {
        .name = "foo",
        .spec = &gPropertySpecFoo,
        .offset = offsetof(Bar, foo)
    },
    {
        .name = "z",
        .spec = &gPropertySpecInt,
        .offset = offsetof(Bar, z)
    }
};
PropertySpec gPropertySpecBar {
    .size = sizeof(Bar),
    .subProperties = gPropertiesBar,
    .numSubProperties = M_ARRAY_LEN(gPropertiesBar)
};

struct Baz {
    Bar bars[4];
    size_t barsCount;
    float y;
    int imguiIndex;
};
Property gPropertiesBaz[] = {
    {
        .name = "bars",
        .spec = &gPropertySpecBar,
        .offset = offsetof(Baz, bars),
        .arrayCapacity = M_ARRAY_LEN(Baz::bars),
        .countOffset = offsetof(Baz, barsCount),
        .imguiIxOffset = offsetof(Baz, imguiIndex)
    },
    {
        .name = "y",
        .spec = &gPropertySpecFloat,
        .offset = offsetof(Baz, y)
    }
};
PropertySpec gPropertySpecBaz {
    .size = sizeof(Baz),
    .subProperties = gPropertiesBaz,
    .numSubProperties = M_ARRAY_LEN(gPropertiesBaz)
};


void Test1() {
    Foo foo {
        .x = 1,
        .y = 2.5f
    };

    Bar bar {
        .foo = foo,
        .z = 5
    };

    serial::Ptree pt = serial::Ptree::MakeNew();
    PropertySave(&gPropertySpecBar, &bar, pt);

    Bar bar2 {};
    PropertyLoad(&gPropertySpecBar, &bar2, pt);
    pt.DeleteData(); 
}

void Test2() {
    Bar bars[4];
    for (int ii = 0; ii < 4; ++ii) {
        bars[ii] = Bar {
            .foo = {
                .x = 1,
                .y = 2.5f
            },
            .z = 5
        };
    }

    serial::Ptree pt = serial::Ptree::MakeNew();
    PropertyArraySave(bars, 4, &gPropertySpecBar, pt);

    Bar bars2[4]; 
    size_t bars2Count = 0;
    PropertyArrayLoad((void*)bars2, &bars2Count, 4, &gPropertySpecBar, pt);
    assert(bars2Count == 4);
    
    pt.DeleteData();  
}

void Test3() {
    Baz baz;

    for (int ii = 0; ii < 3; ++ii) {
        baz.bars[ii] = Bar {
            .foo = {
                .x = 1,
                .y = 2.5f
            },
            .z = 5
        };
    }
    baz.barsCount = 3;

    serial::Ptree pt = serial::Ptree::MakeNew();
    PropertySave(&gPropertySpecBaz, &baz, pt);

    Baz baz2;
    PropertyLoad(&gPropertySpecBaz, &baz2, pt);
    assert(baz2.barsCount == 3);
    
    pt.DeleteData();  

}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {

    Test1();
    Test2();
    Test3();

    Foo foo {
        .x = 1,
        .y = 2.5f
    };

    Bar bar {
        .foo = foo,
        .z = 5
    };

    Bar bars[10];
    for (int ii = 0; ii < 10; ++ii) {
        bars[ii] = bar;
    }

    Baz baz;
    baz.bars[0] = baz.bars[1] = baz.bars[2] = bar;
    baz.barsCount = 3;
        
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window;
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        GLFWvidmode const* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        // Full screen
        // window = glfwCreateWindow(mode->width, mode->height, "Audial", monitor, NULL);

        window = glfwCreateWindow(mode->width, mode->height, "Audial", NULL, NULL);
    }

    if (window == nullptr) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync?
    // glfwSwapInterval(0);  // disable vsync?

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    // glfwSetKeyCallback(window, KeyCallback);

    glEnable(GL_DEPTH_TEST);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // TODO: should I be setting this? imgui_impl_opengl3.h says it's ok to be null.
    ImGui_ImplOpenGL3_Init(/*glsl_version=*/NULL);  

    bool multiSelect = false;
    while (!glfwWindowShouldClose(window)) {
        {
            // ImGuiIO& io = ImGui::GetIO();
            // bool inputEnabled = !io.WantCaptureMouse && !io.WantCaptureKeyboard;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Checkbox("Multiselect", &multiSelect);

        // PropertyImGui(&gPropertySpecFoo, "foo", &foo);
        // PropertyImGui(&gPropertySpecBar, "bar", &bar); 
#if 0
        if (multiSelect) {
            Bar* pBars[10];
            for (int ii = 0; ii < 10; ++ii) {
                pBars[ii] = &bars[ii];
            }
            MultiImGui((void**)pBars, 10, &gPropertySpecBar, "bars"); 
        } else {
            for (int ii = 0; ii < 10; ++ii) {
                char buf[8];
                sprintf(buf, "bar %d", ii);
                PropertyImGui(&gPropertySpecBar, buf, &bars[ii]);
            }
        }
#endif

        PropertyImGui(&gPropertySpecBaz, "baz", &baz);
        
        ImGui::Render();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        glfwPollEvents();

    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}
