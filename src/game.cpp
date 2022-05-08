#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>

#include <portaudio.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "glm/ext/matrix_transform.hpp"

#include "stb_image.h"

#include "audio.h"
#include "beat_clock.h"
#include "shader.h"
#include "cube_verts.h"

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

void ProcessInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    BeatClock beatClock(/*bpm=*/120.0, (unsigned int)sampleRate, audioContext._stream);
    beatClock.Update();

    Shader shaderProgram;
    if (!shaderProgram.Init("shaders/shader.vert", "shaders/shader.frag")) {
        return 1;
    }

    // TEXTURE
    int texWidth, texHeight, texNumChannels;
    std::string const textureFilename("data/textures/wood_container.jpg");
    stbi_set_flip_vertically_on_load(true);
    unsigned char *texData = stbi_load(textureFilename.c_str(), &texWidth, &texHeight, &texNumChannels, 0);
    if (texData == nullptr) {
        std::cout << "Error: could not load texture " << textureFilename << std::endl;
        return 1;
    }
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D, /*mipmapLevel=*/0, /*textureFormat=*/GL_RGB, texWidth, texHeight, /*legacy=*/0,
        /*sourceFormat=*/GL_RGB, /*sourceDataType=*/GL_UNSIGNED_BYTE, texData);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(texData);

    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    std::array<float,180> vertices;
    GetCubeVertices(&vertices);
    int const numVertices = 36;

    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(
        /*attributeIndex=*/0, /*numValues=*/3, /*valueType=*/GL_FLOAT, /*normalized=*/false,
        /*stride=*/5*sizeof(float), /*offsetOfFirstValue=*/(void*)0);
    // This vertex attribute will be read from the VBO that is currently bound
    // to GL_ARRAY_BUFFER, which was bound above to "vbo".
    glEnableVertexAttribArray(/*attributeIndex=*/0);

    // Texture
    glVertexAttribPointer(
        /*attributeIndex=*/1, /*numValues=*/2, /*valueType=*/GL_FLOAT, /*normalized=*/false,
        /*stride=*/5*sizeof(float), /*offsetOfFirstValue=*/(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(/*attributeIndex=*/1);

    // Init event queue with a synth sequence
    {
        double const audioTime = Pa_GetStreamTime(audioContext._stream);
        InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
    }

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

        ProcessInput(window);

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram.Use();

        glm::mat4 transform(1.0f);
        // float angle = (float) beatClock.GetBeatTime() * 2 * (float)M_PI / 4;
        float angle = (float)M_PI / 4.0f;
        transform = glm::rotate(transform, angle, glm::vec3(0.0f,0.0f,1.0f));
        shaderProgram.SetMat4("transform", transform);

        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, /*startIndex=*/0, numVertices);

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