#include <immintrin.h>
#include <stdalign.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define SAMPLE_RATE 48000
#define NUM_VOICES 4
#define SAMPLES_PER_BUFFER 512
// #define VOICE_BUFFER_SIZE NUM_VOICES * SAMPLES_PER_BUFFER
#define PI 3.141592653589793

float MidiToFreq(int midi) {
    int const a4 = 69;  // 440Hz
    return 440.0f * pow(2.0f, (midi - a4) / 12.0f);
}

void FillWithSineSimple(int numBuffers, float* outputBuffer) {
    float phase = 0.f;
    float phaseChange = 2 * PI * 440.f / SAMPLE_RATE;
    float* pOutput = outputBuffer;
    for (int i = 0; i < numBuffers * SAMPLES_PER_BUFFER; ++i) {
        *pOutput++ = 0.5f * sinf(phase);
        phase += phaseChange;
    }
}

void FillWithSquareSimple(int numBuffers, float* outputBuffer) {
    float phase[NUM_VOICES];
    float phaseChange[NUM_VOICES];
    float freq[NUM_VOICES];
    float gain[NUM_VOICES];
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
    }
    freq[0] = MidiToFreq(60);
    freq[1] = MidiToFreq(63);
    freq[2] = MidiToFreq(67);
    freq[3] = MidiToFreq(70);
    for (int i = 0; i < NUM_VOICES; ++i) {
        phaseChange[i] = 2 * PI * freq[i] / SAMPLE_RATE;
    }

    float* pOutput = outputBuffer;
    for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            for (int voice = 0; voice < NUM_VOICES; ++voice) {
                float voicePhase = phase[voice];
                float v = -1.f + (voicePhase < PI) * 2.f;
                v *= gain[voice];
                *pOutput += v;
                voicePhase += phaseChange[voice];
                voicePhase -= (voicePhase > 2.f * PI) * (2.f * PI);
                phase[voice] = voicePhase;
            }
            ++pOutput;
        }
    }
}

void FillWithSquareSimd(int numBuffers, float* outputBuffer) {
    alignas(16) float phase[NUM_VOICES];
    alignas(16) float phaseChange[NUM_VOICES];
    alignas(16) float freq[NUM_VOICES];
    alignas(16) float gain[NUM_VOICES];
    alignas(16) float voiceOutput[NUM_VOICES];
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
    }
    freq[0] = MidiToFreq(60);
    freq[1] = MidiToFreq(63);
    freq[2] = MidiToFreq(67);
    freq[3] = MidiToFreq(70);
    for (int i = 0; i < NUM_VOICES; ++i) {
        phaseChange[i] = 2 * PI * freq[i] / SAMPLE_RATE;
    }

    __m128 phaseV = _mm_load_ps(phase);
    __m128 gainV = _mm_load_ps(gain);
    __m128 phaseChangeV = _mm_load_ps(phaseChange);
    __m128 doubleV = _mm_set1_ps(2.f);
    __m128 piV = _mm_set1_ps(PI);
    __m128 minus1V = _mm_set1_ps(-1.f);
    __m128 twoPiV = _mm_set1_ps(2.f * PI);

    float* pOutput = outputBuffer;
    for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            __m128 oscV = _mm_cmplt_ps(phaseV, piV);
            oscV = _mm_and_ps(oscV, doubleV);
            oscV = _mm_add_ps(oscV, minus1V);
            oscV = _mm_mul_ps(oscV, gainV);
            _mm_store_ps(voiceOutput, oscV);
            phaseV = _mm_add_ps(phaseV, phaseChangeV);
            _mm_store_ps(phase, phaseV);
            // abusing oscV register here to wrap the phase
            oscV = _mm_cmpgt_ps(phaseV, twoPiV);
            oscV = _mm_and_ps(oscV, twoPiV);
            phaseV = _mm_sub_ps(phaseV, oscV);

            for (int v = 0; v < NUM_VOICES; ++v) {
                *pOutput += voiceOutput[v];
            }
            ++pOutput;
        }
    }
}

int main() {

    float const totalTimeSecs = 5.f;
    int const numBuffers = (int) (ceilf(totalTimeSecs * SAMPLE_RATE) / SAMPLES_PER_BUFFER);

    float* outputBuffer = (float*)(calloc(numBuffers * SAMPLES_PER_BUFFER, sizeof(float)));

    clock_t t0 = clock();
    
    // FillWithSineSimple(numBuffers, outputBuffer);
    // FillWithSquareSimple(numBuffers, outputBuffer);
    FillWithSquareSimd(numBuffers, outputBuffer);

    clock_t t1 = clock();

    clock_t ticksElapsed = t1 - t0;
    float elapsedSecs = ((float) ticksElapsed) / CLOCKS_PER_SEC;
    printf("Time: %f s\n", elapsedSecs);

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = 1;
    format.sampleRate = SAMPLE_RATE;
    format.bitsPerSample = 32;
    drwav wav;
    drwav_init_file_write(&wav, "howdy.wav", &format, 0);

    int32_t buffer32[SAMPLES_PER_BUFFER];
    for (int i = 0; i < numBuffers * SAMPLES_PER_BUFFER; i += SAMPLES_PER_BUFFER) {
        drwav_f32_to_s32(buffer32, &(outputBuffer[i]), SAMPLES_PER_BUFFER);
        drwav_write_pcm_frames(&wav, SAMPLES_PER_BUFFER, buffer32);
    }

    drwav_uninit(&wav);

    free(outputBuffer);
    
    return 0;
}
