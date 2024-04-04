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

#if defined __APPLE__
#include <mach-o/dyld.h>
#include <unistd.h>
#endif  /* __APPLE__ */

#include <portaudio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "features.h"

#if COMPUTE_FFT
#include <fftw3.h>
#endif

#include "constants.h"
#include "game_manager.h"
#include "audio.h"
#include "beat_clock.h"
#include "input_manager.h"
// #include "collisions.h"
// #include "component.h"
#include "mesh.h"
#include "renderer.h"
// #include "components/player_controller.h"
// #include "components/beep_on_hit.h"
// #include "components/sequencer.h"
// #include "components/rigid_body.h"
// #include "components/hit_counter.h"
// #include "entity_editing_context.h"
#include "editor.h"
#include "sound_bank.h"
#include "synth_imgui.h"
#include "logger.h"

GameManager gGameManager;

double* gDft = nullptr;
int gDftCount = 0;

double* gDftLogAvgs = nullptr;
double* gDftLogAvgsMaxFreqs = nullptr;
int gDftLogAvgsCount = 0;


void SetViewport(ViewportInfo const& viewport) {
    glViewport(viewport._offsetX, viewport._offsetY, viewport._width, viewport._height);
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Update viewport with new window size, but don't update GameManager just yet just to avoid potential multhreading shit
    int widthScreen, heightScreen;
    glfwGetWindowSize(window, &widthScreen, &heightScreen);
    ViewportInfo viewport = CalculateViewport(gGameManager._aspectRatio, width, height, widthScreen, heightScreen);
    SetViewport(viewport);
}

// TODO: why do we need to call this when window moves? makes no sense
void WindowPosCallback(GLFWwindow* window, int xpos, int ypos) {
    SetViewport(gGameManager._viewportInfo);
}

void ToggleMute(float dt) {
    static bool sMute = false;
    sMute = !sMute;
    static float sCurrentGain = 1.f;
    float targetGain;
    if (sMute) {
        targetGain = 0.f;
    } else {
        targetGain = 1.f;
    }
    float constexpr kFactor = 6.f;
    sCurrentGain = sCurrentGain + kFactor * (targetGain - sCurrentGain);
    
}

struct CommandLineInputs {
    std::optional<std::string> _scriptFilename;
    std::optional<std::string> _synthPatchesFilename;
    float _gain = 1.f;
    bool _editMode = false;
    bool _drawTerrain = false;
};

void ParseCommandLine(CommandLineInputs& inputs, std::vector<std::string> const& argv, bool useDefaultFile);

void ParseCommandLineFile(CommandLineInputs& inputs, std::string fileName) {
    std::vector<std::string> fileArgv;
    std::ifstream cmdLineFile(fileName.c_str());
    if (!cmdLineFile.is_open()) {
        std::cout << "Tried to open command line file \"" << fileName <<
            "\", but could not open file. Skipping." << std::endl;
        return;
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
    ParseCommandLine(inputs, fileArgv, false);
}

bool gRandomLetters = false;

void ParseCommandLine(CommandLineInputs& inputs, std::vector<std::string> const& argv, bool useDefaultFile = true) {
    inputs._editMode = false;

    // Start off args from a default cmd line file
    if (useDefaultFile) {
        ParseCommandLineFile(inputs, "cmd_line.txt");
    }
    
    for (int argIx = 0; argIx < argv.size(); ++argIx) {
        if (argv[argIx] == "-f") {
            ++argIx;
            if (argIx >= argv.size()) {
                std::cout << "Need a command line filename with -f. Ignoring." << std::endl;
                continue;
            }
            ParseCommandLineFile(inputs, argv[argIx]);
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
        } else if (argv[argIx] == "-r") {
            std::cout << "Random enemy letters!" << std::endl;
            gRandomLetters = true;
        } else if (argv[argIx] == "-g") {
            ++argIx;
            if (argIx >= argv.size()) {
                std::cout << "Expected a float argument to -g" << std::endl;
                continue;
            }
            std::string gainValueStr = argv[argIx];
            try {
                inputs._gain = std::stof(gainValueStr);
            } catch (std::exception& e) {
                std::cout << "-g: Failed to parse \"" << gainValueStr << "\" as a float." << std::endl;
            }
        } else if (argv[argIx] == "-t") {
            inputs._drawTerrain = true;
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

void MaybeToggleMute(float dt) {
    static bool sMute = false;
    static bool sRamping = false;
    if (gGameManager._inputManager->IsKeyPressedThisFrame(InputManager::Key::Backspace)) {
        sMute = !sMute;
        sRamping = true;
    }
    if (sRamping) {
        static float sCurrentGain = 1.f;
        float targetGain;
        if (sMute) {
            targetGain = 0.f;
        } else {
            targetGain = 1.f;
        }
        if (std::abs(targetGain - sCurrentGain) < 0.001f) {
            sCurrentGain = targetGain;
            sRamping = false;
        } else {
            float constexpr kFactor = 10.f;
            sCurrentGain += kFactor * dt * (targetGain - sCurrentGain);
        }
        audio::Event e;
        e.type = audio::EventType::SetGain;
        e.newGain = sCurrentGain;
        gGameManager._audioContext->AddEvent(e);
    }
}

void ShutDown(audio::Context& audioContext, SoundBank& soundBank) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    // NOTE: Do not use BeatClock after shutting down audio.
    PaError err = audio::ShutDown(audioContext);
    if (err != paNoError) {
        printf("Error in shutting down audio! error: %s\n", Pa_GetErrorText(err));
    }   
    soundBank.Destroy();
}

int main(int argc, char** argv) {

    logger::Logger logger;
    
#if defined __APPLE__
{
    FILE* f = fopen("cmd_line.txt", "r");
    if (f == nullptr) {
        // Try changing directories to executable directory
        char executablePath[1024];
        uint32_t pathSize = sizeof(executablePath);
        _NSGetExecutablePath(executablePath, &pathSize);
        pathSize = strlen(executablePath);
        int currentIx = pathSize - 1;
        while (currentIx >= 0) {
            if (executablePath[currentIx] == '/') {
                break;
            }
            --currentIx;
        }
        if (currentIx < 0) {
            printf("Invalid executable path: %s\n", executablePath);
            return 1;
        }
        executablePath[currentIx] = '\0';
        chdir(executablePath);

        char buffer[1024];
        uint32_t bufferSize = sizeof(buffer);
        getcwd(buffer, bufferSize);
        printf("pwd: %s\n", buffer);
    } else {
        fclose(f);
    }   
 }
#endif  /* __APPLE__ */    
    
    CommandLineInputs cmdLineInputs;
    ParseCommandLine(cmdLineInputs, argc, argv);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window;
    int refreshRate = 0;
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        GLFWvidmode const* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        // Full screen
        // window = glfwCreateWindow(mode->width, mode->height, "Audial", monitor, NULL);

        int width = (int)(0.9f * mode->width);
        int height = (int)(0.9f * mode->height);
        window = glfwCreateWindow(width, height, "Audial", NULL, NULL);
        {
            // Place window at top-left of screen
            int leftDist, topDist, rightDist, bottomDist;
            glfwGetWindowFrameSize(window, &leftDist, &topDist, &rightDist, &bottomDist);
            int xPos = 0 + leftDist;
            int yPos = 0 + topDist;
            glfwSetWindowPos(window, xPos, yPos);
        }        
        refreshRate = mode->refreshRate;
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

    {        
        glfwGetFramebufferSize(window, &gGameManager._fbWidth, &gGameManager._fbHeight);
        glfwGetWindowSize(window, &gGameManager._windowWidth, &gGameManager._windowHeight);
        gGameManager._aspectRatio = 2560.f / 1494.f;
        gGameManager._viewportInfo = CalculateViewport(gGameManager._aspectRatio, gGameManager._fbWidth, gGameManager._fbHeight, gGameManager._windowWidth, gGameManager._windowHeight);
        SetViewport(gGameManager._viewportInfo);
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowPosCallback(window, WindowPosCallback);

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

    SoundBank soundBank;

    synth::PatchBank synthPatchBank;
    if (cmdLineInputs._synthPatchesFilename.has_value()) {
        bool success = serial::LoadFromFile(cmdLineInputs._synthPatchesFilename->c_str(), synthPatchBank);
        if (!success) {
            printf("FAILED to read synth patches file \"%s\"\n", cmdLineInputs._synthPatchesFilename->c_str());
        }
    }

    SynthGuiState synthGuiState;
    {
        if (cmdLineInputs._synthPatchesFilename.has_value()) {
            synthGuiState._saveFilename = cmdLineInputs._synthPatchesFilename.value();            
        }
        synthGuiState._synthPatches = &synthPatchBank;
    }

    // Init audio
    audio::Context audioContext;
    // Set gain from command line
    audioContext._state._finalGain = cmdLineInputs._gain;

    {        
        if (audio::Init(audioContext, soundBank) != paNoError) {
            ShutDown(audioContext, soundBank);
            return 1;
        }
    }    

    soundBank.LoadSounds(audioContext._sampleRate);

    // Initialize synth to starting patches.
    {
        int const numSynths = audioContext._state.synths.size();
        audio::Event e;
        e.type = audio::EventType::SynthParam;
        e.paramChangeTimeSecs = 0.0;
        for (int ii = 0; ii < numSynths && ii < synthPatchBank._patches.size(); ++ii) {
            synth::Patch const& patch = synthPatchBank._patches[ii];
            e.channel = ii;
            for (int paramIx = 0, n = (int)audio::SynthParamType::Count; paramIx < n; ++paramIx) {
                audio::SynthParamType paramType = (audio::SynthParamType) paramIx;
                float v = patch.Get(paramType);
                e.param = paramType;
                e.newParamValue = v;
                audioContext.AddEvent(e);
            }
        }
    }

    int const bufferFrameCount = audioContext._state._bufferFrameCount;
    gDftCount = bufferFrameCount / 2;
    gDft = new double[gDftCount]();
    double* dftFreqs = new double[gDftCount]();
    float sampleRate = audioContext._sampleRate;
    for (int i = 0; i < gDftCount; ++i) {
        dftFreqs[i] = (i+1) * sampleRate / bufferFrameCount;
    }

    int startDftBin = 0;
    int numOctaves = 0;
    int const minBandwidthPerOctave = 200;
    int const bandsPerOctave = 2; 
    {
        for (int ii = 1; ii < gDftCount; ii = ii << 1) {
            float f = dftFreqs[ii - 1];
            if (f >= minBandwidthPerOctave) {
                startDftBin = ii - 1;
                int x = ii;
                int count = 0;
                while (x <= gDftCount) {
                    x = x << 1;
                    ++count;
                }
                numOctaves = count;
                break;
            }

        }  
    }
    gDftLogAvgsCount = numOctaves * bandsPerOctave;
    gDftLogAvgs = new double[gDftLogAvgsCount]();
    gDftLogAvgsMaxFreqs = new double[gDftLogAvgsCount]();


    InputManager inputManager(window);

    Editor editor;

    renderer::Scene sceneManager;
    sceneManager.SetDrawTerrain(cmdLineInputs._drawTerrain);

    ne::EntityManager neEntityManager;

    BeatClock beatClock;

    gGameManager._editor = &editor;
    gGameManager._scene = &sceneManager;
    gGameManager._inputManager = &inputManager;
    gGameManager._audioContext = &audioContext;
    gGameManager._neEntityManager = &neEntityManager;
    gGameManager._beatClock = &beatClock;
    gGameManager._soundBank = &soundBank;
    gGameManager._synthPatchBank = &synthPatchBank;
    gGameManager._editMode = cmdLineInputs._editMode;

    if (!sceneManager.Init(gGameManager)) {
        std::cout << "scene failed to init. exiting" << std::endl;
        ShutDown(audioContext, soundBank);
        return 1;
    }

    neEntityManager.Init();

    // SCRIPT LOADING
    if (cmdLineInputs._scriptFilename.has_value()) {
        std::cout << "loading " << cmdLineInputs._scriptFilename.value() << std::endl;
        serial::Ptree pt = serial::Ptree::MakeNew();
        if (!pt.LoadFromFile(cmdLineInputs._scriptFilename->c_str())) {
            printf("Script ptree load failed. Exiting.\n");
            pt.DeleteData();
            ShutDown(audioContext, soundBank);
            return 1;
        }

        double bpm = pt.GetDouble("script.bpm");
        beatClock.Init(gGameManager, bpm, audioContext._sampleRate);
        beatClock.Update(gGameManager);        

        {
            // New entity loading. Manually add them in this order to the editor to keep ordering consistent.
            // TODO: make this less shit
            editor._saveFilename = *cmdLineInputs._scriptFilename;
            serial::Ptree entitiesPt = pt.GetChild("script.new_entities");
            int numEntities = 0;
            serial::NameTreePair* children = entitiesPt.GetChildren(&numEntities);
            int64_t nextEditorId = 0;
            for (int i = 0; i < numEntities; ++i) {
                ne::EntityType entityType = ne::StringToEntityType(children[i]._name);
                bool active = true;
                children[i]._pt.TryGetBool("entity_active", &active);
                ne::Entity* entity = gGameManager._neEntityManager->AddEntity(entityType, active);
                entity->Load(children[i]._pt);
                // TODO: THIS SHOULD NOT BE NECESSARY NOW THAT LEVELS ARE UPDATED.
                if (entity->_editorId._id < 0) {
                    entity->_editorId._id = nextEditorId++;
                }
            }
            delete[] children;

            // Now go through and initialize everything.
            for (auto iter = gGameManager._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
                iter.GetEntity()->Init(gGameManager);
            }
        }

        {
            // Editor config loading
            editor._g = &gGameManager;
            serial::Ptree scriptPt = pt.GetChild("script");
            editor.Load(scriptPt);
        }

        pt.DeleteData();
    } else {
        printf("ERROR: TODO NEED TO SUPPLY A HARDCODED TEST SCRIPT\n");
        ShutDown(audioContext, soundBank);
        return 1;
        // std::cout << "loading hardcoded script" << std::endl;
        // beatClock.Init(/*bpm=*/120.0, /*sampleRate=*/SAMPLE_RATE, audioContext._stream);
        // beatClock.Update();
        // LoadTestScript(gGameManager);
    }

    editor.Init(&gGameManager);

#if COMPUTE_FFT
    double* fftIn = (double*) fftw_malloc(sizeof(double) * bufferFrameCount);
    fftw_complex* fftOut = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * bufferFrameCount);
    fftw_plan fftwPlan = fftw_plan_dft_r2c_1d(bufferFrameCount, fftIn, fftOut, FFTW_ESTIMATE);
#endif

    float const fixedTimeStep = 1.f / static_cast<float>(refreshRate);
    bool paused = false;
    while(!glfwWindowShouldClose(window)) {

        // IS IT OKAY THAT I'M USING SCREEN COORDS HERE?!?!?!?!
        glfwGetWindowSize(window, &gGameManager._windowWidth, &gGameManager._windowHeight);
        glfwGetFramebufferSize(window, &gGameManager._fbWidth, &gGameManager._fbHeight);
        gGameManager._viewportInfo = CalculateViewport(gGameManager._aspectRatio, gGameManager._fbWidth, gGameManager._fbHeight, gGameManager._windowWidth, gGameManager._windowHeight);

        beatClock.Update(gGameManager);

#if COMPUTE_FFT
        {
            audioContext._state._recentBufferMutex.lock();
            for (int i = 0; i < bufferFrameCount; ++i) {
                fftIn[i] = audioContext._state._recentBuffer[i];
            }
            audioContext._state._recentBufferMutex.unlock();

            fftw_execute(fftwPlan);

            for (int i = 0; i < gDftCount; ++i) {
                float r = fftOut[i+1][0];
                float img = fftOut[i+1][1];
                float v = 2 * sqrt(r*r + img*img) / bufferFrameCount;
                gDft[i] = v;
            }

            // DFT LOG AVERAGES
            memset(gDftLogAvgs, 0, sizeof(*gDftLogAvgs) * gDftLogAvgsCount);
            float octaveMinFreq = 0.f;
            float octaveMaxFreq = dftFreqs[startDftBin];
            int dftIx = startDftBin;
            float const dftBinHalfBw = (1.f / bufferFrameCount) * sampleRate / 2.f;
            for (int octaveIx = 0; octaveIx < numOctaves; ++octaveIx) {
                float octaveBw = octaveMaxFreq - octaveMinFreq;
                float logBinBw = octaveBw / bandsPerOctave;
                float logBinMinFreq = octaveMinFreq;
                for (int bandIx = 0; bandIx < bandsPerOctave; ++bandIx) {
                    float logBinMaxFreq = logBinMinFreq + logBinBw;
                    
                    // logAvgBinMinFreqs[octaveIx*bandsPerOctave + bandIx] = logBinMinFreq;
                    gDftLogAvgsMaxFreqs[octaveIx*bandsPerOctave + bandIx] = logBinMaxFreq;

                    int count = 0;
                    while (dftIx < gDftCount) {
                        float dftCenter = dftFreqs[dftIx];
                        float dftBinMaxFreq = dftCenter + dftBinHalfBw;
                        gDftLogAvgs[octaveIx * bandsPerOctave + bandIx] += gDft[dftIx];
                        ++count;
                        if (dftBinMaxFreq < logBinMaxFreq) {
                            ++dftIx;
                        } else {
                            break;
                        }
                    }
                    if (count > 1) {
                        gDftLogAvgs[octaveIx*bandsPerOctave + bandIx] /= count;          
                    }

                    logBinMinFreq = logBinMaxFreq;
                }

                octaveMinFreq = octaveMaxFreq;
                octaveMaxFreq *= 2.f; 
            }

        } 
#endif // COMPUTE_FFT

        {
            ImGuiIO& io = ImGui::GetIO();
            bool inputEnabled = !io.WantCaptureMouse && !io.WantCaptureKeyboard;
            inputManager.Update(inputEnabled);
        }

        // if (!cmdLineInputs._editMode && inputManager.IsKeyPressedThisFrame(InputManager::Key::Space)) {
        //     paused = !paused;
        // }

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

        MaybeToggleMute(fixedTimeStep);

        if (gGameManager._editMode) {
            for (auto iter = gGameManager._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
                ne::Entity* e = iter.GetEntity();
                if (editor._enableFlowSectionFilter && e->_flowSectionId >= 0 && editor._flowSectionFilterId != e->_flowSectionId) {
                    continue;
                }
                e->UpdateEditMode(gGameManager, dt, /*isActive=*/true);
            }

            for (auto iter = gGameManager._neEntityManager->GetAllInactiveIterator(); !iter.Finished(); iter.Next()) {
                ne::Entity* e = iter.GetEntity();
                if (editor._enableFlowSectionFilter && e->_flowSectionId >= 0 && editor._flowSectionFilterId != e->_flowSectionId) {
                    continue;
                }
                e->UpdateEditMode(gGameManager, dt, /*isActive=*/false);
            }
        } else {
            for (auto iter = gGameManager._neEntityManager->GetAllIterator(); !iter.Finished(); iter.Next()) {
                ne::Entity* e = iter.GetEntity();                
                e->Update(gGameManager, dt);
            }
        }        

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        editor.Update(dt, synthGuiState);
        ImGui::Render();

        // Rendering
        // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
        neEntityManager.DestroyTaggedEntities(gGameManager);
        neEntityManager.DeactivateTaggedEntities(gGameManager);
        neEntityManager.ActivateTaggedEntities(gGameManager);
    }

#if COMPUTE_FFT
    fftw_destroy_plan(fftwPlan);
    fftw_free(fftIn);
    fftw_free(fftOut);
#endif

    ShutDown(audioContext, soundBank);

    return 0;
}
