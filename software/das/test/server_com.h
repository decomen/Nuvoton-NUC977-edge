#ifndef __SERVER_COM_H__
#define __SERVER_COM_H__

extern i32 client_event_handler_com(S_PACKAGE_HEAD *h);
extern S_PACKAGE_HEAD *try2match_com_head(u8 ch);
extern void send_buf2com(void *buf, u32 len);
extern void send_base_response(const S_MSG *msg, S_MSG_TEST_BASE_RESPONSE *response);
extern void send_ble_mac_response(const S_MSG *msg, S_MSG_TEST_BLE_MAC_RESPONSE *mac_response);
extern void send_test_online_status(byte status);
void TestUartSend(uint32_t instance, uint16_t ch);
void send_adc_get_response(const S_MSG *msg, S_MSG_TEST_GET_ADC_RESPONSE *adc_response);

#endif

