
#ifndef __MD_DM_MD5_H__
#define __MD_DM_MD5_H__

#include "mdtypedef.h"

typedef struct {
    mdUINT32 state[4];     
    mdUINT32 count[2];     
    mdBYTE buffer[64];     
} DM_MD5Context;

#ifdef __cplusplus
extern "C" {
#endif

void DM_MD5_Init(DM_MD5Context * context);
void DM_MD5_Update(DM_MD5Context * context, mdBYTE * buf, mdINT len);
void DM_MD5_Final(DM_MD5Context * context, mdBYTE digest[16]);
void DM_MD5_Final_16(DM_MD5Context * context, mdBYTE digest[8]);


#ifdef __cplusplus
}
#endif

#endif


