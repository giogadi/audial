#include <stdio.h>
#include <fftw3.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int main() {

    int N = 512;

    double* in = fftw_malloc(sizeof(double) * N);
    fftw_complex* out = fftw_malloc(sizeof(fftw_complex) * N);

    float sampleRate = 44100.f;
    float baseF0 = 440.f;
    float baseF1 = 2 * baseF0;
    float phase0 = 0.f;
    float phase1 = 0.f;
    float phaseChange0 = baseF0 * 2 * M_PI / sampleRate;
    float phaseChange1 = baseF1 * 2 * M_PI / sampleRate;

    for (int i = 0; i < N; ++i) {
        in[i] = 0.5 * sin(phase0);
        in[i] += 0.5 * sin(phase1);
        phase0 += phaseChange0;
        phase1 += phaseChange1;
    }
    
    fftw_plan p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

    fftw_execute(p);

    // int fftSize = N / 2 + 1;
    int fftSize = N / 2;
    double fftAmps[fftSize];
    double fftFreqs[fftSize];
    for (int i = 0; i < fftSize; ++i) {
        // float deltaF = (sampleRate * 0.5f) / fftSize;
        // float freq = (float) i * deltaF;
        float freq = (i+1) * sampleRate / N;
        float r = out[i+1][0];
        float img = out[i+1][1];
        float amp = 2 * sqrt(r*r + img*img) / N;
        fftAmps[i] = amp;
        fftFreqs[i] = freq;
        // printf("[%d] %f: %f\n", i, freq, amp);
    }

    int minBandwidthPerOctave = 200;
    int bandsPerOctave = 10; 

    int numOctaves = 0;
    int startDftBin = 0;
    for (int ii = 1; ii < fftSize; ii = ii << 1) {
        float f = fftFreqs[ii - 1];
        if (f >= minBandwidthPerOctave) {
            startDftBin = ii - 1;
            int x = ii;
            int count = 0;
            while (x <= fftSize) {
                x = x << 1;
                ++count;
            }
            numOctaves = count;
            break;
        }
    }

    int numLogAvgBins = numOctaves * bandsPerOctave;
    float* logAvgBins = calloc(numLogAvgBins, sizeof(float));
    float* logAvgBinMinFreqs = calloc(numLogAvgBins, sizeof(float));
    float* logAvgBinMaxFreqs = calloc(numLogAvgBins, sizeof(float));

    float octaveMinFreq = 0.f;
    float octaveMaxFreq = fftFreqs[startDftBin];
    int dftIx = startDftBin;
    float const dftBinHalfBw = (1.f / N) * sampleRate / 2.f;
    for (int octaveIx = 0; octaveIx < numOctaves; ++octaveIx) {
        float octaveBw = octaveMaxFreq - octaveMinFreq;
        float logBinBw = octaveBw / bandsPerOctave;
        float logBinMinFreq = octaveMinFreq;
        for (int bandIx = 0; bandIx < bandsPerOctave; ++bandIx) {
            float logBinMaxFreq = logBinMinFreq + logBinBw;
            
            logAvgBinMinFreqs[octaveIx*bandsPerOctave + bandIx] = logBinMinFreq;
            logAvgBinMaxFreqs[octaveIx*bandsPerOctave + bandIx] = logBinMaxFreq;

            int count = 0;
            while (dftIx < fftSize) {
                float dftCenter = fftFreqs[dftIx];
                // float dftBinMinFreq = dftCenter - dftBinHalfBw;
                float dftBinMaxFreq = dftCenter + dftBinHalfBw;
                // bool overlap = !(dftBinMinFreq > logBinMaxFreq || dftBinMaxFreq < logBinMinFreq);
                // if (overlap) {
                //     logAvgBins[octaveIx*bandsPerOctave + bandIx] += fftAmps[dftIx];
                //    ++count;
                // }
                logAvgBins[octaveIx * bandsPerOctave + bandIx] += fftAmps[dftIx];
                ++count;
                if (dftBinMaxFreq < logBinMaxFreq) {
                    ++dftIx;
                } else {
                    break;
                }
            }
            if (count > 1) {
                logAvgBins[octaveIx*bandsPerOctave + bandIx] /= count;          
            }

            logBinMinFreq = logBinMaxFreq;
        }

        octaveMinFreq = octaveMaxFreq;
        octaveMaxFreq *= 2.f; 
    }

    printf("*********\n");
    for (int i = 0; i < numLogAvgBins; ++i) {
        printf("[%d] %f-%f %f\n", i, logAvgBinMinFreqs[i], logAvgBinMaxFreqs[i], logAvgBins[i]);
    }


    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return 0;
}
