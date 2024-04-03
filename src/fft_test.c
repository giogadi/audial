#include <stdio.h>
#include <fftw3.h>
#include <math.h>
#include <string.h>

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
    int bandsPerOctave = 1; 

    int numOctaves = 0;
    int startDftBin = 0;
    for (int ii = 1; ii < fftSize; ii = ii << 1) {
        float f = fftFreqs[ii - 1];
        if (f >= minBandwidthPerOctave) {
            startDftBin = ii;
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
    float logAvgBins[numOctaves * bandsPerOctave];
    memset(logAvgBins, 0, sizeof(float) * numLogAvgBins);
    float logAvgBinFreqs[numLogAvgBins];
    int prevDftIndex = 0;
    for (int octaveIx = 0; octaveIx < numOctaves; ++octaveIx) {
        int dftBinIx = (startDftBin << octaveIx) - 1;
        printf("%d: %d to %d (%f to %f)\n", octaveIx, prevDftIndex, dftBinIx, fftFreqs[prevDftIndex], fftFreqs[dftBinIx]);
        for (int jj = prevDftIndex; jj <= dftBinIx; ++jj) {
            logAvgBins[octaveIx] += fftAmps[jj]; 
        }
        logAvgBins[octaveIx] /= (dftBinIx - prevDftIndex) + 1;
        logAvgBinFreqs[octaveIx] = fftFreqs[dftBinIx];
        prevDftIndex = dftBinIx + 1; 
    } 

    printf("*********\n");
    for (int i = 0; i < numLogAvgBins; ++i) {
        printf("[%d] %f %f\n", i, logAvgBinFreqs[i], logAvgBins[i]);
    }


    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return 0;
}
