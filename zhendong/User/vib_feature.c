#include "vib_feature.h"
#include <math.h>

float VibFeature_VectorMagnitude(const AccelData *a)
{
    return sqrtf(a->ax * a->ax + a->ay * a->ay + a->az * a->az);
}

void VibFeature_Calc(const float *x, uint16_t n, VibFeature *out)
{
    float sum = 0.0f;
    float energy = 0.0f;
    float max_v = -9999.0f;
    float min_v = 9999.0f;
    float mean;
    uint16_t i;

    for (i = 0U; i < n; i++) {
        sum += x[i];
    }
    mean = sum / (float)n;

    for (i = 0U; i < n; i++) {
        float v = x[i] - mean;
        energy += v * v;
        if (v > max_v) max_v = v;
        if (v < min_v) min_v = v;
    }

    out->mean = mean;
    out->rms = sqrtf(energy / (float)n);
    out->p2p = max_v - min_v;
    out->peak = fabsf(max_v) > fabsf(min_v) ? fabsf(max_v) : fabsf(min_v);
    out->crest = out->rms > 0.0001f ? out->peak / out->rms : 0.0f;
}

