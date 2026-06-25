#ifndef VIB_DETECT_H
#define VIB_DETECT_H

#include "app_types.h"

typedef struct {
    uint16_t learn_count;
    float base_rms;
    float base_p2p;
    float base_crest;
    uint8_t bad_count;
    uint8_t good_count;
    VibState state;
} VibDetector;

void VibDetect_Init(VibDetector *det);
VibResult VibDetect_Update(VibDetector *det, const VibFeature *feature, uint32_t seq, uint32_t dropped);
const char *VibDetect_StateText(VibState state);

#endif

