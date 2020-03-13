#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <loop_adapt_configuration_socket_ringbuffer_types.h>

int ringbuffer_init(int size, Ringbuffer_t *buffer);
int ringbuffer_put(Ringbuffer_t buffer, void* element);
int ringbuffer_get(Ringbuffer_t buffer, void** element);
void ringbuffer_destroy(Ringbuffer_t buffer);

#ifdef DEBUG_RINGBUFFER
void ringbuffer_print(Ringbuffer_t buffer);
#endif

#endif
