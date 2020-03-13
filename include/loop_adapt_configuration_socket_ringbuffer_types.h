#ifndef RINGBUFFER_TYPES_H
#define RINGBUFFER_TYPES_H

typedef struct {
    int in;
    int size;
    void** buffer;
#ifdef DEBUG_RINGBUFFER
    int debug;
#endif
    int out;
} Ringbuffer;

typedef Ringbuffer* Ringbuffer_t;


#endif
