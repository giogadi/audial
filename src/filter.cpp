#include "filter.h"

#include <cmath>

namespace filter {

constexpr float kTwoPi = 2.0*3.14159265358979323846264338327950288419716939937510582097494459230781640628620899;

constexpr float GUI_Q_MIN = 1.0;
constexpr float GUI_Q_MAX = 10.0;
constexpr float MOOG_Q_SLOPE = (4.0 - 0.0) / (GUI_Q_MAX - GUI_Q_MIN);

inline void boundValue(float& value, float minValue, float maxValue)
{
    const float t = value < minValue ? minValue : value;
    value = t > maxValue ? maxValue : t;
}

inline void mapfloatValue(float& value, float min, float max, float minMap, float maxMap)
{
    // --- bound to limits
    boundValue(value, min, max);
    float mapped = ((value - min) / (max - min)) * (maxMap - minMap) + minMap;
    value = mapped;
}

inline void mapfloatValue(float& value, float min, float minMap, float slope)
{
    // --- bound to limits
    value = minMap + slope * (value - min);
}

bool VA1Filter::reset(float _sampleRate)
{
    sampleRate = _sampleRate;
    halfSamplePeriod = 1.0 / (2.0 * sampleRate);
    sn = 0.0;
    output.clearData();
    return true;
}

void VA1Filter::setFilterParams(float _fc, float _Q)
{
    if (fc != _fc)
		{
			fc = _fc;
			update();
		}
}

bool VA1Filter::update()
{
    float g = tan(kTwoPi*fc*halfSamplePeriod); // (2.0 / T)*tan(wd*T / 2.0);
    coeffs.alpha = g / (1.0 + g);

    return true;
}

FilterOutput* VA1Filter::process(float xn)
{
    // --- create vn node
    float vn = (xn - sn)*coeffs.alpha;

    // --- form LP output
    output.filter[LPF1] = vn + sn;

    // --- form the HPF = INPUT = LPF
    // output.filter[HPF1] = xn - output.filter[LPF1];

    // --- form the APF = LPF - HPF
    // output.filter[APF1] = output.filter[LPF1] - output.filter[HPF1];

    // --- anm output
    // output.filter[ANM_LPF1] = output.filter[LPF1] + coeffs.alpha*output.filter[HPF1];

    // --- update memory
    sn = vn + output.filter[LPF1];

    return &output;
}

/** reset members to initialized state */
bool VAMoogFilter::reset(float _sampleRate)
{
    sampleRate = _sampleRate;
    halfSamplePeriod = 1.0 / (2.0 * sampleRate);

    for (uint32_t i = 0; i < MOOG_SUBFILTERS; i++)
		{
			subFilter[i].reset(_sampleRate);
			// subFilterFGN[i].reset(_sampleRate);
		}

    output.clearData();
    return true;
}

void VAMoogFilter::setFilterParams(float _fc, float _Q)
{
    // --- use mapping function for Q -> K
    mapfloatValue(_Q, 1.0, 0.0, MOOG_Q_SLOPE);

    if (fc != _fc || coeffs.K != _Q)
		{
			fc = _fc;
			coeffs.K = _Q;
			update();
		}
}

bool VAMoogFilter::update()
{
    coeffs.g = tan(kTwoPi*fc*halfSamplePeriod); // (2.0 / T)*tan(wd*T / 2.0);
    coeffs.alpha = coeffs.g / (1.0 + coeffs.g);

    // --- alpha0 
    coeffs.alpha0 = 1.0 / (1.0 + coeffs.K*coeffs.alpha*coeffs.alpha*coeffs.alpha*coeffs.alpha);
    float kernel = 1.0 / (1.0 + coeffs.g);

    // --- pre-calculate for distributing to subfilters
    coeffs.subFilterBeta[FLT4] = kernel;
    coeffs.subFilterBeta[FLT3] = coeffs.alpha * coeffs.subFilterBeta[FLT4];
    coeffs.subFilterBeta[FLT2] = coeffs.alpha * coeffs.subFilterBeta[FLT3];
    coeffs.subFilterBeta[FLT1] = coeffs.alpha * coeffs.subFilterBeta[FLT2];

    // --- four sync-tuned filters
    for (uint32_t i = 0; i < MOOG_SUBFILTERS; i++)
		{
			// --- set alpha - no calculation required
			subFilter[i].setAlpha(coeffs.alpha);
			// subFilterFGN[i].setAlpha(coeffs.alpha);

			// --- set beta - no calculation required
			subFilter[i].setBeta(coeffs.subFilterBeta[i]);
			// subFilterFGN[i].setBeta(coeffs.subFilterBeta[i]);
		}

    return true;
}

FilterOutput* VAMoogFilter::process(float xn)
{
    // --- 4th order MOOG:
    float sigma = 0.0;

    for (uint32_t i = 0; i < MOOG_SUBFILTERS; i++)
        sigma += subFilter[i].getFBOutput();

    // --- gain comp 
    xn *= 1.0 + coeffs.bassComp*coeffs.K; // --- bassComp is hard coded

    // --- now figure out u(n) = alpha0*[x(n) - K*sigma]
    float u = coeffs.alpha0*(xn - coeffs.K * sigma);

    // --- send u -> LPF1 and then cascade the outputs to form y(n)
    FilterOutput* subFltOut[4];
    // FilterOutput* subFltOutFGN[4];

    subFltOut[FLT1] = subFilter[FLT1].process(u);
    subFltOut[FLT2] = subFilter[FLT2].process(subFltOut[FLT1]->filter[LPF1]);
    subFltOut[FLT3] = subFilter[FLT3].process(subFltOut[FLT2]->filter[LPF1]);
    subFltOut[FLT4] = subFilter[FLT4].process(subFltOut[FLT3]->filter[LPF1]);

    // --- optional outputs 1,2,3
    /*output.filter[LPF1] = subFltOut[FLT1]->filter[LPF1];
    output.filter[LPF2] = subFltOut[FLT2]->filter[LPF1];
    output.filter[LPF3] = subFltOut[FLT3]->filter[LPF1];*/

    // --- MOOG LP4 output
    output.filter[LPF4] = subFltOut[FLT4]->filter[LPF1];

    // --- OPTIONAL: analog nyquist matched version
    //subFltOutFGN[FLT1] = subFilterFGN[FLT1].process(u);
    //subFltOutFGN[FLT2] = subFilterFGN[FLT2].process(subFltOutFGN[FLT1]->filter[ANM_LPF1]);
    //subFltOutFGN[FLT3] = subFilterFGN[FLT3].process(subFltOutFGN[FLT2]->filter[ANM_LPF1]);
    //subFltOutFGN[FLT4] = subFilterFGN[FLT4].process(subFltOutFGN[FLT3]->filter[ANM_LPF1]);

    //// --- optional outputs 1,2,3
    //output.filter[ANM_LPF1] = subFltOutFGN[FLT1]->filter[ANM_LPF1];
    //output.filter[ANM_LPF2] = subFltOutFGN[FLT2]->filter[ANM_LPF1];
    //output.filter[ANM_LPF3] = subFltOutFGN[FLT3]->filter[ANM_LPF1];

    //// --- MOOG LP4 output
    //output.filter[ANM_LPF4] = subFltOutFGN[FLT4]->filter[ANM_LPF1];

    return &output;
}

}
