
#ifndef __RTU_SHA1_H__
#define __RTU_SHA1_H__

#include <stdint.h>

typedef struct {
	uint32_t  h0,h1,h2,h3,h4;
	uint32_t  nblocks;
	unsigned char buf[64];
	int  count;
} RTU_SHA1_CONTEXT;

void rtu_sha1_init( RTU_SHA1_CONTEXT *hd );
void rtu_sha1_write( RTU_SHA1_CONTEXT *hd, unsigned char *inbuf, int inlen );
void rtu_sha1_final( RTU_SHA1_CONTEXT *hd );

#endif
