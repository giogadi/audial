#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <unordered_map>
#include <optional>
#include <sstream>

#include <portaudio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "constants.h"
#include "game_manager.h"
#include "audio.h"
#include "beat_clock.h"
#include "input_manager.h"
#include "collisions.h"
#include "component.h"
#include "mesh.h"
#include "renderer.h"
#include "components/player_controller.h"
#include "components/beep_on_hit.h"
#include "components/sequencer.h"
#include "components/rigid_body.h"
#include "components/hit_counter.h"
#include "audio_loader.h"
#include "entity_editing_context.h"
#include "editor.h"
#include "sound_bank.h"
#include "synth_imgui.h"

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

void ShowHitCounterWindow(EntityManager& entityMgr) {
    ImGui::Begin("Hit counters");
    entityMgr.ForEveryActiveEntity([&entityMgr](EntityId id) {
        Entity& entity = *entityMgr.GetEntity(id);
        std::shared_ptr<HitCounterComponent> hitComp = entity.FindComponentOfType<HitCounterComponent>().lock();
        if (hitComp) {
            ImGui::Text("%s: %d", entity._name.c_str(), hitComp->_hitsRemaining);
        }
    });
    ImGui::End();
}

struct CommandLineInputs {
    std::optional<std::string> _scriptFilename;
    std::optional<std::string> _synthPatchesFilename;
    bool _editMode = false;
};
void ParseCommandLine(CommandLineInputs& inputs, std::vector<std::string> const& argv) {
    inputs._editMode = false;
    for (int argIx = 0; argIx < argv.size(); ++argIx) {
        if (argv[argIx] == "-f") {
            ++argIx;
            if (argIx >= argv.size()) {
                std::cout << "Need a command line filename with -f. Ignoring." << std::endl;
                continue;
            }
            std::vector<std::string> fileArgv;
            {
                std::ifstream cmdLineFile(argv[argIx]);
                if (!cmdLineFile.is_open()) {
                    std::cout << "Tried to open command line file \"" << argv[argIx] <<
                        "\", but could not open file. Skipping." << std::endl;
                    continue;
                }
                // read from the cmdline file line-by-line so we can filter out // comments
                while (!cmdLineFile.eof()) {
                    std::string line;
                    std::getline(cmdLineFile, line);
                    if (line.size() >= 2 && line[0] == '/' && line[1] == '/') {
                        // comment. skip line.
                        continue;
                    }

                    std::stringstream ss(line);
                    ss.str(line);
                    while (!ss.eof()) {
                        std::string arg;
                        ss >> arg;
                        fileArgv.push_back(std::move(arg));
                    }
                }
            }
            ParseCommandLine(inputs, fileArgv);
        } else if (argv[argIx] == "-s") {
            ++argIx;
            if (argIx >= argv.size()) {
                std::cout << "Need a script filename with -s. Using hardcoded script." << std::endl;
                continue;
            }
            inputs._scriptFilename = argv[argIx];
        } else if (argv[argIx] == "-y") {
            ++argIx;
            if (argIx >= argv.size()) {
                std::cout << "need a synth patch filename with -y. Using hardcoded synth patches" << std::endl;
                continue;
            }
            inputs._synthPatchesFilename = argv[argIx];
        } else if (argv[argIx] == "-e") {
            std::cout << "Edit mode enabled!" << std::endl;
            inputs._editMode = true;
        }
    }
}

void ParseCommandLine(CommandLineInputs& inputs, int argc, char** argv) {
    std::vector<std::string> argVec;
    argVec.reserve(argc - 1);  // ignore first parameter
    for (int i = 1; i < argc; ++i) {
        argVec.push_back(argv[i]);
    }
    ParseCommandLine(inputs, argVec);
}

GameManager gGameManager;

int main(int argc, char** argv) {
    CommandLineInputs cmdLineInputs;
    ParseCommandLine(cmdLineInputs, argc, argv);

    SoundBank soundBank;
    soundBank.LoadSounds();

    // Init audio
    audio::Context audioContext;
    {
        // Load in synth patch data if we have it
        std::vector<synth::Patch> patches;
        if (cmdLineInputs._synthPatchesFilename.has_value()) {
            if (LoadSynthPatches(cmdLineInputs._synthPatchesFilename->c_str(), patches)) {
                std::cout << "Loaded synth patch data from \"" << *cmdLineInputs._synthPatchesFilename << "\"." << std::endl;
            }
        }

        if (audio::Init(audioContext, patches, soundBank) != paNoError) {
            return 1;
        }
    }

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

    InputManager inputManager(window);

    renderer::Scene sceneManager;

    EntityManager entityManager;
    ne::EntityManager neEntityManager;

    CollisionManager collisionManager;

    BeatClock beatClock;

    gGameManager = GameManager {
        &sceneManager, &inputManager, &audioContext, &entityManager, &neEntityManager, &collisionManager, &beatClock, &soundBank,
        cmdLineInputs._editMode };

    if (!sceneManager.Init(gGameManager)) {
        std::cout << "scene failed to init. exiting" << std::endl;
        return 1;
    }

    neEntityManager.Init();

    Editor editor;
    editor.Init(&gGameManager);    

    // SCRIPT LOADING
    if (cmdLineInputs._scriptFilename.has_value()) {
        std::cout << "loading " << cmdLineInputs._scriptFilename.value() << std::endl;
        bool dieOnConnectFail = !cmdLineInputs._editMode;
        serial::Ptree pt = serial::Ptree::MakeNew();
        if (!pt.LoadFromFile(cmdLineInputs._scriptFilename->c_str())) {
            printf("Script ptree load failed. Exiting.\n");
            pt.DeleteData();
            return 1;
        }

        double bpm = pt.GetDouble("script.bpm");
        beatClock.Init(bpm, SAMPLE_RATE, audioContext._stream);
        beatClock.Update();

        {
            // New entity loading. Manually add them in this order to the editor to keep ordering consistent.
            // TODO: make this less shit
            editor._saveFilename = *cmdLineInputs._scriptFilename;
            serial::Ptree entitiesPt = pt.GetChild("script.new_entities");
            int numEntities = 0;
            serial::NameTreePair* children = entitiesPt.GetChildren(&numEntities);
            for (int i = 0; i < numEntities; ++i) {
                ne::EntityType entityType = ne::StringToEntityType(children[i]._name);
                ne::Entity* entity = gGameManager._neEntityManager->AddEntity(entityType);
                entity->Load(children[i]._pt);
                editor._entityIds.push_back(entity->_id);
            }
            delete[] children;

            // Now go through and initialize everything.
            for (auto iter = gGameManager._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
                iter.GetEntity()->Init(gGameManager);
            }
        }

        serial::Ptree entitiesPt = pt.TryGetChild("script.entities");
        if (entitiesPt.IsValid()) {
            if (!entityManager.LoadAndConnect(entitiesPt, dieOnConnectFail, gGameManager)) {
                printf("Entities load failed. Exiting.\n");
                return 1;
            }
        } else {
            printf("No old-school entities!\n");
        }

        pt.DeleteData();
    } else {
        std::cout << "loading hardcoded script" << std::endl;
        beatClock.Init(/*bpm=*/120.0, /*sampleRate=*/SAMPLE_RATE, audioContext._stream);
        beatClock.Update();
        LoadTestScript(gGameManager);
    }

    SynthGuiState synthGuiState;
    {
        char const* filename = "";
        if (cmdLineInputs._synthPatchesFilename.has_value()) {
            filename = cmdLineInputs._synthPatchesFilename->c_str();
        }
        InitSynthGuiState(audioContext, filename, synthGuiState);
    }

    EntityEditingContext entityEditingContext;
    assert(entityEditingContext.Init(gGameManager));
    if (cmdLineInputs._scriptFilename.has_value()) {
        entityEditingContext._saveFilename = *cmdLineInputs._scriptFilename;
    }    

    bool showSynthWindow = false;
    bool showEntitiesWindow = false;
    bool showDemoWindow = false;
    bool showHitCounters = false;
    float const fixedTimeStep = 1.f / 60.f;
    bool paused = false;
    while(!glfwWindowShouldClose(window)) {

        glfwGetWindowSize(window, &gGameManager._windowWidth, &gGameManager._windowHeight);

        beatClock.Update();

        {
            ImGuiIO& io = ImGui::GetIO();
            bool inputEnabled = !io.WantCaptureMouse && !io.WantCaptureKeyboard;
            inputManager.Update(inputEnabled);
        }

        if (!cmdLineInputs._editMode && inputManager.IsKeyPressedThisFrame(InputManager::Key::Space)) {
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
        if (/*editMode &&*/ inputManager.IsKeyPressedThisFrame(InputManager::Key::E)) {
            showEntitiesWindow = !showEntitiesWindow;
        }
        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::Space)) {
            showDemoWindow = !showDemoWindow;
        }
        // if (inputManager.IsKeyPressedThisFrame(InputManager::Key::H)) {
        //     showHitCounters = !showHitCounters;
        // }

        entityEditingContext.Update(dt, cmdLineInputs._editMode, gGameManager);        

        if (cmdLineInputs._editMode) {
            entityManager.EditModeUpdate(dt);
        } else {
            entityManager.Update(dt);
            // TODO do we wanna run collision manager during edit mode?
            collisionManager.Update(dt);
        }

        for (auto iter = gGameManager._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
            iter.GetEntity()->Update(gGameManager, dt);
        }

        if (inputManager.IsKeyPressed(InputManager::Key::Escape)) {
            glfwSetWindowShouldClose(window, true);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        editor.Update(dt);
        if (showSynthWindow) {
            DrawSynthGuiAndUpdatePatch(synthGuiState, audioContext);
        }
        if (showEntitiesWindow) {
            entityEditingContext.DrawEntitiesWindow(entityManager, gGameManager);
        }        
        if (showHitCounters) {
            ShowHitCounterWindow(entityManager);
        }
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }
        ImGui::Render();

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float timeInSecs = (float) glfwGetTime();
        sceneManager.Draw(gGameManager._windowWidth, gGameManager._windowHeight, timeInSecs);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // We're doing this last so that it's not possible for the following to happen:
        // 1. player hits planet with 1hp
        // 2. planet gets deleted
        // 3. player's position is wacky
        // 4. renderer takes wacky position
        //
        // Rule here is: We ALWAYS have at least one frame to deal with a deleted reference before a render happens. Not a bad rule!
        entityManager.DestroyTaggedEntities();
        neEntityManager.DestroyTaggedEntities(gGameManager);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    // NOTE: Do not use BeatClock after shutting down audio.
    if (audio::ShutDown(audioContext) != paNoError) {
        return 1;
    }

    soundBank.Destroy();

    return 0;
}
