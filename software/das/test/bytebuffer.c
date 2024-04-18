
#include "bytebuffer.h"

// Wrap around an existing buf - will not copy buf
___INLINE byte_buffer_t *bb_new_from_buf(byte_buffer_t *bb, uint8_t *buf, size_t len) {
	bb->pos = 0;
	bb->len = len;
	bb->buf = buf;
	return bb;
}

// 跳过len字节
___INLINE void bb_skip(byte_buffer_t *bb, size_t len) {
	bb->pos += len;
}

// 复位pos
___INLINE void bb_rewind(byte_buffer_t *bb) {
	bb->pos = 0;
}

// 流大小
___INLINE uint32_t bb_limit(byte_buffer_t *bb) {
	return bb->len;
}

// 剩余大小
___INLINE uint32_t bb_remain(byte_buffer_t *bb) {
	return bb->len - bb->pos;
}

// 当前位置
___INLINE void bb_set_pos(byte_buffer_t *bb, uint32_t pos) {
	bb->pos = pos;
}

___INLINE uint32_t bb_get_pos(byte_buffer_t *bb) {
	return bb->pos;
}

// 清空buf内容, 同时复位pos
___INLINE void bb_clear(byte_buffer_t *bb) {
	memset(bb->buf, 0, bb->len);
	bb->pos = 0;
}

// 读取当前字节, pos 不后移
___INLINE uint8_t bb_peek(byte_buffer_t *bb) {
	//return *(uint8_t*)(bb->buf+bb->pos);
	return bb->buf[bb->pos];
}

___INLINE uint8_t *bb_buffer(byte_buffer_t *bb) {
	return bb->buf;
}

// 取得当前指针
___INLINE uint8_t *bb_point(byte_buffer_t *bb) {
	return bb->buf + bb->pos;
}

// 读取当前字节, pos+1
___INLINE uint8_t bb_get(byte_buffer_t *bb) {
	return bb->buf[bb->pos++];
}

// 读取指定位置字节, pos 不后移
___INLINE uint8_t bb_get_at(byte_buffer_t *bb, uint32_t index) {
	return bb->buf[index];
}

___INLINE void bb_get_bytes(byte_buffer_t *bb, uint8_t *dest, size_t len) {
	memcpy(dest, bb->buf + bb->pos, len);
	bb->pos += len;
}

___INLINE void bb_get_bytes_at(byte_buffer_t *bb, uint32_t index, uint8_t *dest, size_t len) {
	memcpy(dest, bb->buf + index, len);
}

___INLINE uint32_t bb_get_int(byte_buffer_t *bb) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 24);
	ret |= (bb->buf[bb->pos++] << 16);
	ret |= (bb->buf[bb->pos++] << 8);
	ret |= (bb->buf[bb->pos++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 0);
	ret |= (bb->buf[bb->pos++] << 8);
	ret |= (bb->buf[bb->pos++] << 16);
	ret |= (bb->buf[bb->pos++] << 24);
#endif

	return ret;
}

___INLINE uint32_t bb_get_int_at(byte_buffer_t *bb, uint32_t index) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 24);
	ret |= (bb->buf[index++] << 16);
	ret |= (bb->buf[index++] << 8);
	ret |= (bb->buf[index++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 0);
	ret |= (bb->buf[index++] << 8);
	ret |= (bb->buf[index++] << 16);
	ret |= (bb->buf[index++] << 24);
#endif

	return ret;
}

___INLINE uint16_t bb_get_short(byte_buffer_t *bb) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 8);
	ret |= (bb->buf[bb->pos++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 0);
	ret |= (bb->buf[bb->pos++] << 8);
#endif

	return ret;
}

___INLINE uint16_t bb_get_short_at(byte_buffer_t *bb, uint32_t index) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 8);
	ret |= (bb->buf[index++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 0);
	ret |= (bb->buf[index++] << 8);
#endif

	return ret;
}

// Relative write of the entire contents of another ByteBuffer (src)
___INLINE void bb_put(byte_buffer_t *bb, uint8_t value) {
	bb->buf[bb->pos++] = value;
}

___INLINE void bb_put_at(byte_buffer_t *bb, uint8_t value, uint32_t index) {
	bb->buf[index] = value;
}

___INLINE void bb_put_bytes(byte_buffer_t *bb, uint8_t *arr, size_t len) {
	memcpy (bb->buf + bb->pos, arr, len);
	bb->pos += len;
}

___INLINE void bb_put_bytes_at(byte_buffer_t *bb, uint8_t *arr, size_t len, uint32_t index) {
	memcpy (bb->buf + index, arr, len);
}

___INLINE void bb_put_int(byte_buffer_t *bb, uint32_t value) {
#ifdef LITTLE_EDIAN
	bb->buf[bb->pos++] = (value >> 24) & 0xFF;
	bb->buf[bb->pos++] = (value >> 16) & 0xFF;
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
	bb->buf[bb->pos++] = (value >> 0) & 0xFF;
#else
	bb->buf[bb->pos++] = (value >> 0 ) & 0xFF;
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
	bb->buf[bb->pos++] = (value >> 16) & 0xFF;
	bb->buf[bb->pos++] = (value >> 24) & 0xFF;
#endif
}

___INLINE void bb_put_int_at(byte_buffer_t *bb, uint32_t value, uint32_t index) {
#ifdef LITTLE_EDIAN
	bb->buf[index++] = (value >> 24) & 0xFF;
	bb->buf[index++] = (value >> 16) & 0xFF;
	bb->buf[index++] = (value >> 8) & 0xFF;
	bb->buf[index++] = (value >> 0) & 0xFF;
#else
	bb->buf[index++] = (value >> 0 ) & 0xFF;
	bb->buf[index++] = (value >> 8) & 0xFF;
	bb->buf[index++] = (value >> 16) & 0xFF;
	bb->buf[index++] = (value >> 24) & 0xFF;
#endif
}

___INLINE void bb_put_short(byte_buffer_t *bb, uint16_t value) {
#ifdef LITTLE_EDIAN
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
	bb->buf[bb->pos++] = (value >> 0) & 0xFF;
#else
	bb->buf[bb->pos++] = (value >> 0) & 0xFF;
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
#endif
}

___INLINE void bb_put_short_at(byte_buffer_t *bb, uint16_t value, uint32_t index) {
#ifdef LITTLE_EDIAN
	bb->buf[index++] = (value >> 8) & 0xFF;
	bb->buf[index++] = (value >> 0) & 0xFF;
#else
	bb->buf[index++] = (value >> 0) & 0xFF;
	bb->buf[index++] = (value >> 8) & 0xFF;
#endif
}