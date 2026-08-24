#include "byte_queue.h"


void byte_queue_init(byte_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

bool byte_queue_is_empty(const byte_queue_t *q)
{
    return q->count == 0;
}

bool byte_queue_is_full(const byte_queue_t *q)
{
    return q->count == BYTE_QUEUE_SIZE;
}

bool byte_queue_push(byte_queue_t *q, uint8_t data)
{
    if (byte_queue_is_full(q))
    {
        return false; // 队列已满，无法入队
    }

    q->buf[q->tail] = data;
    q->tail = (q->tail + 1) % BYTE_QUEUE_SIZE;
    q->count++;
    return true; // 入队成功
}

bool byte_queue_pop(byte_queue_t *q, uint8_t *data)
{
    if (byte_queue_is_empty(q))
    {
        return false; // 队列为空，无法出队
    }

    *data = q->buf[q->head];
    q->head = (q->head + 1) % BYTE_QUEUE_SIZE;
    q->count--;
    return true; // 出队成功
}