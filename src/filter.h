#pragma once

#include <cstring>
#include <cstdint>

#define MOOG_SUBFILTERS 4

namespace filter {

enum { FLT1, FLT2, FLT3, FLT4 };

enum { LPF1, LPF4, NUM_FILTER_OUTPUTS };

struct FilterOutput
{
    FilterOutput() { clearData(); }
    float filter[NUM_FILTER_OUTPUTS];
    void clearData()
    {
        memset(&filter[0], 0, sizeof(float) * NUM_FILTER_OUTPUTS);
    }
};

struct VA1Coeffs
{
    // --- filter coefficients
    float alpha = 0.0;			///< alpha is (wcT/2)
    float beta = 1.0;			///< beta value, not used
};

class VA1Filter
{
public:
    // --- these match SynthModule names
    bool reset(float _sampleRate);
    bool update();
    FilterOutput* process(float xn); 
    void setFilterParams(float _fc, float _Q);

    // --- set coeffs directly, bypassing coeff calculation
    void setAlpha(float _alpha) { coeffs.alpha = _alpha; }
    void setBeta(float _beta) { coeffs.beta = _beta; }
    void setCoeffs(VA1Coeffs& _coeffs) {
        coeffs = _coeffs;
    }

    void copyCoeffs(VA1Filter& destination) { 
        destination.setCoeffs(coeffs);
    }
		
    // --- added for MOOG & K35, need access to this output value, scaled by beta
    float getFBOutput() { return coeffs.beta * sn; }

protected:
    FilterOutput output;
    float sampleRate = 44100.0;				///< current sample rate
    float halfSamplePeriod = 1.0;
    float fc = 0.0;

    // --- state storage
    float sn = 0.0;						///< state variables

    // --- filter coefficients
    VA1Coeffs coeffs;
};

struct VAMoogCoeffs
{
    // --- filter coefficients
    float K = 1.0;			///< beta value, not used
    float alpha = 0.0;			///< alpha is (wcT/2)
    float alpha0 = 1.0;			///< beta value, not used
    float sigma = 1.0;			///< beta value, not used
    float bassComp = 1.0;			///< beta value, not used
    float g = 1.0;			///< beta value, not used

    // --- these are to minimize repeat calculations for left/right pairs
    float subFilterBeta[MOOG_SUBFILTERS] = { 0.0, 0.0, 0.0, 0.0 };
};

class VAMoogFilter
{
public:

    // --- these match SynthModule names
    bool reset(float _sampleRate);
    bool update();
    FilterOutput* process(float xn); 
    void setFilterParams(float _fc, float _Q);

    // --- set coeffs directly, bypassing coeff calculation
    void setCoeffs(const VAMoogCoeffs& _coeffs) {
        coeffs = _coeffs;

        // --- four sync-tuned filters
        for (uint32_t i = 0; i < MOOG_SUBFILTERS; i++)
			{
				// --- set alpha directly
				subFilter[i].setAlpha(coeffs.alpha);
				// subFilterFGN[i].setAlpha(coeffs.alpha);

				// --- set beta directly
				subFilter[i].setBeta(coeffs.subFilterBeta[i]);
				// subFilterFGN[i].setBeta(coeffs.subFilterBeta[i]);
			}
    }

    void copyCoeffs(VAMoogFilter& destination) {
        destination.setCoeffs(coeffs);
    }

protected:
    FilterOutput output;
    VA1Filter subFilter[MOOG_SUBFILTERS];
    // VA1Filter subFilterFGN[MOOG_SUBFILTERS];
    float sampleRate = 44100.0;				///< current sample rate
    float halfSamplePeriod = 1.0;
    float fc = 0.0;

    // --- filter coefficients
    VAMoogCoeffs coeffs;
};

}
