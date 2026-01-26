#include "ringbuffer.h"


struct ringbuffer
{
    uint32_t tail;
    uint32_t head;
    uint32_t length;
    uint8_t data[];
};

ringbuffer_t rb8_new(uint8_t *data,uint32_t length)
{
    ringbuffer_t rb = (ringbuffer_t)data;
    rb->length = length - sizeof(struct ringbuffer);
    return rb;
}

uint32_t rb8_next_head(ringbuffer_t rb)
{
    return rb->head + 1 < rb->length ? rb->head + 1 : 0;
}

uint32_t rb8_next_tail(ringbuffer_t rb)
{
    return rb->tail + 1 < rb->length ? rb->tail + 1 : 0;
}

bool rb8_is_empty(ringbuffer_t rb)
{
    return rb->head == rb->tail;
}

bool rb8_is_full(ringbuffer_t rb)
{
    return rb8_next_head(rb) == rb->tail;
}

bool rb8_put(ringbuffer_t rb,uint8_t data)
{
    if(!rb8_is_full(rb))
    {
        rb->data[rb->head] = data;
        rb->head = rb8_next_head(rb);
        return true;
    }
    return false;
}

bool rb8_get(ringbuffer_t rb,uint8_t* data)
{
    if(!rb8_is_empty(rb))
    {
        *data = rb->data[rb->tail];
        rb->tail = rb8_next_tail(rb);
        return true;
    }
    return false;
}

bool rb8_puts(ringbuffer_t rb,uint8_t* data,uint32_t length)
{
    for(uint32_t i=0;i<length;i++)
    {
        if(!rb8_put(rb,data[i]))
            return false;
    }
    return true;
}

bool rb8_gets(ringbuffer_t rb,uint8_t* data,uint32_t length)
{
    for(uint32_t i=0;i<length;i++)
    {
        if(!rb8_get(rb,&data[i]))
            return false;
    }
    return true;
}
