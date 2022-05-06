#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstring>

#include <portaudio.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "audio.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

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

    glViewport(0, 0, 800, 600);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    double prevBeatTime = Pa_GetStreamTime(audioContext._stream);
    while(!glfwWindowShouldClose(window)) {
        double const audioTime = Pa_GetStreamTime(audioContext._stream);
        constexpr double kBpm = 120.0;
        double const beatTime = audioTime * (kBpm / 60.0);
        if (floor(beatTime) != floor(prevBeatTime)) {
            // New beat! queue a sound for the very next beat.
            double soundPlayBeatTime = floor(beatTime) + 1.0;
            double soundPlayTime = soundPlayBeatTime * (60.0 / kBpm);
            long timeInTicks = (long) (soundPlayTime * SAMPLE_RATE);
            audio::Event e;
            e.type = audio::EventType::PlayPcm;
            e.timeInTicks = timeInTicks;
            if (!audioContext._eventQueue.try_push(e)) {
                std::cout << "Failed to queue audio event" << std::endl;
            }
        }
        prevBeatTime = beatTime;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (audio::ShutDown(audioContext) != paNoError) {
        return 1;
    }

    drwav_free(pSampleData, NULL);

    return 0;
}