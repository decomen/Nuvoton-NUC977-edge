#ifndef __RTU_DES_H__
#define __RTU_DES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int rtu_des_encrypt( unsigned char *out, unsigned char *in, long datalen, unsigned char key[8] );
int rtu_des_decrypt( unsigned char *out, unsigned char *in, long datalen, unsigned char key[8] );

#ifdef __cplusplus
}
#endif

#endif



