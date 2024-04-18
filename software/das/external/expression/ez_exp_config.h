#ifndef __EZ_EXP_CONFIG_H__
#define __EZ_EXP_CONFIG_H__

#define __TO_STR(R)     #R  
#define __DEF_TO_STR(R) __TO_STR(R)

/** 寄存器最大数目 */
#define EZ_EXP_REG_COUNT        64
/** 寄存器名称长度 */
#define EZ_EXP_REG_NAME_LIMIT   16

// if used custom function EZ_EXP_USE_HASH must be 1
#define EZ_EXP_USE_HASH         1

#endif

