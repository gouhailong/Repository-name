#ifndef VIB_FEATURE_H
#define VIB_FEATURE_H

#include "app_types.h"
#include <stdint.h>

void VibFeature_Calc(const float *x, uint16_t n, VibFeature *out);
float VibFeature_VectorMagnitude(const AccelData *a);

#endif

