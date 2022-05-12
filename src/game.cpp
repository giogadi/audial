#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>

#include <portaudio.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include "stb_image.h"

#include "audio.h"
#include "beat_clock.h"
#include "shader.h"
#include "cube_verts.h"
#include "input_manager.h"
#include "component.h"
#include "components/player_controller.h"
#include "components/beep_on_hit.h"

void InitEventQueueWithSequence(audio::EventQueue* queue, BeatClock const& beatClock) {
    double const firstNoteBeatTime = beatClock.GetDownBeatTime() + 1.0;
    for (int i = 0; i < 16; ++i) {
        double const beatTime = firstNoteBeatTime + (double)i;
        unsigned long const tickTime = beatClock.BeatTimeToTickTime(beatTime);

        audio::Event e;
        e.type = audio::EventType::NoteOn;
        e.timeInTicks = tickTime;
        e.midiNote = 69 + i;
        queue->push(e);

        e.type = audio::EventType::NoteOff;
        e.timeInTicks = beatClock.BeatTimeToTickTime(beatTime + 0.5);
        queue->push(e);
    }
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void CreateCube(SceneManager* sceneMgr, BoundMesh const& mesh, Entity* e) {
    auto unique = std::make_unique<TransformComponent>();
    TransformComponent* t = unique.get();
    e->_components.push_back(std::move(unique));
    e->_components.push_back(std::make_unique<ModelComponent>(t, &mesh, sceneMgr));
}

void CreateLight(SceneManager* sceneMgr, Entity* e) {
    auto unique = std::make_unique<TransformComponent>();
    TransformComponent* t = unique.get();
    e->_components.push_back(std::move(unique));
    e->_components.push_back(std::make_unique<LightComponent>(t, glm::vec3(0.2f,0.2f,0.2f), glm::vec3(1.f,1.f,1.f), sceneMgr));
}

void CreateCamera(SceneManager* sceneMgr, InputManager* inputMgr, Entity* e) {
    auto unique = std::make_unique<TransformComponent>();
    TransformComponent* t = unique.get();
    e->_components.push_back(std::move(unique));
    e->_components.push_back(std::make_unique<CameraComponent>(t, inputMgr, sceneMgr));
}

void DrawImGuiWindow() {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

int main() {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32("data/sounds/kick.wav", &channels, &sampleRate, &totalPCMFrameCount, NULL);
    if (pSampleData == NULL) {
        // Error opening and reading WAV file.
        std::cout << "error opening and reading wav file" << std::endl;
        return 1;
    }
    std::cout << "wav file: " << channels << " channels, " << totalPCMFrameCount << " frames." << std::endl;

    audio::Context audioContext;
    if (audio::Init(audioContext, pSampleData, totalPCMFrameCount) != paNoError) {
        return 1;
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "Audial", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync?
    // glfwSwapInterval(0);  // disable vsync?

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

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

    Shader shaderProgram;
    if (!shaderProgram.Init("shaders/shader.vert", "shaders/shader.frag")) {
        return 1;
    }

    std::unique_ptr<Texture> texture =
        Texture::CreateTextureFromFile("data/textures/wood_container.jpg");
    if (texture == nullptr) {
        return 1;
    }

    Material material;
    material._shader = shaderProgram;
    material._texture = texture.get();

    BoundMesh cubeMesh;
    {
        std::array<float,kCubeVertsNumValues> cubeVerts;
        GetCubeVertices(&cubeVerts);
        int const numCubeVerts = 36;
        cubeMesh.Init(cubeVerts.data(), numCubeVerts, &material);
    }

    BeatClock beatClock(/*bpm=*/120.0, (unsigned int)sampleRate, audioContext._stream);
    beatClock.Update();

    InputManager inputManager(window);

    SceneManager sceneManager;

    EntityManager entityManager;

    HitManager hitManager;

    // Camera
    Entity* camera = entityManager.AddEntity();
    CreateCamera(&sceneManager, &inputManager, camera);
    {
        TransformComponent* t = camera->DebugFindComponentOfType<TransformComponent>();
        float angle = glm::radians(45.f);
        glm::vec3 dir(0.f, sin(angle), cos(angle));
        float dist = 15.f;
        t->SetPos(dist * dir);
        t->_transform = glm::rotate(t->_transform, -angle, glm::vec3(1.f, 0.f, 0.f));
    }

    // Light
    Entity* light = entityManager.AddEntity();
    CreateLight(&sceneManager, light);
    {
        TransformComponent* t = light->DebugFindComponentOfType<TransformComponent>();
        t->SetPos(glm::vec3(0.f, 3.f, 0.f));
    }

    // Cube1
    Entity* cube1 = entityManager.AddEntity();
    CreateCube(&sceneManager, cubeMesh, cube1);
    {
        TransformComponent* tComp = cube1->DebugFindComponentOfType<TransformComponent>();
        // glm::mat4& t = tComp->_transform;
        // t = glm::rotate(t, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
        // t = glm::rotate(t, glm::radians(45.f), glm::vec3(1.f, 0.f, 0.f));
        // t = glm::translate(t, glm::vec3(1.f, 0.f, 0.f));

        cube1->_components.push_back(std::make_unique<BeepOnHitComponent>(tComp, &hitManager, &audioContext, &beatClock));
    }

    // Cube2
    Entity* cube2 = entityManager.AddEntity();
    CreateCube(&sceneManager, cubeMesh, cube2);
    {
        TransformComponent* tComp = cube2->DebugFindComponentOfType<TransformComponent>();
        glm::mat4& t = tComp->_transform;
        t = glm::translate(t, glm::vec3(-1.f,0.f,0.f));

        VelocityComponent* v = nullptr;
        {
            auto vComp = std::make_unique<VelocityComponent>(tComp);
            v = vComp.get();
            cube2->_components.push_back(std::move(vComp));
        }
        cube2->_components.push_back(
            std::make_unique<PlayerControllerComponent>(tComp, v, &inputManager, &hitManager));
    }

    // Init event queue with a synth sequence
    {
        // double const audioTime = Pa_GetStreamTime(audioContext._stream);
        // InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
    }

    float lastGlfwTime = (float)glfwGetTime();
    bool showImGuiWindow = true;
    while(!glfwWindowShouldClose(window)) {
        float thisGlfwTime = (float)glfwGetTime();
        float dt = thisGlfwTime - lastGlfwTime;
        lastGlfwTime = thisGlfwTime;

        beatClock.Update();
        if (beatClock.IsNewBeat()) {
            audio::Event e;
            e.type = audio::EventType::PlayPcm;
            e.timeInTicks = beatClock.BeatTimeToTickTime(beatClock.GetDownBeatTime() + 1.0);
            if (!audioContext._eventQueue.try_push(e)) {
                std::cout << "Failed to queue audio event" << std::endl;
            }
        }

        inputManager.Update();

        entityManager.Update(dt);

        if (inputManager.IsKeyPressed(InputManager::Key::Escape)) {
            glfwSetWindowShouldClose(window, true);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (showImGuiWindow) {
            DrawImGuiWindow();
        }
        ImGui::Render();    

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        sceneManager.Draw(windowWidth, windowHeight);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    // NOTE: Do not use BeatClock after shutting down audio.
    if (audio::ShutDown(audioContext) != paNoError) {
        return 1;
    }

    drwav_free(pSampleData, NULL);

    return 0;
}