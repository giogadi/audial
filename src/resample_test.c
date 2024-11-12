#include "samplerate.h"
#include "portaudio.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CHANNELS (2)
#define INTERNAL_SR (48000)
#define OUTPUT_SR (44100)
//#define OUTPUT_SR (48000)
//#define OUTPUT_SR (96000)
#define FRAMES_PER_BUFFER (256)
#define NUM_SECONDS (8)

typedef struct {
    float phase;

    SRC_STATE *resampleState;
    float *internalBuffer;
    int internalBufferFrameCount;
} AudioData;

static int AudioCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData ) {
    assert(framesPerBuffer == FRAMES_PER_BUFFER);
    AudioData *data = (AudioData*)userData;
    float *out = (float*)outputBuffer;

    float const kFreq = 440.f;
    float dphase = kFreq * 2 * M_PI / INTERNAL_SR; 

    float *internalOut = data->internalBuffer;
    for (int frameIx = 0; frameIx < data->internalBufferFrameCount; ++frameIx) {
        float v = sin(data->phase);
        for (int channel = 0; channel < NUM_CHANNELS; ++channel) {
            *internalOut++ = v;
        }
        data->phase += dphase;
        if (data->phase > 2 * M_PI) {
            data->phase -= 2 * M_PI;
        }
    }

    SRC_DATA resampleData = {
        .data_in = data->internalBuffer,
        .data_out = out,
        .input_frames = data->internalBufferFrameCount,
        .output_frames = framesPerBuffer,
        .src_ratio = (double)OUTPUT_SR / INTERNAL_SR
    };
    int err = src_process(data->resampleState, &resampleData);
    if (err) {
        printf("ERROR: %s\n", src_strerror(err));
    }
    if (resampleData.output_frames_gen != FRAMES_PER_BUFFER) {
        printf("%ld %ld\n", resampleData.input_frames_used, resampleData.output_frames_gen);
        memset(out, 0, sizeof(float) * NUM_CHANNELS * framesPerBuffer);
    }

    out = (float*)outputBuffer;
    for (int frameIx = 0; frameIx < framesPerBuffer; ++frameIx) {
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            float v = *out;
            if (v < -1.f) {
                v = -1.f;
            } else if (v > 1.f) {
                v = 1.f;
            }
            *out++ = v;
        }
    }
    return 0;
}

int main() {
    PaStream *stream;
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) goto error;

    AudioData data = {0};
#if 1
    int resampleError = 0;
    data.resampleState = src_new(SRC_SINC_BEST_QUALITY, NUM_CHANNELS, &resampleError);
    if (!data.resampleState) {
        printf("libsamplerate error: %s\n", src_strerror(resampleError));
        goto error;
    }
    data.internalBufferFrameCount = (int) ceil((float) FRAMES_PER_BUFFER * INTERNAL_SR / OUTPUT_SR);
    printf("internal buffer frame count: %d\n", data.internalBufferFrameCount);
    data.internalBuffer = malloc(sizeof(float) * NUM_CHANNELS * data.internalBufferFrameCount);
#endif

    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,          /* no input channels */
                                NUM_CHANNELS,          /* stereo output */
                                paFloat32,  /* 32 bit floating point output */
                                OUTPUT_SR,
                                FRAMES_PER_BUFFER,        /* frames per buffer */
                                AudioCallback,
                                &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    /* Sleep for several seconds. */
    Pa_Sleep(NUM_SECONDS*1000);

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    src_delete(data.resampleState);
    Pa_Terminate();
    printf("Test finished.\n");
    return err;


error:
    src_delete(data.resampleState);
    Pa_Terminate();
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;

    return 0;
}
