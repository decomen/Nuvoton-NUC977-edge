#ifndef __IN_OUT_H__
#define __IN_OUT_H__

#include <rtdef.h>

typedef enum{
	PIN_RESET = 0x00,
	PIN_SET,
}eInOut_stat_t;


typedef enum{
	RELAYS_OUTPUT_1 = 0x00,
	RELAYS_OUTPUT_2,
	RELAYS_OUTPUT_3,
	RELAYS_OUTPUT_4,
	RELAYS_OUTPUT_NUM,
}eRelays_OutPut_Chanel_t;


typedef enum{
	TTL_OUTPUT_1 = 0x00,
	TTL_OUTPUT_2,
	TTL_OUTPUT_NUM,
}eTTL_Output_Chanel_t;

typedef enum{
	TTL_INPUT_1 = 0x00,
	TTL_INPUT_2,
	TTL_INPUT_3,
	TTL_INPUT_4,
	TTL_INPUT_5,
	TTL_INPUT_6,
	TTL_INPUT_7,
	TTL_INPUT_8,
	TTL_INPUT_NUM,
}eTTL_Input_Chanel_t;

#define DI_CHAN_NUM         (TTL_INPUT_NUM)
#define DO_CHAN_NUM         (RELAYS_OUTPUT_NUM+TTL_OUTPUT_NUM)

void vInOutInit(void);
void vRelaysOutputConfig(eRelays_OutPut_Chanel_t chan, eInOut_stat_t sta);
void vRelaysOutputGet(eRelays_OutPut_Chanel_t chan, eInOut_stat_t *sta);
void vRelaysOutputToggle(eRelays_OutPut_Chanel_t chan);

void vTTLOutputConfig(eTTL_Output_Chanel_t chan, eInOut_stat_t sta);
void vTTLOutputGet(eTTL_Output_Chanel_t chan, eInOut_stat_t *sta);
void vTTLOutputToggle(eTTL_Output_Chanel_t chan);

void vTTLInputputGet(eTTL_Input_Chanel_t chan, eInOut_stat_t *sta);

#endif
