#include "stdint.h"
#include "rtdef.h"
#include "rtcrc32.h"

//crc?????0
uint32_t dm_crc32(uint32_t crc, void *ptr, uint32_t len)
{
	return ulRTCrc32(crc, ptr, len);
}

