#include <stdio.h>
#include <fftw3.h>
#include <math.h>

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
        in[i] = 0.95 * sin(phase0);
        in[i] += 0.95 * sin(phase1);
        phase0 += phaseChange0;
        phase1 += phaseChange1;
    }
    
    fftw_plan p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

    fftw_execute(p);

    for (int i = 0; i < N / 2 + 1; ++i) {
        float a = (float)i / N;
        float b = (float)i / (N / sampleRate);
        float m = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
        float r = out[i][0] / N;
        float img = out[i][1] / N;
        float v = r*r + img*img;
        printf("%f %f (%f,%f,%f,%f,%f)\n", a, b, out[i][0], out[i][1], m, v, sqrt(v));
    }     

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return 0;
}
