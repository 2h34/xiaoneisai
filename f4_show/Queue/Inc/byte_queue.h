#ifndef __BYTE_QUEUE_H
#define __BYTE_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#define BYTE_QUEUE_SIZE 16

typedef struct{
    uint8_t buf[BYTE_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} byte_queue_t;

void byte_queue_init(byte_queue_t *q);
bool byte_queue_is_empty(const byte_queue_t *q);
bool byte_queue_is_full(const byte_queue_t *q);
bool byte_queue_push(byte_queue_t *q, uint8_t data);
bool byte_queue_pop(byte_queue_t *q, uint8_t *data);



#endif