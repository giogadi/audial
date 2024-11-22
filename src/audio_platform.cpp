#include "audio_platform.h"

#include <portaudio.h>

#ifdef _WIN32
#include "portaudio/include/pa_win_wasapi.h"
#endif

#define FRAMES_PER_BUFFER  (512)

namespace audio {
namespace {

PaStreamParameters sOutputParameters;
PaStream* sStream = nullptr;

void StreamFinished(void* userData)
{
}

void OnPortAudioError(PaError const& err) {
    Pa_Terminate();
    std::cout << "An error occured while using the portaudio stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
}

int PortAudioCallback(
    const void *inputBuffer, void *const outputBufferUntyped,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* /*timeInfo*/,
    PaStreamCallbackFlags /*statusFlags*/,
    void *userData) {
    AudioCallback((float const*)inputBuffer, (float *)outputBufferUntyped, framesPerBuffer, (StateData*)userData);

    return paContinue;
}

PaError PortAudioInit(
    Context& context, SoundBank const& soundBank) {
    int const kPreferredSampleRate = 48000;
    //int const kPreferredSampleRate = 44100;
    //int const kPreferredSampleRate = 96000;

    PaError err;

    err = Pa_Initialize();
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    sOutputParameters = {0};

    context._outputSampleRate = kPreferredSampleRate;
    std::vector<PaError> errors;
#ifdef _WIN32
    PaHostApiIndex const hostApiCount = Pa_GetHostApiCount();
    PaHostApiInfo const* hostApiInfo = 0;
    for (PaHostApiIndex hostApiIx = 0; hostApiIx < hostApiCount; ++hostApiIx) {
        PaHostApiInfo const *thisHost = Pa_GetHostApiInfo(hostApiIx);
        if (thisHost->type == paWASAPI) {
            hostApiInfo = thisHost;
            break;
        }
    }
    if (!hostApiInfo) {
        printf("WARNING: could not find a WASAPI audio host. You may have degraded/delayed audio performance.\n");
        hostApiInfo = Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
        printf("Using audio host API \"%s\"\n", hostApiInfo->name);
    }
    PaDeviceIndex deviceIx = hostApiInfo->defaultOutputDevice;
    sOutputParameters.device = deviceIx;
    PaDeviceInfo const* pDeviceInfo = Pa_GetDeviceInfo(deviceIx);
    // Check if our preferred sample rate is supported. If not, just use the device's default sample rate.
    PaStreamParameters params = {0};
    params.device = sOutputParameters.device;
    params.channelCount = audio::NumOutputChannels();
    params.sampleFormat = paFloat32;
    double sampleRate = kPreferredSampleRate;
    err = Pa_IsFormatSupported(0, &params, sampleRate);
    if (err) {
        // TODO: show previous error
        sampleRate = pDeviceInfo->defaultSampleRate;
        err = Pa_IsFormatSupported(0, &params, sampleRate);
        if (err) {
            printf("Default device does not support required audio formats.\n");
        } else {
            printf("Your audio device is set to an audio rate of %d. This will work, but you may experience better performance if you use this game's preferred audio rate of %d.\n", (int)sampleRate, kPreferredSampleRate);
        }
    }
    context._outputSampleRate = (int)sampleRate;
    sOutputParameters.device = deviceIx;
    printf("Selected audio device: %s\n", pDeviceInfo->name);
    printf("Device sample rate: %d\n", (int)sampleRate);
#else
    sOutputParameters.device = Pa_GetDefaultOutputDevice();
#endif     

    if (err != paNoError) {
        printf("Audio error: default audio device not supported.\n");
        char const* finalErrMsg = Pa_GetErrorText(err);
        printf("Final error: %s\n", finalErrMsg);
        printf("Error list:\n");
        for (int i = 0; i < errors.size(); ++i) {
            char const* errMsg = Pa_GetErrorText(errors[i]);
            printf("Error %d: %s\n", i, errMsg);
        }
        return err;
    }

    InitStateData(context._state, soundBank, context._outputSampleRate, FRAMES_PER_BUFFER);

    sOutputParameters.channelCount = NumOutputChannels();
    sOutputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    sOutputParameters.suggestedLatency = Pa_GetDeviceInfo(sOutputParameters.device)->defaultLowOutputLatency;

    err = Pa_OpenStream(
        &sStream,
        NULL, /* no input */
        &sOutputParameters,
        context._outputSampleRate,
        FRAMES_PER_BUFFER,
        paNoFlag,  // no flags (clipping is on)
        PortAudioCallback,
        &context._state);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_SetStreamFinishedCallback(sStream, &StreamFinished);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_StartStream(sStream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    return paNoError;
}

PaError PortAudioShutDown() {
    PaError err = Pa_StopStream(sStream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_CloseStream(sStream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    Pa_Terminate();
    
    return paNoError;
}

}

bool Context::AddEvent(Event const &e) {
    return audio::AddEvent(e);
}

bool Context::Init(SoundBank const &soundBank) {
    if (PortAudioInit(*this, soundBank) != paNoError) {
        ShutDown();
        return false;
    }
    return true;
}

void Context::ShutDown() {
    PaError err = PortAudioShutDown();
    if (err != paNoError) {
        printf("Error in shutting down audio! error: %s\n", Pa_GetErrorText(err));
    }
    DestroyStateData(_state);
}

double Context::GetAudioTime() {
    return Pa_GetStreamTime(sStream);
}

}  // namespace audio