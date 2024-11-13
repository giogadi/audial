#include "samplerate.h"
#include "portaudio.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUM_CHANNELS (2)
#define INTERNAL_SR (48000)
//#define INTERNAL_SR (44100)
//#define OUTPUT_SR (44100)
//#define OUTPUT_SR (48000)
#define OUTPUT_SR (96000)
#define FRAMES_PER_BUFFER (256)
#define NUM_SECONDS (4)

typedef struct {
    float phase;

    SRC_STATE *resampleState;
    float *internalBuffer;
    int internalBufferFrameCount;
    int leftoverInputFrames;

    float *outputCopy;
    int outputCopyIx;

    float *playbackBuf;
    int playbackBufFrames;
    int playbackBufIx;
} AudioData;

static int AudioCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData ) {
    static int sNumBuffers = 0;
    static int sNumFrames = 0;

    ++sNumBuffers;

    assert(framesPerBuffer == FRAMES_PER_BUFFER);
    AudioData *data = (AudioData*)userData;
    float *out = (float*)outputBuffer;

    if (data->playbackBuf) {
        for (int ii = 0; ii < framesPerBuffer; ++ii) {
            for (int c = 0; c < NUM_CHANNELS; ++c) {
                out[NUM_CHANNELS*ii + c] = data->playbackBuf[NUM_CHANNELS*data->playbackBufIx + c];
            }
            ++data->playbackBufIx;
            if (data->playbackBufIx >= data->playbackBufFrames) {
                printf("FLIPPED!\n");
                data->playbackBufIx = 0;
            }
        }
        return 0;
    }

    float const kFreq = 440.f;
    float dphase = kFreq * 2 * M_PI / INTERNAL_SR; 

    float numInputFramesExact = (float)framesPerBuffer * (float) INTERNAL_SR / (float) OUTPUT_SR;
    int numInputFrames = (int) ceilf(numInputFramesExact);

    if (data->leftoverInputFrames > 0) {
        float *leftoverFrames = data->internalBuffer + (NUM_CHANNELS * data->internalBufferFrameCount) - (NUM_CHANNELS * data->leftoverInputFrames);
        size_t size = (sizeof(float) * data->leftoverInputFrames * NUM_CHANNELS);
        if ((void*)(data->internalBuffer) + size > (void*)leftoverFrames) {
            printf("WOOPS, overlapping data!!! num frames: %d\n", data->leftoverInputFrames);
        }
        size_t endOfLeftovers = (void*)leftoverFrames + size;
        size_t endOfBuffer = (void*)data->internalBuffer + sizeof(float) * data->internalBufferFrameCount * NUM_CHANNELS;
        if (endOfLeftovers > endOfBuffer) {
            printf("UH-OH!\n");
        }
        //memcpy(data->internalBuffer, leftoverFrames, size);
        memmove(data->internalBuffer, leftoverFrames, size);
    } 

    float *internalOut = data->internalBuffer + (data->leftoverInputFrames * NUM_CHANNELS);
    for (int frameIx = data->leftoverInputFrames; frameIx < numInputFrames; ++frameIx) { 
        float v = sin(data->phase);
        for (int channel = 0; channel < NUM_CHANNELS; ++channel) {
            *internalOut++ = v;
        }
        data->phase += dphase;
        if (data->phase > 2 * M_PI) {
            data->phase -= 2 * M_PI;
        } 
        ++sNumFrames;
    }

    

    SRC_DATA resampleData = {
        .data_in = data->internalBuffer,
        .data_out = out,
        .input_frames = numInputFrames,
        .output_frames = framesPerBuffer,
        .src_ratio = (double)OUTPUT_SR / INTERNAL_SR
    };
    int err = src_process(data->resampleState, &resampleData);
    if (err) {
        printf("ERROR: %s\n", src_strerror(err));
    }
    data->leftoverInputFrames = numInputFrames - resampleData.input_frames_used;
    if (resampleData.input_frames_used != numInputFrames) {
        printf("WHOA: e:%d a:%ld g:%ld\n", numInputFrames, resampleData.input_frames_used, resampleData.output_frames_gen);
        if (data->leftoverInputFrames < 0) {
            printf("negative input frames?!?! %d\n", data->leftoverInputFrames);
            data->leftoverInputFrames = 0;
        }
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

    out = (float*) outputBuffer;
    for (int frameIx = 0; frameIx < framesPerBuffer; ++frameIx) {
        data->outputCopy[data->outputCopyIx + frameIx] = out[2*frameIx];
    }
    data->outputCopyIx += framesPerBuffer;

    return 0;
}

typedef struct {
    float x;
    int ix;
} FloatAndIx;

int CompareFloats(void const *a, void const *b) {
    FloatAndIx *a1 = (FloatAndIx*)a;
    FloatAndIx *b1 = (FloatAndIx*)b;

    if (a1->x < b1->x) {
        return -1;
    }
    if (a1->x > b1->x) {
        return 1;
    }
    return 0;
}

void Analyze() {
    FILE *fp = fopen("resample_output.txt", "r");
    char *line = 0;
    size_t len = 0;
    int ix = 0;
    #define MAX_DIFF_COUNT 50
    FloatAndIx maxDiffs[MAX_DIFF_COUNT] = {0};
    bool hasPrev = false;
    float prev = 0.f;
    float avg = 0.f;
    while (getline(&line, &len, fp) != -1) {
        float x = 0.;
        sscanf(line, "%f", &x);
        if (hasPrev) {
            float diff = fabs(x - prev);
            if (diff > maxDiffs[0].x) {
                maxDiffs[0].x = diff;
                maxDiffs[0].ix = ix;
                qsort(maxDiffs, MAX_DIFF_COUNT, sizeof(FloatAndIx), CompareFloats);
            }  
            avg += diff;
        }
        prev = x;
        hasPrev = true; 
        ++ix;
    }
    avg /= ix;

    for (int ii = 0; ii < MAX_DIFF_COUNT; ++ii) {
        printf("%d: %f\n", maxDiffs[ii].ix, maxDiffs[ii].x);
    }

    printf("FINISHED: %d samples.\n", ix);
    printf("Avg diff: %f\n", avg);
}

void FillPlaybackBuf(float *outputBuf, int numOutputFrames) {

    float const kFreq = 440.f;
    float dphase = kFreq * 2 * M_PI / INTERNAL_SR; 


    float phase = 0.f;
    float numInputFramesExact = (float) numOutputFrames * (float)INTERNAL_SR / (float)OUTPUT_SR;
    int numInputFrames = ceilf(numInputFramesExact) + 1000;
    float *inputBuf = malloc(sizeof(float) * NUM_CHANNELS * numInputFrames);
    float *p = inputBuf;
    for (int ii = 0; ii < numInputFrames; ++ii) {
        float v = sin(phase);
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            *p++ = v;
        } 
        phase += dphase;
        if (phase > 2 * M_PI) {
            phase -= 2 * M_PI;
        }
    }

    int err = 0;
    SRC_STATE *srcState = src_new(SRC_SINC_BEST_QUALITY, NUM_CHANNELS, &err);
    if (err) {
        printf("error in creating src state: %s\n", src_strerror(err));
    }

    int inputFramesProcessed = 0;
    int outputFramesProcessed = 0;
    float *pIn = inputBuf;
    float *pOut = outputBuf;
    int expectedInputFramesPerBuffer = ceilf((float)FRAMES_PER_BUFFER * (float)INTERNAL_SR / (float)OUTPUT_SR);
    while (outputFramesProcessed < numOutputFrames) {
        SRC_DATA data = {
            .data_in = pIn,
            .data_out = pOut,
            //.input_frames = numInputFrames - inputFramesProcessed,
            .input_frames = expectedInputFramesPerBuffer,
            .output_frames = FRAMES_PER_BUFFER,
            .end_of_input = false,
            .src_ratio = (double)OUTPUT_SR / (double)INTERNAL_SR
        };
        int err = src_process(srcState, &data);
        if (err) {
            printf("error in src process: %s\n", src_strerror(err));
        }
        if (data.input_frames_used != expectedInputFramesPerBuffer || data.output_frames_gen != FRAMES_PER_BUFFER) {
            printf("??? ix: %d e:%i a:%ld g:%ld\n", outputFramesProcessed, expectedInputFramesPerBuffer, data.input_frames_used, data.output_frames_gen);
        } 
        //pIn += data.input_frames_used * NUM_CHANNELS;
        //pOut += data.output_frames_gen * NUM_CHANNELS;
        //inputFramesProcessed += data.input_frames_used;
        //outputFramesProcessed += data.output_frames_gen;
        pIn += expectedInputFramesPerBuffer * NUM_CHANNELS;
        pOut += FRAMES_PER_BUFFER * NUM_CHANNELS;
        inputFramesProcessed += expectedInputFramesPerBuffer;
        outputFramesProcessed += data.output_frames_gen; 
    } 

    free(inputBuf);
    src_delete(srcState);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "a") == 0) {
        Analyze();
        return 0;
    }
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
    data.outputCopy = malloc(sizeof(float) * OUTPUT_SR * NUM_SECONDS * 2);
#endif

    if (argc > 1 && strcmp(argv[1], "p") == 0) {
        int numPlaybackFrames = NUM_SECONDS * OUTPUT_SR;
        float *playbackBuf = malloc(sizeof(float) * NUM_CHANNELS * numPlaybackFrames);
        FillPlaybackBuf(playbackBuf, numPlaybackFrames);
        data.playbackBuf = playbackBuf;
        data.playbackBufFrames = numPlaybackFrames;
    }


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

    FILE *fp = fopen("resample_output.txt", "w");
    for (int ii = 0; ii < data.outputCopyIx; ++ii) {
        fprintf(fp, "%f\n", data.outputCopy[ii]);
    }
    fclose(fp);

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
