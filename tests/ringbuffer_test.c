#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include <loop_adapt_configuration_socket_ringbuffer.h>

#define NUM_ELEMENTS 32

void producer(void* data)
{
    int i = 0;
    Ringbuffer_t buffer = (Ringbuffer_t)data;

}

void consumer(void* data)
{
    int i = 0;
    Ringbuffer_t buffer = (Ringbuffer_t)data;

}


int main()
{
    int i = 0;
    int err = 0;
    void* pre = NULL;
    Ringbuffer_t rb = NULL;
    err = ringbuffer_init(NUM_ELEMENTS, &rb);
    if (err != 0 || rb == NULL)
    {
        printf("Error %d or Ringbuffer == NULL\n", err);
        return 1;
    }

    err = ringbuffer_get(rb, &pre);
    if (err != -1)
    {
        printf("Error %d\n", err);
    }
#ifdef DEBUG_RINGBUFFER
    ringbuffer_print(rb);
#endif

    for (i = 0; i < NUM_ELEMENTS; i++)
    {
        err = ringbuffer_put(rb, (void*)&i);
        if (err != 0)
        {
            printf("Error %d\n", err);
            break;
        }
#ifdef DEBUG_RINGBUFFER
        ringbuffer_print(rb);
#endif
    }
    printf("Added %d elements\n", NUM_ELEMENTS);

    err = ringbuffer_put(rb, (void*)&i);
    if (err != -1)
    {
        printf("Error %d\n", err);
    }
#ifdef DEBUG_RINGBUFFER
    ringbuffer_print(rb);
#endif

    for (i = 0; i < NUM_ELEMENTS; i++)
    {
        void* t = NULL;
        err = ringbuffer_get(rb, (void*)&t);
        if (err != 0 || t == NULL)
        {
            printf("Error %d or data == NULL\n", err);
            break;
        }
#ifdef DEBUG_RINGBUFFER
        ringbuffer_print(rb);
#endif
    }
    printf("Got %d elements\n\n", NUM_ELEMENTS);

    for (i = 0; i < 2*NUM_ELEMENTS; i++)
    {
        void* t = NULL;
        err = ringbuffer_put(rb, (void*)&i);
        if (err != 0)
        {
            printf("Error %d\n", err);
            break;
        }
#ifdef DEBUG_RINGBUFFER
        ringbuffer_print(rb);
#endif
        err = ringbuffer_get(rb, (void*)&t);
        if (err != 0 || t == NULL)
        {
            printf("Error %d or data == NULL\n", err);
            break;
        }
#ifdef DEBUG_RINGBUFFER
        ringbuffer_print(rb);
#endif
    }


    ringbuffer_destroy(rb);

    return 0;
}
