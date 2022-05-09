#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>

#include <portaudio.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

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
#include "camera.h"
#include "component.h"

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

void CreateCube(
    Shader const& shader, glm::mat4 const* viewProjTrans, BoundMesh const& mesh, Entity* e) {
    auto t = std::make_unique<TransformComponent>();
    auto m = std::make_unique<ModelComponent>(t.get(), viewProjTrans, &mesh);
    e->_transform = std::move(t);
    e->_model = std::move(m);
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
    // glfwSwapInterval(1);  // vsync?

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    glEnable(GL_DEPTH_TEST);

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
        std::array<float,180> cubeVerts;
        GetCubeVertices(&cubeVerts);
        int const numCubeVerts = 36;
        cubeMesh.Init(cubeVerts.data(), numCubeVerts, &material);
    }

    BeatClock beatClock(/*bpm=*/120.0, (unsigned int)sampleRate, audioContext._stream);
    beatClock.Update();

    InputManager inputManager(window);

    DebugCamera camera(inputManager);
    camera._pos = glm::vec3(0.0f, 0.0f, 3.0f);

    // NOTE!!! DO NOT TRY TO USE THIS UNTIL IT'S BEEN SET IN THE RENDER LOOP BELOW
    glm::mat4 viewProjTransform(1.f);

    Entity cubeEntity;
    CreateCube(shaderProgram, &viewProjTransform, cubeMesh, &cubeEntity);

    Entity cubeEntity2;
    CreateCube(shaderProgram, &viewProjTransform, cubeMesh, &cubeEntity2);
    cubeEntity2._transform->_transform = 
        glm::translate(cubeEntity2._transform->_transform, glm::vec3(-1.f,0.f,0.f));

    // Init event queue with a synth sequence
    {
        double const audioTime = Pa_GetStreamTime(audioContext._stream);
        InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
    }

    float lastGlfwTime = (float)glfwGetTime();
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

        camera.Update(dt);

        if (inputManager.IsKeyPressed(InputManager::Key::Escape)) {
            glfwSetWindowShouldClose(window, true);
        }        

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        float aspectRatio = (float)windowWidth / windowHeight;
        glm::mat4 projTransform = glm::perspective(
            /*fovy=*/glm::radians(45.f), aspectRatio, /*near=*/0.1f, /*far=*/100.0f);
        viewProjTransform = projTransform * camera.GetViewMatrix();

        cubeEntity.Update(dt);
        cubeEntity2.Update(dt);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    // NOTE: Do not use BeatClock after shutting down audio.
    if (audio::ShutDown(audioContext) != paNoError) {
        return 1;
    }

    drwav_free(pSampleData, NULL);

    return 0;
}