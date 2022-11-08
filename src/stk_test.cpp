#include <cstdio>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <portaudio.h>

#include <stk/BlitSaw.h>
#include <stk/ADSR.h>
#include <stk/BiQuad.h>

static int gKeyState = 0;  // 0: nothing new; 1: on; 2: off

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE) {
	if (action == GLFW_PRESS) {
	    gKeyState = 1;
	} else if (action == GLFW_RELEASE) {
	    gKeyState = 2;
	}
    }
}

float constexpr kPi = 3.14159265359f;

struct AudioData {
    stk::ADSR _adsr;
    stk::BlitSaw _saw;
    stk::BiQuad _biquad;
    float _phase = 0.f;
};

int PortAudioCallback( const void *inputBuffer, void *outputBuffer,
		       unsigned long samplesPerBuffer,
		       const PaStreamCallbackTimeInfo* timeInfo,
		       PaStreamCallbackFlags statusFlags,
		       void *userData )
{
    /* Cast data passed through stream to our structure. */
    AudioData *data = (AudioData*)userData;
    float *out = (float*)outputBuffer;
    (void) inputBuffer; /* Prevent unused variable warning. */

    // Check for control inputs.
    if (gKeyState == 1) {
	data->_adsr.keyOn();
	gKeyState = 0;
    } else if (gKeyState == 2) {
	data->_adsr.keyOff();
	gKeyState = 0;
    }

    // Modulate biquad resonance from 0 to 0.5*sampleRate
    // lfo: let's say we want the period to be 1 second.
    // function: 0.5*sin(x - pi/2) + 0.5
    float const kLfoRate = 2.f;  // hertz
    float const lfoPhaseChange = kLfoRate * 2 * kPi / 44100.f;

    for(int i = 0; i < samplesPerBuffer; ++i)
    {
	float f = data->_saw.tick();

	float lfo = 0.5f * sin(data->_phase - 0.5f*kPi) + 0.5f;
	data->_phase += lfoPhaseChange;
	if (data->_phase > 2 * kPi) {
	    data->_phase -= 2 * kPi;
	}
	data->_biquad.setResonance(lfo * 0.5f*44100.f, 0.98f, true);	
	f = data->_biquad.tick(f);
	
	float env = data->_adsr.tick();
	f *= env;
	*out++ = f;
	*out++ = f;
    }
    return 0;
}

int main() {
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    glfwSetKeyCallback(window, KeyCallback);

    // Audio
    double constexpr kSampleRate = 44100.0;
    unsigned long constexpr kSamplesPerAudioBuffer = 64;
    stk::Stk::setSampleRate(kSampleRate);
    
    PaStream *stream;
    PaError err;
    err = Pa_Initialize();
    AudioData audioData;
    audioData._adsr.setAttackTime(0.05f);
    audioData._adsr.setReleaseTime(0.5f);
    audioData._biquad.setResonance(440.f, 0.98f, true);
    

    err = Pa_OpenDefaultStream( &stream,
                                0,          /* no input channels */
                                2,          /* stereo output */
                                paFloat32,  /* 32 bit floating point output */
                                kSampleRate,
                                kSamplesPerAudioBuffer,        /* frames per buffer */
                                PortAudioCallback,
                                &audioData );
    assert(err == paNoError);

    err = Pa_StartStream( stream );
    assert(err == paNoError);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    assert(Pa_StopStream(stream) == paNoError);
    assert(Pa_CloseStream(stream) == paNoError);
    Pa_Terminate();
    

    glfwTerminate();
    
    return 0;
}
