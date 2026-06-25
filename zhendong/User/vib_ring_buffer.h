#ifndef VIB_RING_BUFFER_H
#define VIB_RING_BUFFER_H

#include <stdint.h>

typedef struct {
    float *data;
    uint16_t size;
    uint16_t write_index;
    uint16_t count;
    uint32_t total_written;
} VibRingBuffer;

void VibRing_Init(VibRingBuffer *rb, float *storage, uint16_t size);
void VibRing_Push(VibRingBuffer *rb, float value);
uint8_t VibRing_IsFull(const VibRingBuffer *rb);
void VibRing_CopyLatest(const VibRingBuffer *rb, float *out, uint16_t n);

#endif

