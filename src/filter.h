#pragma once

#include <cstring>
#include <cstdint>

#define MOOG_SUBFILTERS 4

namespace filter {

enum { FLT1, FLT2, FLT3, FLT4 };

enum { LPF1, LPF2, LPF3, LPF4, HPF1, HPF2, HPF3, HPF4, BPF2, BPF4, BSF2, BSF4, 
		APF1, APF2, ANM_LPF1, ANM_LPF2, ANM_LPF3, ANM_LPF4, NUM_FILTER_OUTPUTS };

struct FilterOutput
{
    FilterOutput() { clearData(); }
    double filter[NUM_FILTER_OUTPUTS];
    void clearData()
    {
        memset(&filter[0], 0, sizeof(double) * NUM_FILTER_OUTPUTS);
    }
};

struct VA1Coeffs
{
    // --- filter coefficients
    double alpha = 0.0;			///< alpha is (wcT/2)
    double beta = 1.0;			///< beta value, not used
};

class VA1Filter
{
public:
    // --- these match SynthModule names
    virtual bool reset(double _sampleRate);
    virtual bool update();
    virtual FilterOutput* process(double xn); 
    virtual void setFilterParams(double _fc, double _Q);

    // --- set coeffs directly, bypassing coeff calculation
    void setAlpha(double _alpha) { coeffs.alpha = _alpha; }
    void setBeta(double _beta) { coeffs.beta = _beta; }
    void setCoeffs(VA1Coeffs& _coeffs) {
        coeffs = _coeffs;
    }

    void copyCoeffs(VA1Filter& destination) { 
        destination.setCoeffs(coeffs);
    }
		
    // --- added for MOOG & K35, need access to this output value, scaled by beta
    double getFBOutput() { return coeffs.beta * sn; }

protected:
    FilterOutput output;
    double sampleRate = 44100.0;				///< current sample rate
    double halfSamplePeriod = 1.0;
    double fc = 0.0;

    // --- state storage
    double sn = 0.0;						///< state variables

    // --- filter coefficients
    VA1Coeffs coeffs;
};

struct VAMoogCoeffs
{
    // --- filter coefficients
    double K = 1.0;			///< beta value, not used
    double alpha = 0.0;			///< alpha is (wcT/2)
    double alpha0 = 1.0;			///< beta value, not used
    double sigma = 1.0;			///< beta value, not used
    double bassComp = 1.0;			///< beta value, not used
    double g = 1.0;			///< beta value, not used

    // --- these are to minimize repeat calculations for left/right pairs
    double subFilterBeta[MOOG_SUBFILTERS] = { 0.0, 0.0, 0.0, 0.0 };
};

class VAMoogFilter
{
public:

    // --- these match SynthModule names
    virtual bool reset(double _sampleRate);
    virtual bool update();
    virtual FilterOutput* process(double xn); 
    virtual void setFilterParams(double _fc, double _Q);

    // --- set coeffs directly, bypassing coeff calculation
    void setCoeffs(const VAMoogCoeffs& _coeffs) {
        coeffs = _coeffs;

        // --- four sync-tuned filters
        for (uint32_t i = 0; i < MOOG_SUBFILTERS; i++)
			{
				// --- set alpha directly
				subFilter[i].setAlpha(coeffs.alpha);
				subFilterFGN[i].setAlpha(coeffs.alpha);

				// --- set beta directly
				subFilter[i].setBeta(coeffs.subFilterBeta[i]);
				subFilterFGN[i].setBeta(coeffs.subFilterBeta[i]);
			}
    }

    void copyCoeffs(VAMoogFilter& destination) {
        destination.setCoeffs(coeffs);
    }

protected:
    FilterOutput output;
    VA1Filter subFilter[MOOG_SUBFILTERS];
    VA1Filter subFilterFGN[MOOG_SUBFILTERS];
    double sampleRate = 44100.0;				///< current sample rate
    double halfSamplePeriod = 1.0;
    double fc = 0.0;

    // --- filter coefficients
    VAMoogCoeffs coeffs;
};

}