#include <cstdio>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <audio.h>
#include <sound_bank.h>
#include <synth_imgui.h>

bool gGetPianoInput = true;

int GetNoteIndexFromKey(int glfwKey) {
    switch (glfwKey) {
        case GLFW_KEY_A: { return 0; }
        case GLFW_KEY_W: { return 1; }
        case GLFW_KEY_S: { return 2; }
        case GLFW_KEY_E: { return 3; }
        case GLFW_KEY_D: { return 4; }
        case GLFW_KEY_F: { return 5; }
        case GLFW_KEY_T: { return 6; }
        case GLFW_KEY_G: { return 7; }
        case GLFW_KEY_Y: { return 8; }
        case GLFW_KEY_H: { return 9; }
        case GLFW_KEY_U: { return 10; }
        case GLFW_KEY_J: { return 11; }
        case GLFW_KEY_K: { return 12; }
    }
    return -1;
}

int GetMidiNoteFromKey(int glfwKey, int octave) {
    assert(octave >= 0);
    int offset = GetNoteIndexFromKey(glfwKey);
    if (offset < 0) {
        return -1;
    }
    int midiNote = 12 * (octave + 1) + offset;
    return midiNote;
}

audio::Context* gpContext = nullptr;
int gOctave = 2;
int gCurrentSynthIx = 0;

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_LEFT) {
            gOctave = std::max(0, gOctave - 1);
        } else if (key == GLFW_KEY_RIGHT) {
            gOctave = std::min(7, gOctave + 1);
        }
    }
    
    int midiNote = GetMidiNoteFromKey(key, gOctave);
    if (midiNote < 0) {
        return;
    }
    audio::Event e;
    e.channel = gCurrentSynthIx;
    if (action == GLFW_PRESS) {
        e.type = audio::EventType::NoteOn;
        e.midiNote = midiNote;	
    } else if (action == GLFW_RELEASE) {
        e.type = audio::EventType::NoteOff;
        e.midiNote = midiNote;
    }
    if (gpContext != nullptr) {
        gpContext->AddEvent(e);
    }
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expecting exactly one argument: a synth patch filename.\n");
        return 0;
    }

    audio::Context audioContext;
    gpContext = &audioContext;

    synth::PatchBank synthPatchBank;
    if (serial::LoadFromFile(argv[1], synthPatchBank)) {
        printf("Loaded synth patch data from %s.\n", argv[1]);
    }

    SynthGuiState synthGuiState;
    synthGuiState._saveFilename = argv[1];
    synthGuiState._synthPatches = &synthPatchBank;

    SoundBank soundBank;
    if (audio::Init(audioContext, soundBank) != paNoError) {
        return 1;
    }

    // TODO UGH GROSS. not thread-safe, etc etc
    int const numSynths = audioContext._state.synths.size();
    for (int ii = 0; ii < numSynths && ii < synthPatchBank._patches.size(); ++ii) {
        synth::StateData& synth = audioContext._state.synths[ii];
        synth.patch = synthPatchBank._patches[ii];
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
    glfwSetKeyCallback(window, KeyCallback);

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

    bool showSynthWindow = true;
    while (!glfwWindowShouldClose(window)) {
        {
            ImGuiIO& io = ImGui::GetIO();
            bool inputEnabled = !io.WantCaptureMouse && !io.WantCaptureKeyboard;
            gGetPianoInput = inputEnabled;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (showSynthWindow) {
            DrawSynthGuiAndUpdatePatch(synthGuiState, audioContext);
            gCurrentSynthIx = synthGuiState._currentSynthIx;
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

    if (audio::ShutDown(audioContext) != paNoError) {
        return 1;
    }

    return 0;
}

