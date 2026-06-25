#include "vib_ring_buffer.h"

void VibRing_Init(VibRingBuffer *rb, float *storage, uint16_t size)
{
    rb->data = storage;
    rb->size = size;
    rb->write_index = 0U;
    rb->count = 0U;
    rb->total_written = 0U;
}

void VibRing_Push(VibRingBuffer *rb, float value)
{
    rb->data[rb->write_index] = value;
    rb->write_index++;
    if (rb->write_index >= rb->size) {
        rb->write_index = 0U;
    }
    if (rb->count < rb->size) {
        rb->count++;
    }
    rb->total_written++;
}

uint8_t VibRing_IsFull(const VibRingBuffer *rb)
{
    return rb->count >= rb->size;
}

void VibRing_CopyLatest(const VibRingBuffer *rb, float *out, uint16_t n)
{
    uint16_t start;
    uint16_t i;

    if (n > rb->count) {
        n = rb->count;
    }

    start = (rb->write_index + rb->size - n) % rb->size;
    for (i = 0U; i < n; i++) {
        out[i] = rb->data[(start + i) % rb->size];
    }
}

