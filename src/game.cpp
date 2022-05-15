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

#include "stb_image.h"

#include "constants.h"
#include "audio.h"
#include "beat_clock.h"
#include "shader.h"
#include "cube_verts.h"
#include "input_manager.h"
#include "collisions.h"
#include "component.h"
#include "model.h"
#include "renderer.h"
#include "components/player_controller.h"
#include "components/beep_on_hit.h"
#include "components/sequencer.h"
#include "components/rigid_body.h"

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

void InitEventQueueWithParamSequence(audio::EventQueue* queue, BeatClock const& beatClock) {
    double const firstNoteBeatTime = beatClock.GetDownBeatTime() + 1.0;
    // gradually increase the cutoff from 0 to 1 in 4 measures = 16 beats.
    for (int i = 0; i < 16; ++i) {
        double const beatTime = firstNoteBeatTime + (double)i;
        unsigned long const tickTime = beatClock.BeatTimeToTickTime(beatTime);

        audio::Event e;
        e.channel = 1;
        e.type = audio::EventType::SynthParam;
        e.timeInTicks = tickTime;
        e.param = audio::ParamType::Cutoff;
        e.newParamValue = i * (1.f / 16.f);
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
    e->_components.push_back(std::make_unique<LightComponent>(t, Vec3(0.2f,0.2f,0.2f), Vec3(1.f,1.f,1.f), sceneMgr));
}

void CreateCamera(SceneManager* sceneMgr, InputManager* inputMgr, Entity* e) {
    auto unique = std::make_unique<TransformComponent>();
    TransformComponent* t = unique.get();
    e->_components.push_back(std::move(unique));
    e->_components.push_back(std::make_unique<CameraComponent>(t, inputMgr, sceneMgr));
}

void CreateDrone(audio::Context* audio, BeatClock const* beatClock, Entity* e) {
    auto seqComp = std::make_unique<SequencerComponent>(audio, beatClock);
    {
        audio::Event e;
        e.channel = 1;
        e.type = audio::EventType::NoteOn;
        e.midiNote = 45;
        e.timeInTicks = 0;
        seqComp->AddToSequence(e);

        e.midiNote = 49;
        seqComp->AddToSequence(e);

        e.midiNote = 52;
        seqComp->AddToSequence(e);
    }
    e->_components.push_back(std::move(seqComp));
}

// TODO: Maybe just use polymorphism instead of this awkward shit lol. Synth
// params stay constant, you only do the allocations once.
char const* GetSynthParamName(audio::ParamType param) {
    switch (param) {
        case audio::ParamType::Cutoff:
            return "LPF Cutoff";
    }
}
float GetSynthParamMinValue(audio::ParamType param) {
    switch (param) {
        case audio::ParamType::Cutoff:
            return 0.f;
    }
}
float GetSynthParamMaxValue(audio::ParamType param) {
    switch (param) {
        case audio::ParamType::Cutoff:
            return 1.f;
    }
}
struct SynthParam {
    audio::ParamType _param;
    float _currentValue;
    float _prevValue;    
};
struct SynthPatch {
    static int constexpr kNumParams = 1;
    std::string _name;
    std::array<SynthParam, kNumParams> _params;
};

struct SynthGuiState {
    int _activeSynthIx = 0;
    std::array<char const*, audio::kNumSynths> _synthListItems;
    std::array<SynthPatch, audio::kNumSynths> _synths;
};
void InitSynthGuiState(SynthGuiState& guiState, audio::StateData const& audioState, int sampleRate) {
    {
        SynthPatch& patch = guiState._synths[0];
        patch._name = "beep";
        SynthParam& p = patch._params[0];
        p._param = audio::ParamType::Cutoff;
        p._currentValue = p._prevValue = std::max(1.f, audioState.synths[0].cutoffFreq / (float)sampleRate);
    }
    {
        SynthPatch& patch = guiState._synths[1];
        patch._name = "drone";
        SynthParam& p = patch._params[0];
        p._param = audio::ParamType::Cutoff;
        p._currentValue = p._prevValue = std::max(1.f, audioState.synths[1].cutoffFreq / (float)sampleRate);
    }

    for (int i = 0; i < audio::kNumSynths; ++i) {
        guiState._synthListItems[i] = guiState._synths[i]._name.c_str();
    }
}

void RequestSynthParamChange(int synthIx, SynthParam const& param, audio::Context& audioContext) {
    audio::Event e;
    e.channel = synthIx;
    e.timeInTicks = 0;
    e.type = audio::EventType::SynthParam;
    e.param = param._param;
    e.newParamValue = param._currentValue;
    if (!audioContext._eventQueue.try_push(e)) {
        std::cout << "Failed to push synth param change!" << std::endl;
    }
}

// TODO weird that this also requests synth param changes.
void DrawSynthGuiAndUpdatePatch(SynthGuiState& state, audio::Context& audioContext) {
    ImGui::Begin("Synth settings");
    // TODO: demo adds one more argument to this. what does it do?
    ImGui::ListBox("Synth list", &state._activeSynthIx, state._synthListItems.data(), /*numItems=*/2);
    SynthPatch& patch = state._synths[state._activeSynthIx];
    for (int paramIx = 0; paramIx < SynthPatch::kNumParams; ++paramIx) {
        SynthParam& param = patch._params[paramIx];
        ImGui::SliderFloat(
            GetSynthParamName(param._param), &param._currentValue,
            GetSynthParamMinValue(param._param), GetSynthParamMaxValue(param._param));
        if (param._currentValue != param._prevValue) {
            RequestSynthParamChange(state._activeSynthIx, param, audioContext);
            param._prevValue = param._currentValue;
        }
    }
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

    CollisionManager collisionManager;

    // Camera
    Entity* camera = entityManager.AddEntity();
    CreateCamera(&sceneManager, &inputManager, camera);
    {
        TransformComponent* t = camera->DebugFindComponentOfType<TransformComponent>();
        float angle = 45.f * kDeg2Rad;
        Vec3 dir(0.f, sin(angle), cos(angle));
        float dist = 15.f;
        t->SetPos(dist * dir);
        Mat3 rot = Mat3::FromAxisAngle(Vec3(1.f, 0.f, 0.f), -angle);
        t->SetRot(rot);
        
    }

    // Light
    Entity* light = entityManager.AddEntity();
    CreateLight(&sceneManager, light);
    {
        TransformComponent* t = light->DebugFindComponentOfType<TransformComponent>();
        t->SetPos(Vec3(0.f, 3.f, 0.f));
    }

    // Cube1
    Entity* cube1 = entityManager.AddEntity();
    CreateCube(&sceneManager, cubeMesh, cube1);
    {
        TransformComponent* tComp = cube1->DebugFindComponentOfType<TransformComponent>();
        auto beepComp = std::make_unique<BeepOnHitComponent>(tComp, &audioContext, &beatClock);
        auto rbComp = std::make_unique<RigidBodyComponent>(tComp, &collisionManager, MakeCubeAabb(0.5f));
        rbComp->SetOnHitCallback(std::bind(&BeepOnHitComponent::OnHit, beepComp.get()));
        auto velComp = std::make_unique<VelocityComponent>(tComp);
        velComp->_angularY = 2*kPi;
        cube1->_components.push_back(std::move(beepComp));
        cube1->_components.push_back(std::move(rbComp));
        cube1->_components.push_back(std::move(velComp));
    }

    // Cube2
    Entity* cube2 = entityManager.AddEntity();
    CreateCube(&sceneManager, cubeMesh, cube2);
    {
        TransformComponent* tComp = cube2->DebugFindComponentOfType<TransformComponent>();
        Mat4& t = tComp->_transform;
        t.Translate(Vec3(-2.f,0.f,0.f));

        auto rbComp = std::make_unique<RigidBodyComponent>(tComp, &collisionManager, MakeCubeAabb(0.5f));
        rbComp->_static = false;
        auto controller = std::make_unique<PlayerControllerComponent>(tComp, rbComp.get(), &inputManager);
        rbComp->SetOnHitCallback(std::bind(&PlayerControllerComponent::OnHit, controller.get()));

        cube2->_components.push_back(std::move(controller));
        cube2->_components.push_back(std::move(rbComp));
    }

    // drone sequencer
    Entity* droneSeq = entityManager.AddEntity();
    CreateDrone(&audioContext, &beatClock, droneSeq);

    // Init event queue with a synth sequence
    {
        // double const audioTime = Pa_GetStreamTime(audioContext._stream);
        // InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
        // InitEventQueueWithParamSequence(&audioContext._eventQueue, beatClock);
    }

    SynthGuiState synthGuiState;
    InitSynthGuiState(synthGuiState, audioContext._state, sampleRate);

    bool showSynthWindow = false;
    float const fixedTimeStep = 1.f / 60.f;
    bool paused = false;
    while(!glfwWindowShouldClose(window)) {

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

        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Space)) {
            paused = !paused;
        }

        float dt = 0.f;
        if (paused) {
            if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Right)) {
                dt = fixedTimeStep;
            } else {
                dt = 0.f;
            }
        } else {
            dt = fixedTimeStep;
        }

        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Y)) {
            showSynthWindow = !showSynthWindow;
        }

        entityManager.Update(dt);

        collisionManager.Update(dt);

        if (inputManager.IsKeyPressed(InputManager::Key::Escape)) {
            glfwSetWindowShouldClose(window, true);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (showSynthWindow) {
            DrawSynthGuiAndUpdatePatch(synthGuiState, audioContext);
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