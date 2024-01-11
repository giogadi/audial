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
#define PI 3.141592653589793
#define PI2 PI*2

float MidiToFreq(int midi) {
    int const a4 = 69;  // 440Hz
    return 440.0f * pow(2.0f, (midi - a4) / 12.0f);
}

void Clampf(float* x, float min, float max) {
    if (*x < min) {
        *x = min;
    } else if (*x > max) {
        *x = max;
    }
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

typedef struct VA1Filter {    
    // coeffs
    float alpha;

    // state
    float sn;

    float output;
} VA1Filter;

void VA1Filter_Init(VA1Filter* s, float fc) {
    s->alpha = 0.f;
    s->sn = 0.f;
    s->output = 0.f;

    float halfSamplePeriod = 1.f / (2.f * SAMPLE_RATE);
    float g = tanf(PI2 * fc * halfSamplePeriod);
    s->alpha = g / (1.f + g);
}
void VA1Filter_Process(VA1Filter* s, float input) {
    float dn = input - s->sn;
    float vn = dn * s->alpha;
    s->output = vn + s->sn;
    s->sn = vn + s->output;
}
void VA1Filter_ProcessClamped(VA1Filter* s, float input) {
    float dn = input - s->sn;
    float vn = dn * s->alpha;
    s->output = vn + s->sn;
    Clampf(&s->output, -1.f, 1.f);
    s->sn = vn + s->output;
}

typedef struct MoogFilter {
    // coeffs
    float K;
    float alpha;
    float alpha0;
    float bassComp;
    float subFilterBeta[4];

    VA1Filter subFilter[4];

    float output;
    
} MoogFilter;

// Q is in [0,1]
void MoogFilter_Init(MoogFilter* s, float fc, float Q) {
    if (Q < 0.f) {
        Q = 0.f;
    } else if (Q > 1.f) {
        Q = 1.f;
    }

    // K must be in [0,4]
    s->K = Q * 4.f;
    s->alpha = 0.f;
    s->alpha0 = 1.f;
    s->bassComp = 1.f;
    s->output = 0.f;

    for (int i = 0; i < 4; ++i) {
        VA1Filter_Init(&(s->subFilter[i]), fc);
    }

    float halfSamplePeriod = 1.f / (2.f * SAMPLE_RATE);
    float g = tanf(PI2 * fc * halfSamplePeriod);
    float kernel = 1.f / (1.f + g);
    s->alpha = g * kernel;

    s->alpha0 = 1.f / (1.f + s->K*s->alpha*s->alpha*s->alpha*s->alpha);

    s->subFilterBeta[3] = kernel;
    s->subFilterBeta[2] = s->alpha * s->subFilterBeta[3];
    s->subFilterBeta[1] = s->alpha * s->subFilterBeta[2];
    s->subFilterBeta[0] = s->alpha * s->subFilterBeta[1];

    // maybe not necessary?
    for (int i = 0; i < 4; ++i) {
        s->subFilter[i].alpha = s->alpha;       
    }
}
void MoogFilter_Process(MoogFilter* s, float xn) {
    float sigma = 0.f;
    for (int i = 0; i < 4; ++i) {
        VA1Filter* subFlt = &(s->subFilter[i]);
        sigma += s->subFilterBeta[i] * subFlt->sn;
    }

    xn *= 1.f + s->bassComp * s->K;

    float u = s->alpha0 * (xn - s->K * sigma);
    VA1Filter_Process(&s->subFilter[0], u);
    VA1Filter_Process(&s->subFilter[1], s->subFilter[0].output);
    VA1Filter_Process(&s->subFilter[2], s->subFilter[1].output);
    VA1Filter_Process(&s->subFilter[3], s->subFilter[2].output);

    s->output = s->subFilter[3].output;
}


void FillWithVA1SquareSimple(int numBuffers, float* outputBuffer) {
    float phase[NUM_VOICES];
    float phaseChange[NUM_VOICES];
    float freq[NUM_VOICES];
    float gain[NUM_VOICES];
    VA1Filter filters[NUM_VOICES];
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
        VA1Filter_Init(&filters[i], 1000.f);
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
                VA1Filter* flt = &filters[voice];
                VA1Filter_Process(flt, v);
                *pOutput += flt->output;
                voicePhase += phaseChange[voice];
                voicePhase -= (voicePhase > 2.f * PI) * (2.f * PI);
                phase[voice] = voicePhase;
            }
            ++pOutput;
        }
    }
}

void FillWithMoogSquareSimple(int numBuffers, float* outputBuffer) {
    float phase[NUM_VOICES];
    float phaseChange[NUM_VOICES];
    float freq[NUM_VOICES];
    float gain[NUM_VOICES];
    MoogFilter filters[NUM_VOICES];
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
        MoogFilter_Init(&filters[i], 1000.f, 0.8f);
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
                MoogFilter* flt = &filters[voice];
                MoogFilter_Process(flt, v);
                v = flt->output * gain[voice];
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

void FillWithSquareSimd8(int numBuffers, float* outputBuffer) {
    alignas(32) float phase[2*NUM_VOICES];
    alignas(32) float phaseChange[2*NUM_VOICES];
    alignas(32) float freq[2*NUM_VOICES];
    alignas(32) float gain[2*NUM_VOICES];
    alignas(32) float voiceOutput[2*NUM_VOICES];
    for (int i = 0; i < 2*NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
    }
    freq[0] = MidiToFreq(60);
    freq[1] = MidiToFreq(63);
    freq[2] = MidiToFreq(67);
    freq[3] = MidiToFreq(70);
    freq[4] = MidiToFreq(72);
    freq[5] = MidiToFreq(75);
    freq[6] = MidiToFreq(79);
    freq[7] = MidiToFreq(82);
    for (int i = 0; i < 2*NUM_VOICES; ++i) {
        phaseChange[i] = 2 * PI * freq[i] / SAMPLE_RATE;
    }

    __m256 phaseV = _mm256_load_ps(phase);
    __m256 gainV = _mm256_load_ps(gain);
    __m256 phaseChangeV = _mm256_load_ps(phaseChange);
    __m256 doubleV = _mm256_set1_ps(2.f);
    __m256 piV = _mm256_set1_ps(PI);
    __m256 minus1V = _mm256_set1_ps(-1.f);
    __m256 twoPiV = _mm256_set1_ps(2.f * PI);

    float* pOutput = outputBuffer;
    for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            __m256 oscV = _mm256_cmp_ps(phaseV, piV, _CMP_LT_OS);
            oscV = _mm256_and_ps(oscV, doubleV);
            oscV = _mm256_add_ps(oscV, minus1V);
            oscV = _mm256_mul_ps(oscV, gainV);
            _mm256_store_ps(voiceOutput, oscV);
            phaseV = _mm256_add_ps(phaseV, phaseChangeV);
            _mm256_store_ps(phase, phaseV);
            // abusing oscV register here to wrap the phase
            oscV = _mm256_cmp_ps(phaseV, twoPiV, _CMP_GT_OS);
            oscV = _mm256_and_ps(oscV, twoPiV);
            phaseV = _mm256_sub_ps(phaseV, oscV);

            for (int v = 0; v < 2*NUM_VOICES; ++v) {
                *pOutput += voiceOutput[v];
            }
            ++pOutput;
        }
    }
}

void FillWithVA1SquareSimd(int numBuffers, float* outputBuffer) {
    alignas(16) float phase[NUM_VOICES];
    alignas(16) float phaseChange[NUM_VOICES];
    alignas(16) float freq[NUM_VOICES];
    alignas(16) float gain[NUM_VOICES];
    
    alignas(16) float fltFc[NUM_VOICES];
    alignas(16) float fltAlpha[NUM_VOICES];
    alignas(16) float fltSn[NUM_VOICES];
    alignas(16) float fltOut[NUM_VOICES];
    
    alignas(16) float voiceOutput[NUM_VOICES];
    
    freq[0] = MidiToFreq(60);
    freq[1] = MidiToFreq(63);
    freq[2] = MidiToFreq(67);
    freq[3] = MidiToFreq(70);
    float const halfSamplePeriod = 1.f / (2.f * SAMPLE_RATE);
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
        phaseChange[i] = 2 * PI * freq[i] / SAMPLE_RATE;

        fltFc[i] = 1000.f;
        float g = tanf(PI2 * fltFc[i] * halfSamplePeriod);
        fltAlpha[i] = g / (1.f + g);
        fltSn[i] = 0.f;
        fltOut[i] = 0.f;
    }

    __m128 phaseV = _mm_load_ps(phase);
    __m128 gainV = _mm_load_ps(gain);
    __m128 phaseChangeV = _mm_load_ps(phaseChange);
    __m128 doubleV = _mm_set1_ps(2.f);
    __m128 piV = _mm_set1_ps(PI);
    __m128 minus1V = _mm_set1_ps(-1.f);
    __m128 twoPiV = _mm_set1_ps(2.f * PI);

    __m128 fltAlphaV = _mm_load_ps(fltAlpha);
    __m128 fltSnV = _mm_load_ps(fltSn);

    float* pOutput = outputBuffer;
    for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            __m128 oscV = _mm_cmplt_ps(phaseV, piV);
            oscV = _mm_and_ps(oscV, doubleV);
            oscV = _mm_add_ps(oscV, minus1V);

            // Filter
            __m128 vnV = _mm_sub_ps(oscV, fltSnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, fltSnV);
            fltSnV = _mm_add_ps(vnV, oscV);

            oscV = _mm_mul_ps(oscV, gainV);
            
            _mm_store_ps(voiceOutput, oscV);
            phaseV = _mm_add_ps(phaseV, phaseChangeV);
            // _mm_store_ps(phase, phaseV);
            
            // abusing oscV register here to wrap the phase
            oscV = _mm_cmpgt_ps(phaseV, twoPiV);
            oscV = _mm_and_ps(oscV, twoPiV);
            phaseV = _mm_sub_ps(phaseV, oscV);

            for (int v = 0; v < NUM_VOICES; ++v) {
                *pOutput += voiceOutput[v];
            }
            ++pOutput;
        }
        // TODO: STORE out registers when we go back to processing one buffer at
        // a time.
    }
}

void FillWithMoogSquareSimd(int numBuffers, float* outputBuffer) {
    alignas(16) float phase[NUM_VOICES];
    alignas(16) float phaseChange[NUM_VOICES];
    alignas(16) float freq[NUM_VOICES];
    alignas(16) float gain[NUM_VOICES];

    alignas(16) float fltFc[NUM_VOICES];
    alignas(16) float fltK[NUM_VOICES];
    alignas(16) float fltAlpha[NUM_VOICES];
    alignas(16) float fltAlpha0[NUM_VOICES];
    alignas(16) float fltBassComp[NUM_VOICES];
    
    alignas(16) float flt0Sn[NUM_VOICES];
    alignas(16) float flt1Sn[NUM_VOICES];
    alignas(16) float flt2Sn[NUM_VOICES];
    alignas(16) float flt3Sn[NUM_VOICES];

    alignas(16) float flt0Beta[NUM_VOICES];
    alignas(16) float flt1Beta[NUM_VOICES];
    alignas(16) float flt2Beta[NUM_VOICES];
    alignas(16) float flt3Beta[NUM_VOICES];    

    alignas(16) float voiceOutput[NUM_VOICES];
    
    freq[0] = MidiToFreq(60);
    freq[1] = MidiToFreq(63);
    freq[2] = MidiToFreq(67);
    freq[3] = MidiToFreq(70);
    float const halfSamplePeriod = 1.f / (2.f * SAMPLE_RATE);
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
        phaseChange[i] = 2 * PI * freq[i] / SAMPLE_RATE;

        fltFc[i] = 1000.f;
        fltK[i] = 0.8f * 4.f;
        float g = tanf(PI2 * fltFc[i] * halfSamplePeriod);
        float kernel = 1.f / (1.f + g);
        fltAlpha[i] = g * kernel;
        fltAlpha0[i] = 1.f / (1.f + fltK[i] * fltAlpha[i] * fltAlpha[i] * fltAlpha[i] * fltAlpha[i]);
        fltBassComp[i] = 1.f;
        
        flt0Sn[i] = 0.f;
        flt1Sn[i] = 0.f;
        flt2Sn[i] = 0.f;
        flt3Sn[i] = 0.f;

        // TODO: this is multiplying a^4, which we can reuse for alpha0 above
        flt3Beta[i] = kernel;
        flt2Beta[i] = fltAlpha[i] * flt3Beta[i];
        flt1Beta[i] = fltAlpha[i] * flt2Beta[i];
        flt0Beta[i] = fltAlpha[i] * flt1Beta[i];
    }

    __m128 phaseV = _mm_load_ps(phase);
    __m128 gainV = _mm_load_ps(gain);
    __m128 phaseChangeV = _mm_load_ps(phaseChange);
    __m128 doubleV = _mm_set1_ps(2.f);
    __m128 piV = _mm_set1_ps(PI);
    __m128 plus1V = _mm_set1_ps(1.f);
    __m128 twoPiV = _mm_set1_ps(2.f * PI);

    __m128 fltKV = _mm_load_ps(fltK);
    __m128 fltAlphaV = _mm_load_ps(fltAlpha);
    __m128 fltAlpha0V = _mm_load_ps(fltAlpha0);
    __m128 fltBassCompV = _mm_load_ps(fltBassComp);
    
    __m128 flt0SnV = _mm_load_ps(flt0Sn);
    __m128 flt1SnV = _mm_load_ps(flt1Sn);
    __m128 flt2SnV = _mm_load_ps(flt2Sn);
    __m128 flt3SnV = _mm_load_ps(flt3Sn);
    
    __m128 flt0BetaV = _mm_load_ps(flt0Beta);
    __m128 flt1BetaV = _mm_load_ps(flt1Beta);
    __m128 flt2BetaV = _mm_load_ps(flt2Beta);
    __m128 flt3BetaV = _mm_load_ps(flt3Beta);

    float* pOutput = outputBuffer;
    for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            // square wave
            __m128 oscV = _mm_cmplt_ps(phaseV, piV);
            oscV = _mm_and_ps(oscV, doubleV);
            oscV = _mm_sub_ps(oscV, plus1V);

            __m128 sigma = _mm_set1_ps(0.f);
            __m128 vnV = _mm_mul_ps(flt0BetaV, flt0SnV);
            sigma = _mm_add_ps(sigma, vnV);
            vnV = _mm_mul_ps(flt1BetaV, flt1SnV);
            sigma = _mm_add_ps(sigma, vnV);
            vnV = _mm_mul_ps(flt2BetaV, flt2SnV);
            sigma = _mm_add_ps(sigma, vnV);
            vnV = _mm_mul_ps(flt3BetaV, flt3SnV);
            sigma = _mm_add_ps(sigma, vnV);

            // prepare filter input
            vnV = _mm_mul_ps(fltBassCompV, fltKV);
            vnV = _mm_add_ps(vnV, plus1V);
            oscV = _mm_mul_ps(oscV, vnV);
            vnV = _mm_mul_ps(fltKV, sigma);
            oscV = _mm_sub_ps(oscV, vnV);
            oscV = _mm_mul_ps(oscV, fltAlpha0V);

            // Filter0
            vnV = _mm_sub_ps(oscV, flt0SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt0SnV);
            flt0SnV = _mm_add_ps(vnV, oscV);

            // Filter1
            vnV = _mm_sub_ps(oscV, flt1SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt1SnV);
            flt1SnV = _mm_add_ps(vnV, oscV);

            // Filter2
            vnV = _mm_sub_ps(oscV, flt2SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt2SnV);
            flt2SnV = _mm_add_ps(vnV, oscV);

            // Filter3
            vnV = _mm_sub_ps(oscV, flt3SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt3SnV);
            flt3SnV = _mm_add_ps(vnV, oscV);

            oscV = _mm_mul_ps(oscV, gainV);
            
            _mm_store_ps(voiceOutput, oscV);
            phaseV = _mm_add_ps(phaseV, phaseChangeV);
            // _mm_store_ps(phase, phaseV);
            
            // abusing oscV register here to wrap the phase
            oscV = _mm_cmpgt_ps(phaseV, twoPiV);
            oscV = _mm_and_ps(oscV, twoPiV);
            phaseV = _mm_sub_ps(phaseV, oscV);

            for (int v = 0; v < NUM_VOICES; ++v) {
                *pOutput += voiceOutput[v];
            }
            ++pOutput;
        }
        // TODO: STORE out registers when we go back to processing one buffer at
        // a time.
    }
}

void FillWithMoogSquareSimd2(int numBuffers, float* outputBuffer) {
    alignas(16) float phase[NUM_VOICES];
    alignas(16) float phaseChange[NUM_VOICES];
    alignas(16) float freq[NUM_VOICES];
    alignas(16) float gain[NUM_VOICES];

    alignas(16) float fltFc[NUM_VOICES];
    alignas(16) float fltK[NUM_VOICES];
    alignas(16) float fltAlpha[NUM_VOICES];
    alignas(16) float fltAlpha0[NUM_VOICES];
    alignas(16) float fltBassComp[NUM_VOICES];
    
    alignas(16) float flt0Sn[NUM_VOICES];
    alignas(16) float flt1Sn[NUM_VOICES];
    alignas(16) float flt2Sn[NUM_VOICES];
    alignas(16) float flt3Sn[NUM_VOICES];

    alignas(16) float flt0Beta[NUM_VOICES];
    alignas(16) float flt1Beta[NUM_VOICES];
    alignas(16) float flt2Beta[NUM_VOICES];
    alignas(16) float flt3Beta[NUM_VOICES];    

    alignas(16) float voiceOutput[NUM_VOICES * SAMPLES_PER_BUFFER];
    
    freq[0] = MidiToFreq(60);
    freq[1] = MidiToFreq(63);
    freq[2] = MidiToFreq(67);
    freq[3] = MidiToFreq(70);
    float const halfSamplePeriod = 1.f / (2.f * SAMPLE_RATE);
    for (int i = 0; i < NUM_VOICES; ++i) {
        phase[i] = 0.f;
        gain[i] = 0.1f;
        phaseChange[i] = 2 * PI * freq[i] / SAMPLE_RATE;

        fltFc[i] = 1000.f;
        fltK[i] = 0.8f * 4.f;
        float g = tanf(PI2 * fltFc[i] * halfSamplePeriod);
        float kernel = 1.f / (1.f + g);
        fltAlpha[i] = g * kernel;
        fltAlpha0[i] = 1.f / (1.f + fltK[i] * fltAlpha[i] * fltAlpha[i] * fltAlpha[i] * fltAlpha[i]);
        fltBassComp[i] = 1.f;
        
        flt0Sn[i] = 0.f;
        flt1Sn[i] = 0.f;
        flt2Sn[i] = 0.f;
        flt3Sn[i] = 0.f;

        // TODO: this is multiplying a^4, which we can reuse for alpha0 above
        flt3Beta[i] = kernel;
        flt2Beta[i] = fltAlpha[i] * flt3Beta[i];
        flt1Beta[i] = fltAlpha[i] * flt2Beta[i];
        flt0Beta[i] = fltAlpha[i] * flt1Beta[i];
    }

    __m128 gainV = _mm_load_ps(gain);
    __m128 phaseChangeV = _mm_load_ps(phaseChange);
    __m128 doubleV = _mm_set1_ps(2.f);
    __m128 piV = _mm_set1_ps(PI);
    __m128 plus1V = _mm_set1_ps(1.f);
    __m128 twoPiV = _mm_set1_ps(2.f * PI);

    __m128 fltKV = _mm_load_ps(fltK);
    __m128 fltAlphaV = _mm_load_ps(fltAlpha);
    __m128 fltAlpha0V = _mm_load_ps(fltAlpha0);
    __m128 fltBassCompV = _mm_load_ps(fltBassComp);
    
    __m128 flt0SnV = _mm_load_ps(flt0Sn);
    __m128 flt1SnV = _mm_load_ps(flt1Sn);
    __m128 flt2SnV = _mm_load_ps(flt2Sn);
    __m128 flt3SnV = _mm_load_ps(flt3Sn);
    
    __m128 flt0BetaV = _mm_load_ps(flt0Beta);
    __m128 flt1BetaV = _mm_load_ps(flt1Beta);
    __m128 flt2BetaV = _mm_load_ps(flt2Beta);
    __m128 flt3BetaV = _mm_load_ps(flt3Beta);

    float* pOutput = outputBuffer;
    for (int bufferIx = 0; bufferIx < numBuffers; ++bufferIx) {
        __m128 phaseV = _mm_load_ps(phase);
        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            // square wave
            __m128 oscV = _mm_cmplt_ps(phaseV, piV);
            oscV = _mm_and_ps(oscV, doubleV);
            oscV = _mm_sub_ps(oscV, plus1V);

            oscV = _mm_mul_ps(oscV, gainV);
            
            _mm_store_ps(&voiceOutput[NUM_VOICES*i], oscV);
            phaseV = _mm_add_ps(phaseV, phaseChangeV);

            // abusing oscV register here to wrap the phase
            oscV = _mm_cmpgt_ps(phaseV, twoPiV);
            oscV = _mm_and_ps(oscV, twoPiV);
            phaseV = _mm_sub_ps(phaseV, oscV);
        }
        _mm_store_ps(phase, phaseV);

        for (int i = 0; i < SAMPLES_PER_BUFFER; ++i) {
            __m128 oscV = _mm_load_ps(&voiceOutput[NUM_VOICES*i]);

            __m128 sigma = _mm_set1_ps(0.f);
            __m128 vnV = _mm_mul_ps(flt0BetaV, flt0SnV);
            sigma = _mm_add_ps(sigma, vnV);
            vnV = _mm_mul_ps(flt1BetaV, flt1SnV);
            sigma = _mm_add_ps(sigma, vnV);
            vnV = _mm_mul_ps(flt2BetaV, flt2SnV);
            sigma = _mm_add_ps(sigma, vnV);
            vnV = _mm_mul_ps(flt3BetaV, flt3SnV);
            sigma = _mm_add_ps(sigma, vnV);

            // prepare filter input
            vnV = _mm_mul_ps(fltBassCompV, fltKV);
            vnV = _mm_add_ps(vnV, plus1V);
            oscV = _mm_mul_ps(oscV, vnV);
            vnV = _mm_mul_ps(fltKV, sigma);
            oscV = _mm_sub_ps(oscV, vnV);
            oscV = _mm_mul_ps(oscV, fltAlpha0V);

            // Filter0
            vnV = _mm_sub_ps(oscV, flt0SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt0SnV);
            flt0SnV = _mm_add_ps(vnV, oscV);

            // Filter1
            vnV = _mm_sub_ps(oscV, flt1SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt1SnV);
            flt1SnV = _mm_add_ps(vnV, oscV);

            // Filter2
            vnV = _mm_sub_ps(oscV, flt2SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt2SnV);
            flt2SnV = _mm_add_ps(vnV, oscV);

            // Filter3
            vnV = _mm_sub_ps(oscV, flt3SnV);
            vnV = _mm_mul_ps(vnV, fltAlphaV);
            oscV = _mm_add_ps(vnV, flt3SnV);
            flt3SnV = _mm_add_ps(vnV, oscV);
            
            _mm_store_ps(&voiceOutput[NUM_VOICES*i], oscV);

            for (int v = 0; v < NUM_VOICES; ++v) {
                *pOutput += voiceOutput[NUM_VOICES*i + v];
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
    // FillWithSquareSimd8(numBuffers, outputBuffer);

    // FillWithVA1SquareSimple(numBuffers, outputBuffer);
    // FillWithVA1SquareSimd(numBuffers, outputBuffer);

    // FillWithMoogSquareSimple(numBuffers, outputBuffer);
    // FillWithMoogSquareSimd(numBuffers, outputBuffer);
    // FillWithMoogSquareSimd2(numBuffers, outputBuffer);

    clock_t t1 = clock();

    clock_t ticksElapsed = t1 - t0;
    float elapsedSecs = ((float) ticksElapsed) / CLOCKS_PER_SEC;
    printf("Time: %f s\n", elapsedSecs);

    // CLAMP
    for (int i = 0; i < numBuffers * SAMPLES_PER_BUFFER; ++i) {
        Clampf(&outputBuffer[i], -0.99f, 0.99f);
    }

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
