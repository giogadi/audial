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
    TransformManager* transformMgr, SceneManager* sceneMgr,
    BoundMesh const& mesh, Entity* e) {
    TransformComponent* t = transformMgr->CreateTransform();
    e->_components.push_back(t);
    e->_components.push_back(sceneMgr->AddModel(t, &mesh));
}

void CreateLight(TransformManager* transformMgr, SceneManager* sceneMgr, Entity* e) {
    TransformComponent* t = transformMgr->CreateTransform();
    e->_components.push_back(t);
    e->_components.push_back(sceneMgr->AddLight(t, glm::vec3(0.2f,0.2f,0.2f), glm::vec3(1.f,1.f,1.f)));
}

void CreateCamera(
    TransformManager* transformMgr, SceneManager* sceneMgr, InputManager* inputMgr,
    Entity* e) {
    TransformComponent* t = transformMgr->CreateTransform();
    e->_components.push_back(t);
    e->_components.push_back(sceneMgr->AddCamera(t, inputMgr));
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
        std::array<float,kCubeVertsNumValues> cubeVerts;
        GetCubeVertices(&cubeVerts);
        int const numCubeVerts = 36;
        cubeMesh.Init(cubeVerts.data(), numCubeVerts, &material);
    }

    BeatClock beatClock(/*bpm=*/120.0, (unsigned int)sampleRate, audioContext._stream);
    beatClock.Update();

    InputManager inputManager(window);

    TransformManager transformManager;

    SceneManager sceneManager;

    EntityManager entityManager;

    // Camera
    Entity* camera = entityManager.AddEntity();
    CreateCamera(&transformManager, &sceneManager, &inputManager, camera);
    {
        TransformComponent* t = camera->DebugFindComponentOfType<TransformComponent>();
        t->SetPos(glm::vec3(0.f, 0.f, 3.f));
    }

    // Light
    Entity* light = entityManager.AddEntity();
    CreateLight(&transformManager, &sceneManager, light);
    {
        TransformComponent* t = light->DebugFindComponentOfType<TransformComponent>();
        t->SetPos(glm::vec3(0.f, 3.f, 0.f));
    }

    // Cube1
    Entity* cube1 = entityManager.AddEntity();
    CreateCube(&transformManager, &sceneManager, cubeMesh, cube1);
    {
        glm::mat4& t = cube1->DebugFindComponentOfType<TransformComponent>()->_transform;
        t = glm::rotate(t, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
        t = glm::rotate(t, glm::radians(45.f), glm::vec3(1.f, 0.f, 0.f));
        t = glm::translate(t, glm::vec3(1.f, 0.f, 0.f));
    }

    // Cube2
    Entity* cube2 = entityManager.AddEntity();
    CreateCube(&transformManager, &sceneManager, cubeMesh, cube2);
    {
        glm::mat4& t = cube2->DebugFindComponentOfType<TransformComponent>()->_transform;
        t = glm::translate(t, glm::vec3(-1.f,0.f,0.f));
    }

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

        sceneManager.Update(dt);

        if (inputManager.IsKeyPressed(InputManager::Key::Escape)) {
            glfwSetWindowShouldClose(window, true);
        }        

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // turn cube1
        {
            glm::mat4& t = cube1->DebugFindComponentOfType<TransformComponent>()->_transform;
            t = glm::rotate(t, dt * glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        // turn cube2
        {
            glm::mat4& t = cube2->DebugFindComponentOfType<TransformComponent>()->_transform;
            t = glm::rotate(t, dt * glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        sceneManager.Draw(windowWidth, windowHeight);

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