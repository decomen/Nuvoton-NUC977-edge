/*
   ByteBuffer (C implementation)
   bytebuffer.h
*/

#ifndef _BYTEBUFFER_H_
#define _BYTEBUFFER_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define LITTLE_EDIAN



#define ___INLINE 

//#define size_t u32 

typedef struct {
    uint32_t pos;	// Read/Write position
    uint32_t len;		// Length of buf array
    uint8_t *buf;
} byte_buffer_t;

// Creat functions
byte_buffer_t *bb_new_from_buf(byte_buffer_t *bb, uint8_t *buf, size_t len);

// Utility
void bb_skip(byte_buffer_t *bb, size_t len);
void bb_rewind(byte_buffer_t *bb);
uint32_t bb_limit(byte_buffer_t *bb);
uint32_t bb_remain(byte_buffer_t *bb);
void bb_set_pos(byte_buffer_t *bb, uint32_t pos);
uint32_t bb_get_pos(byte_buffer_t *bb);

void bb_clear(byte_buffer_t *bb);

// Read functions
uint8_t bb_peek(byte_buffer_t *bb);
uint8_t *bb_buffer(byte_buffer_t *bb);
uint8_t *bb_point(byte_buffer_t *bb);
uint8_t bb_get(byte_buffer_t *bb);
uint8_t bb_get_at(byte_buffer_t *bb, uint32_t index);
void bb_get_bytes(byte_buffer_t *bb, uint8_t *dest, size_t len);
void bb_get_bytes_at(byte_buffer_t *bb, uint32_t index, uint8_t *dest, size_t len);
uint32_t bb_get_int(byte_buffer_t *bb);
uint32_t bb_get_int_at(byte_buffer_t *bb, uint32_t index);
uint16_t bb_get_short(byte_buffer_t *bb);
uint16_t bb_get_short_at(byte_buffer_t *bb, uint32_t index);

// Put functions (simply drop bytes until there is no more room)
void bb_put(byte_buffer_t *bb, uint8_t value);
void bb_put_at(byte_buffer_t *bb, uint8_t value, uint32_t index);
void bb_put_bytes(byte_buffer_t *bb, uint8_t *arr, size_t len);
void bb_put_bytes_at(byte_buffer_t *bb, uint8_t *arr, size_t len, uint32_t index);
void bb_put_int(byte_buffer_t *bb, uint32_t value);
void bb_put_int_at(byte_buffer_t *bb, uint32_t value, uint32_t index);
void bb_put_short(byte_buffer_t *bb, uint16_t value);
void bb_put_short_at(byte_buffer_t *bb, uint16_t value, uint32_t index);



#endif
