#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <unordered_map>
#include <optional>

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
#include "game_manager.h"
#include "resource_manager.h"
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
#include "entity_loader.h"
#include "audio_loader.h"
#include "entity_editing_context.h"

#include "test_script.h"

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
        e.param = audio::SynthParamType::Cutoff;
        e.newParamValue = i * (1.f / 16.f);
        queue->push(e);
    }
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// TODO: Maybe just use polymorphism instead of this awkward shit lol. Synth
// params stay constant, you only do the allocations once.
struct SynthParamSpec {
    char const* _name;
    float _minValue;
    float _maxValue;
};
SynthParamSpec GetSynthParamSpec(audio::SynthParamType param) {
    switch (param) {
        case audio::SynthParamType::Gain:
            return SynthParamSpec {"Gain", 0.f, 1.f };
        case audio::SynthParamType::Cutoff:
            return SynthParamSpec { "LPF Cutoff", 0.f, 1.f };
        case audio::SynthParamType::Peak:
            return SynthParamSpec { "LPF Peak", 0.f, 0.99f };
        case audio::SynthParamType::PitchLFOGain:
            return SynthParamSpec { "Pitch LFO Gain", 0.f, 1.f };
        case audio::SynthParamType::PitchLFOFreq:
            return SynthParamSpec { "Pitch LFO Freq", 0.f, 30.f };
        case audio::SynthParamType::CutoffLFOGain:
            return SynthParamSpec { "Cutoff LFO Gain", 0.f, 1.f };
        case audio::SynthParamType::CutoffLFOFreq:
            return SynthParamSpec { "Cutoff LFO Freq", 0.f, 30.f };
        case audio::SynthParamType::AmpEnvAttack:
            return SynthParamSpec { "Cutoff Env Atk", 0.f, 1.f };
        case audio::SynthParamType::AmpEnvDecay:
            return SynthParamSpec { "Amp Env Decay", 0.f, 1.f };
        case audio::SynthParamType::AmpEnvSustain:
            return SynthParamSpec { "Amp Env Sus", 0.f, 1.f };
        case audio::SynthParamType::AmpEnvRelease:
            return SynthParamSpec { "Amp Env Rel", 0.f, 1.f };    
        case audio::SynthParamType::CutoffEnvGain:
            return SynthParamSpec { "Cutoff Env Gain", 0.f, 20000.f };
        case audio::SynthParamType::CutoffEnvAttack:
            return SynthParamSpec { "Cutoff Env Atk", 0.f, 1.f };
        case audio::SynthParamType::CutoffEnvDecay:
            return SynthParamSpec { "Cutoff Env Decay", 0.f, 1.f };
        case audio::SynthParamType::CutoffEnvSustain:
            return SynthParamSpec { "Cutoff Env Sus", 0.f, 1.f };
        case audio::SynthParamType::CutoffEnvRelease:
            return SynthParamSpec { "Cutoff Env Rel", 0.f, 1.f };
        default:
            return SynthParamSpec { "UNSUPPORTED", 0.f, 0.f };
    }
}
struct SynthParam {
    audio::SynthParamType _param;
    float _currentValue;
    float _prevValue;    
};
struct SynthPatch {
    static int constexpr kNumParams = static_cast<int>(audio::SynthParamType::Count);
    std::string _name;
    std::array<SynthParam, kNumParams> _params;
};
void InitSynthPatch(
    SynthPatch& patch, audio::StateData const& audioState, int synthIx, int sampleRate,
    char const* name) {
    synth::Patch const& synthPatch = audioState.synths[synthIx].patch;

    patch._name = name;
    int paramIx = 0;
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::Gain;
        p._currentValue = p._prevValue = synthPatch.gainFactor;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::Cutoff;
        p._currentValue = p._prevValue = std::min(1.f, synthPatch.cutoffFreq / (float)sampleRate);
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::Peak;
        p._currentValue = p._prevValue = std::min(1.f, synthPatch.cutoffK / 4.f);
    }
    {
        // When this is 0, variation is 0. when this is 1, the pitch varies up and down by a whole octave (so a full spread of 2 octaves)
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::PitchLFOGain;
        p._currentValue = p._prevValue = synthPatch.pitchLFOGain;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::PitchLFOFreq;
        p._currentValue = p._prevValue = synthPatch.pitchLFOFreq;
    }
    {
        // When this is 0, variation is 0. when this is 1, the cutoff varies up
        // and down by a whole octave (so cutoff freq goes from 0.5*cutoff to
        // 2.0*cutoff)
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffLFOGain;
        p._currentValue = p._prevValue = synthPatch.cutoffLFOGain;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffLFOFreq;
        p._currentValue = p._prevValue = synthPatch.cutoffLFOFreq;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::AmpEnvAttack;
        p._currentValue = p._prevValue = synthPatch.ampEnvSpec.attackTime;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::AmpEnvDecay;
        p._currentValue = p._prevValue = synthPatch.ampEnvSpec.decayTime;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::AmpEnvSustain;
        p._currentValue = p._prevValue = synthPatch.ampEnvSpec.sustainLevel;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::AmpEnvRelease;
        p._currentValue = p._prevValue = synthPatch.ampEnvSpec.releaseTime;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffEnvGain;
        p._currentValue = p._prevValue = synthPatch.cutoffEnvGain;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffEnvAttack;
        p._currentValue = p._prevValue = synthPatch.cutoffEnvSpec.attackTime;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffEnvDecay;
        p._currentValue = p._prevValue = synthPatch.cutoffEnvSpec.decayTime;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffEnvSustain;
        p._currentValue = p._prevValue = synthPatch.cutoffEnvSpec.sustainLevel;
    }
    {
        SynthParam& p = patch._params[paramIx++];
        p._param = audio::SynthParamType::CutoffEnvRelease;
        p._currentValue = p._prevValue = synthPatch.cutoffEnvSpec.releaseTime;
    }
    // Give the rest of the params an invalid value for now.
    for (; paramIx < SynthPatch::kNumParams; ++paramIx) {
        SynthParam& p = patch._params[paramIx];
        p._param = audio::SynthParamType::Count;
        p._currentValue = p._prevValue = 0.f;
    }
}

struct SynthGuiState {
    int _activeSynthIx = 0;
    std::array<char const*, audio::kNumSynths> _synthListItems;
    std::array<SynthPatch, audio::kNumSynths> _synths;
};
void InitSynthGuiState(SynthGuiState& guiState, audio::StateData const& audioState, int sampleRate) {
    {
        SynthPatch& patch = guiState._synths[0];
        InitSynthPatch(patch, audioState, /*synthIx=*/0, sampleRate, "beep");
    }
    {
        SynthPatch& patch = guiState._synths[1];
        InitSynthPatch(patch, audioState, /*synthIx=*/1, sampleRate, "drone");
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
        // skip unsupported params
        if (param._param == audio::SynthParamType::Count) {
            continue;
        }
        SynthParamSpec const spec = GetSynthParamSpec(param._param);
        ImGui::SliderFloat(
            spec._name, &param._currentValue, spec._minValue, spec._maxValue);
        if (param._currentValue != param._prevValue) {
            RequestSynthParamChange(state._activeSynthIx, param, audioContext);
            param._prevValue = param._currentValue;
        }
    }
    ImGui::End();
}

int main(int argc, char** argv) {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        "../data/sounds/kick.wav", &channels, &sampleRate, &totalPCMFrameCount, NULL);
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
    if (!shaderProgram.Init("../shaders/shader.vert", "../shaders/shader.frag")) {
        return 1;
    }

    std::unique_ptr<Texture> texture =
        Texture::CreateTextureFromFile("../data/textures/wood_container.jpg");
    if (texture == nullptr) {
        return 1;
    }

    Material material;
    material._shader = shaderProgram;
    material._texture = texture.get();

    ModelManager modelManager;

    {
        std::array<float,kCubeVertsNumValues> cubeVerts;
        GetCubeVertices(&cubeVerts);
        int const numCubeVerts = 36;
        auto mesh = std::make_unique<BoundMesh>();
        mesh->Init(cubeVerts.data(), numCubeVerts, &material);
        assert(modelManager._modelMap.emplace("wood_box", std::move(mesh)).second);
    }

    BeatClock beatClock(/*bpm=*/120.0, (unsigned int)sampleRate, audioContext._stream);
    beatClock.Update();

    InputManager inputManager(window);

    SceneManager sceneManager;

    EntityManager entityManager;

    CollisionManager collisionManager;

    GameManager gameManager {
        &sceneManager, &inputManager, &audioContext, &entityManager, &collisionManager, &modelManager, &beatClock };

    std::optional<std::string> scriptFilename;
    std::optional<std::string> saveFilename;
    bool editMode = false;
    for (int argIx = 1; argIx < argc; ++argIx) {
        if (strcmp(argv[argIx], "-f") == 0) {
            ++argIx;
            if (argIx >= argc) {
                std::cout << "Need a filename with -f. Using hardcoded script." << std::endl;
                continue;
            }
            scriptFilename = argv[argIx];
        } else if (strcmp(argv[argIx], "-s") == 0) {
            ++argIx;
            if (argIx >= argc) {
                std::cout << "need a filename with -s. Not saving." << std::endl;
                continue;
            }
            saveFilename = argv[argIx];
        } else if (strcmp(argv[argIx], "-e") == 0) {
            std::cout << "Edit mode enabled!" << std::endl;
            editMode = true;            
        }
    }

    if (scriptFilename.has_value()) {
        std::cout << "loading " << scriptFilename.value() << std::endl;
        if (!LoadEntities(scriptFilename->c_str(), entityManager, gameManager)) {
            std::cout << "Load failed. Exiting" << std::endl;
            return 1;
        }
    } else {
        std::cout << "loading hardcoded script" << std::endl;
        LoadTestScript(gameManager);
    }

    if (saveFilename.has_value()) {
        std::cout << "saving to " << saveFilename.value() << std::endl;
        SaveEntities(saveFilename->c_str(), entityManager);
    }

    // SAVE/LOAD SYNTH SETTINGS
    SaveSynthPatch("tmp/synth_patch.xml", audioContext._state.synths[0].patch);    

    SynthGuiState synthGuiState;
    InitSynthGuiState(synthGuiState, audioContext._state, sampleRate);

    bool showSynthWindow = false;
    bool showEntitiesWindow = false;
    bool showDemoWindow = false;
    float const fixedTimeStep = 1.f / 60.f;
    bool paused = false;
    EntityEditingContext entityEditingContext;
    while(!glfwWindowShouldClose(window)) {

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

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
        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::E)) {
            showEntitiesWindow = !showEntitiesWindow;
        }
        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Space)) {
            showDemoWindow = !showDemoWindow;
        }

        if (!editMode) {
            entityManager.Update(dt);
            collisionManager.Update(dt);
        }

        entityEditingContext.Update(dt, editMode, gameManager, windowWidth, windowHeight);

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
        if (showEntitiesWindow) {
            entityEditingContext.DrawEntitiesWindow(entityManager, gameManager);
        }
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }
        ImGui::Render();    

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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