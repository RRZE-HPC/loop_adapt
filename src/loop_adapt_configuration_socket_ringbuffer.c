#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <loop_adapt_configuration_socket_ringbuffer.h>

int ringbuffer_init(int size, Ringbuffer_t *buffer)
{
    if (size <= 0 || buffer == NULL)
    {
        return -EINVAL;
    }
    Ringbuffer_t b = malloc(sizeof(Ringbuffer));
    if (!b)
    {
        return -ENOMEM;
    }
    b->buffer = malloc(sizeof(void*) * (size+1));
    if (!b->buffer)
    {
        free(b);
        return -ENOMEM;
    }
    b->size = size+1;
    b->in = 0;
    b->out = 0;
#ifdef DEBUG_RINGBUFFER
    b->debug = 1;
#endif
    *buffer = b;
    return 0;
}


int ringbuffer_put(Ringbuffer_t buffer, void* element)
{
    if (buffer->in == buffer->out-1 || (buffer->in == buffer->size-1 && buffer->out == 0))
    {
#ifdef DEBUG_RINGBUFFER
        printf("Ringbuffer full\n");
#endif
        return -1;
    }
#ifdef DEBUG_RINGBUFFER
    printf("Put element at position %d\n", buffer->in);
#endif
    buffer->buffer[buffer->in] = element;
    buffer->in++;
    if (buffer->in >= buffer->size)
    {
        buffer->in = 0;
    }
    return 0;
}

int ringbuffer_get(Ringbuffer_t buffer, void** element)
{
    if (buffer->out == buffer->in)
    {
#ifdef DEBUG_RINGBUFFER
        printf("Ringbuffer empty\n");
#endif
        return -1;
    }
#ifdef DEBUG_RINGBUFFER
    printf("Get element at position %d\n", buffer->out);
#endif
    *element = buffer->buffer[buffer->out];
    buffer->out++;
    if (buffer->out >= buffer->size)
    {
        buffer->out = 0;
    }
    return 0;
}

void ringbuffer_destroy(Ringbuffer_t buffer)
{
    if (buffer)
    {
        free(buffer->buffer);
        memset(buffer, 0, sizeof(Ringbuffer));
        free(buffer);
    }
}

#ifdef DEBUG_RINGBUFFER
void ringbuffer_print(Ringbuffer_t buffer)
{
    int fill = buffer->in - buffer->out;
    if (buffer->in == 0 && buffer->out == buffer->size-1) fill = 1;
    if (buffer) printf("In %d Out %d Fill %d\n", buffer->in, buffer->out, fill);
}
#endif
