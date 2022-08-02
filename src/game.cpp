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

void RequestSynthParamChange(int synthIx, audio::SynthParamType const& param, double value, audio::Context& audioContext) {
    audio::Event e;
    e.channel = synthIx;
    e.timeInTicks = 0;
    e.type = audio::EventType::SynthParam;
    e.param = param;
    e.newParamValue = value;
    audioContext.AddEvent(e);
}

void RequestSynthParamChangeInt(int synthIx, audio::SynthParamType const& param, int value, audio::Context& audioContext) {
    audio::Event e;
    e.channel = synthIx;
    e.timeInTicks = 0;
    e.type = audio::EventType::SynthParam;
    e.param = param;
    e.newParamValueInt = value;
    audioContext.AddEvent(e);
}

struct SynthGuiState {
    std::vector<synth::Patch> _synthPatches;
    std::string _saveFilename;
    int _currentSynthIx = -1;
};
void InitSynthGuiState(audio::Context const& audioContext, char const* saveFilename, SynthGuiState& guiState) {
    guiState._synthPatches.clear();
    for (synth::StateData const& synthState : audioContext._state.synths) {
        guiState._synthPatches.push_back(synthState.patch);
    }
    assert(!guiState._synthPatches.empty());
    guiState._currentSynthIx = 0;
    guiState._saveFilename = std::string(saveFilename);
};
void DrawSynthGuiAndUpdatePatch(SynthGuiState& synthGuiState, audio::Context& audioContext) {
    ImGui::Begin("Synth settings");
    {
        // kill sound
        if (ImGui::Button("All notes off")) {
            for (int i = 0; i < audioContext._state.synths.size(); ++i) {
                audio::Event e;
                e.type = audio::EventType::AllNotesOff;
                e.channel = i;
                e.timeInTicks = 0;
                audioContext.AddEvent(e);
            }
        }

        // Saving
        char saveFilenameBuffer[256];
        strcpy(saveFilenameBuffer, synthGuiState._saveFilename.c_str());
        ImGui::InputText("Save filename", saveFilenameBuffer, 256);
        synthGuiState._saveFilename = saveFilenameBuffer;
        if (ImGui::Button("Save")) {
            if (SaveSynthPatches(synthGuiState._saveFilename.c_str(), synthGuiState._synthPatches)) {
                std::cout << "Saved synth patches to \"" << synthGuiState._saveFilename << "\"." << std::endl;
            }
        }
    }

    // TODO: consider caching this.
    std::vector<char const*> listNames;
    listNames.reserve(synthGuiState._synthPatches.size());
    for (auto const& s : synthGuiState._synthPatches) {
        listNames.push_back(s.name.c_str());
    }
    ImGui::ListBox("Synth list", &synthGuiState._currentSynthIx, listNames.data(), /*numItems=*/listNames.size());

    if (synthGuiState._currentSynthIx >= 0) {
        synth::Patch& patch = synthGuiState._synthPatches[synthGuiState._currentSynthIx];

        char nameBuffer[128];
        strcpy(nameBuffer, patch.name.c_str());
        ImGui::InputText("Name", nameBuffer, 128);
        patch.name = nameBuffer;

        bool changed = ImGui::SliderFloat("Gain", &patch.gainFactor, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Gain, patch.gainFactor, audioContext);
        }

        int const numWaveforms = static_cast<int>(synth::Waveform::Count);
        {
            int currentWaveIx = static_cast<int>(patch.osc1Waveform);
            changed = ImGui::Combo("Osc1 Wave", &currentWaveIx, synth::gWaveformStrings, numWaveforms);
            patch.osc1Waveform = static_cast<synth::Waveform>(currentWaveIx);
            if (changed) {
                RequestSynthParamChangeInt(synthGuiState._currentSynthIx, audio::SynthParamType::Osc1Waveform, currentWaveIx, audioContext);
            }
        }

        {
            int currentWaveIx = static_cast<int>(patch.osc2Waveform);
            changed = ImGui::Combo("Osc2 Wave", &currentWaveIx, synth::gWaveformStrings, numWaveforms);
            patch.osc2Waveform = static_cast<synth::Waveform>(currentWaveIx);
            if (changed) {
                RequestSynthParamChangeInt(synthGuiState._currentSynthIx, audio::SynthParamType::Osc2Waveform, currentWaveIx, audioContext);
            }
        }

        changed = ImGui::SliderFloat("Detune", &patch.detune, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Detune, patch.detune, audioContext);
        }

        changed = ImGui::SliderFloat("OscFader", &patch.oscFader, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::OscFader, patch.oscFader, audioContext);
        }

        changed = ImGui::SliderFloat("LPF Cutoff", &patch.cutoffFreq, 0.f, 44100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Cutoff, patch.cutoffFreq, audioContext);
        }

        changed = ImGui::SliderFloat("Peak", &patch.cutoffK, 0.f, 3.99f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::Peak, patch.cutoffK, audioContext);
        }

        changed = ImGui::SliderFloat("PitchLFOGain", &patch.pitchLFOGain, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchLFOGain, patch.pitchLFOGain, audioContext);
        }

        changed = ImGui::SliderFloat("PitchLFOFreq", &patch.pitchLFOFreq, 0.f, 100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::PitchLFOFreq, patch.pitchLFOFreq, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffLFOGain", &patch.cutoffLFOGain, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffLFOGain, patch.cutoffLFOGain, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffLFOFreq", &patch.cutoffLFOFreq, 0.f, 100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffLFOFreq, patch.cutoffLFOFreq, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvAtk", &patch.ampEnvSpec.attackTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvAttack, patch.ampEnvSpec.attackTime, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvDecay", &patch.ampEnvSpec.decayTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvDecay, patch.ampEnvSpec.decayTime, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvSustain", &patch.ampEnvSpec.sustainLevel, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvSustain, patch.ampEnvSpec.sustainLevel, audioContext);
        }

        changed = ImGui::SliderFloat("AmpEnvRelease", &patch.ampEnvSpec.releaseTime, 0.f, 5.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::AmpEnvRelease, patch.ampEnvSpec.releaseTime, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvGain", &patch.cutoffEnvGain, 0.f, 44100.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvGain, patch.cutoffEnvGain, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvAtk", &patch.cutoffEnvSpec.attackTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvAttack, patch.cutoffEnvSpec.attackTime, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvDecay", &patch.cutoffEnvSpec.decayTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvDecay, patch.cutoffEnvSpec.decayTime, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvSustain", &patch.cutoffEnvSpec.sustainLevel, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvSustain, patch.cutoffEnvSpec.sustainLevel, audioContext);
        }

        changed = ImGui::SliderFloat("CutoffEnvRelease", &patch.cutoffEnvSpec.releaseTime, 0.f, 1.f);
        if (changed) {
            RequestSynthParamChange(synthGuiState._currentSynthIx, audio::SynthParamType::CutoffEnvRelease, patch.cutoffEnvSpec.releaseTime, audioContext);
        }
    }
    ImGui::End();
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

void LoadSoundData(std::vector<audio::PcmSound>& sounds) {
    std::vector<char const*> soundFilenames = {
        "data/sounds/kick_deep.wav"
        , "data/sounds/woodblock_reverb_mono.wav"
    };
    for (char const* filename : soundFilenames) {
        audio::PcmSound sound;
        unsigned int numChannels;
        unsigned int sampleRate;
        sound._buffer = drwav_open_file_and_read_pcm_frames_f32(
            filename, &numChannels, &sampleRate, &sound._bufferLength, /*???*/NULL);
        assert(numChannels == 1);
        assert(sampleRate == 44100);
        assert(sound._buffer != nullptr);
        std::cout << filename << ": " << numChannels << " channels, " << sound._bufferLength << " samples." << std::endl;
        sounds.push_back(std::move(sound));
    }
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

int main(int argc, char** argv) {
    CommandLineInputs cmdLineInputs;
    ParseCommandLine(cmdLineInputs, argc, argv);

    std::vector<audio::PcmSound> pcmSounds;
    LoadSoundData(pcmSounds);

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

        if (audio::Init(audioContext, patches, pcmSounds) != paNoError) {
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

    BeatClock beatClock(/*bpm=*/120.0, SAMPLE_RATE, audioContext._stream);
    beatClock.Update();

    InputManager inputManager(window);

    renderer::Scene sceneManager;
    if (!sceneManager.Init()) {
        std::cout << "scene failed to init. exiting" << std::endl;
        return 1;
    }

    EntityManager entityManager;

    CollisionManager collisionManager;

    GameManager gameManager {
        &sceneManager, &inputManager, &audioContext, &entityManager, &collisionManager, &beatClock,
        cmdLineInputs._editMode };

    if (cmdLineInputs._scriptFilename.has_value()) {
        std::cout << "loading " << cmdLineInputs._scriptFilename.value() << std::endl;
        bool dieOnConnectFail = !cmdLineInputs._editMode;
        if (!entityManager.LoadAndConnect(cmdLineInputs._scriptFilename->c_str(), dieOnConnectFail, gameManager)) {
            std::cout << "Load failed. Exiting" << std::endl;
            return 1;
        }
    } else {
        std::cout << "loading hardcoded script" << std::endl;
        LoadTestScript(gameManager);
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
    assert(entityEditingContext.Init(gameManager));
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

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        beatClock.Update();

        {
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
                inputManager.Update();
            }
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
        if (inputManager.IsKeyPressedThisFrame(InputManager::Key::H)) {
            showHitCounters = !showHitCounters;
        }

        entityEditingContext.Update(dt, cmdLineInputs._editMode, gameManager, windowWidth, windowHeight);

        if (cmdLineInputs._editMode) {
            entityManager.EditModeUpdate(dt);
        } else {
            entityManager.Update(dt);
            // TODO do we wanna run collision manager during edit mode?
            collisionManager.Update(dt);
        }        

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

        sceneManager.Draw(windowWidth, windowHeight);

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
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    // NOTE: Do not use BeatClock after shutting down audio.
    if (audio::ShutDown(audioContext) != paNoError) {
        return 1;
    }

    for (audio::PcmSound& sound : pcmSounds) {
        drwav_free(sound._buffer, /*???*/NULL);
    }

    return 0;
}