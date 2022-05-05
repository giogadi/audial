#include "portaudio_util.h"

void OnPortAudioError(PaError const& err) {
    Pa_Terminate();
    std::cout << "An error occured while using the portaudio stream" << std::endl;
    std::cout << "Error number: " << err << std::endl;
    std::cout << "Error message: " << Pa_GetErrorText(err) << std::endl;
}

PaError InitPortAudio(PortAudioContext& context)
{
    PaError err;
    
    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    synth::InitEventQueueWithSequence(&context._eventQueue, SAMPLE_RATE);
    synth::InitStateData(context._state, &context._eventQueue, SAMPLE_RATE);

    err = Pa_Initialize();
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    context._outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (context._outputParameters.device == paNoDevice) {
      printf("Error: No default output device.\n");
      return err;
    }
    context._outputParameters.channelCount = 2;       /* stereo output */
    context._outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    context._outputParameters.suggestedLatency = Pa_GetDeviceInfo( context._outputParameters.device )->defaultLowOutputLatency;
    context._outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &context._stream,
              NULL, /* no input */
              &context._outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              Callback,
              &context._state );
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_SetStreamFinishedCallback( context._stream, &StreamFinished );
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_StartStream( context._stream );
    if( err != paNoError ) {
        OnPortAudioError(err);
        return err;
    }

    return paNoError;
}

PaError ShutDownPortAudio(PaStream* stream) {
    PaError err = Pa_StopStream(stream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        OnPortAudioError(err);
        return err;
    }

    Pa_Terminate();
    return paNoError;
}