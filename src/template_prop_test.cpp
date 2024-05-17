#include <cstdio>
#include <memory>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "property.h"
#include "serial.h"
#include "util.h"

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

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
bool PropImGui(char const* name, int& x) {
    return ImGui::InputInt(name, &x);
}
bool PropImGui(char const* name, float& x) {
    return ImGui::InputFloat(name, &x);
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

void EntityImGui(BaseEntity& e) {
    EntityType type = e.Type();
    PropertyImGuiFunctor<BaseEntity> baseImGuiFn = PropertyImGuiFunctor_Make(e);
    PropSerial<decltype(baseImGuiFn), BaseEntity>::Serialize(baseImGuiFn);
    switch (type) {
        case EntityType::Base: break;
        case EntityType::Derived1: {
            Derived1Entity& derived = static_cast<Derived1Entity&>(e);
            PropertyImGuiFunctor<Derived1Entity> imguiFn = PropertyImGuiFunctor_Make(derived);
            PropSerial<decltype(imguiFn), Derived1Entity>::Serialize(imguiFn);
            break;
        }
        case EntityType::Derived2: {
            Derived2Entity& derived = static_cast<Derived2Entity&>(e);
            PropertyImGuiFunctor<Derived2Entity> imguiFn = PropertyImGuiFunctor_Make(derived);
            PropSerial<decltype(imguiFn), Derived2Entity>::Serialize(imguiFn);
            break;
        } 
    }
}

// void TypeSpecificMultiEntityImGui(std::vector<std::unique_ptr<BaseEntity>>& entities) {
    // TODO GROSS
    // std::vector
// }

void MultiEntityImGui(std::vector<std::unique_ptr<BaseEntity>>& entities) {
    if (entities.empty()) {
        return;
    }
    // common base props first
    // TODO DISGUSTING
    BaseEntity** entityPtrs = new BaseEntity*[entities.size()];
    for (int ii = 0; ii < entities.size(); ++ii) {
        entityPtrs[ii] = entities[ii].get();
    }
    
    PropertyMultiImGuiFunctor<BaseEntity, BaseEntity> baseImGuiFn = PropertyMultiImGuiFunctor_Make<BaseEntity, BaseEntity>(entityPtrs, entities.size());
    PropSerial<decltype(baseImGuiFn), BaseEntity>::Serialize(baseImGuiFn);

    EntityType type = entities.front()->Type();
    bool allSameType = true;
    for (int ii = 1; ii < entities.size(); ++ii) {
        if (entities[ii]->Type() != type) {
            allSameType = false;
            break;
        } 
    }

    if (allSameType) {
        switch (type) {
            case EntityType::Base: {
                break;
            }
            case EntityType::Derived1: {
                PropertyMultiImGuiFunctor<Derived1Entity, BaseEntity> imguiFn = PropertyMultiImGuiFunctor_Make<Derived1Entity, BaseEntity>(entityPtrs, entities.size());
                PropSerial<decltype(imguiFn), Derived1Entity>::Serialize(imguiFn);
                break;
            }
            case EntityType::Derived2: {
                PropertyMultiImGuiFunctor<Derived2Entity, BaseEntity> imguiFn = PropertyMultiImGuiFunctor_Make<Derived2Entity, BaseEntity>(entityPtrs, entities.size());
                PropSerial<decltype(imguiFn), Derived2Entity>::Serialize(imguiFn);

                break;
            }
        }
    }

    delete[] entityPtrs;
}

void MakeEntities(std::vector<std::unique_ptr<BaseEntity>>& entities) {
    {
        std::unique_ptr<BaseEntity> base = std::make_unique<BaseEntity>();
        base->bar.foo.x = 0;
        base->bar.foo.y = 1.f;
        base->bar.z = 2;
        base->x = 2.5f;
        entities.push_back(std::move(base));
    }

    {
        std::unique_ptr<Derived1Entity> derived1 = std::make_unique<Derived1Entity>();
        derived1->bar = entities[0]->bar;
        derived1->x = entities[0]->x;
        derived1->y = 3;
        entities.push_back(std::move(derived1));
    }

    {
        std::unique_ptr<Derived2Entity> derived2 = std::make_unique<Derived2Entity>();
        derived2->bar = entities[0]->bar;
        derived2->x = entities[0]->x;
        derived2->z = 4.f;
        entities.push_back(std::move(derived2));
    }
}

void MakeEntitiesSame(std::vector<std::unique_ptr<BaseEntity>>& entities) {
    {
        std::unique_ptr<Derived1Entity> derived1 = std::make_unique<Derived1Entity>();
        derived1->bar.foo.x = 0;
        derived1->bar.foo.y = 1.f;
        derived1->bar.z = 2;

        derived1->x = 2.5f;
        derived1->y = 3;
        entities.push_back(std::move(derived1));
    }

    {
        std::unique_ptr<Derived1Entity> derived1 = std::make_unique<Derived1Entity>();
        derived1->bar = entities[0]->bar;
        derived1->x = entities[0]->x;
        derived1->y = 4;
        entities.push_back(std::move(derived1));
    }

    {
        std::unique_ptr<Derived1Entity> derived1 = std::make_unique<Derived1Entity>();
        derived1->bar = entities[0]->bar;
        derived1->x = entities[0]->x;
        derived1->y = 5;
        entities.push_back(std::move(derived1));
    }

}

void Tests() {
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

}

// TODO: gotta handle
// - lists of stuff
// - multiselect ImGui
//

// ....do we add a window and shit to test the ImGui stuff?

int main() {

    std::vector<std::unique_ptr<BaseEntity>> entities;
    // MakeEntities(entities);
    MakeEntitiesSame(entities);
    
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

        if (multiSelect) {
            MultiEntityImGui(entities);
        } else {
            for (int ii = 0; ii < entities.size(); ++ii) {
                auto& entity = entities[ii];
                char buf[32];
                sprintf(buf, "Entity %d", ii);
                if (ImGui::TreeNode(buf)) {
                    EntityImGui(*entity);

                    ImGui::TreePop();
                }
            }
        }
        
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
