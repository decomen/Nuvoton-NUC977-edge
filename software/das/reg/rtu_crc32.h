
#ifndef __RT_CRC32_H__
#define __RT_CRC32_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t rtu_crc32(uint32_t crc, const void *data, int len);
uint32_t rtu_ncrc32(uint32_t crc, const void *data, int len);

#ifdef __cplusplus
}
#endif

#endif


