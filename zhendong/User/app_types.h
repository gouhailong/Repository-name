#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

typedef struct {
    float ax;
    float ay;
    float az;
} AccelData;

typedef struct {
    float rms;
    float p2p;
    float peak;
    float crest;
    float mean;
} VibFeature;

typedef enum {
    VIB_STATE_LEARNING = 0,
    VIB_STATE_NORMAL,
    VIB_STATE_WARNING,
    VIB_STATE_ALARM,
    VIB_STATE_SENSOR_ERROR
} VibState;

typedef struct {
    uint32_t seq;
    VibFeature feature;
    uint8_t score;
    VibState state;
    uint32_t dropped_windows;
} VibResult;

#endif

