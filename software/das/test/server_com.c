

#include <board.h>


static volatile ushort com_sequence = 0;

#define SERVER_BUF_SIZE_COM (1024)

//全局buffer给整个通信用
static unsigned char com_server_buf[SERVER_BUF_SIZE_COM];

// 尝试匹配帧头(有效防止丢包导致的多帧数据丢失)
S_PACKAGE_HEAD *try2match_com_head(u8 ch)
{
	static S_PACKAGE_HEAD head;
	bQueueWrite(g_queue_com_head, ch);

	// 1.判断队列满
	if (bQueueFull(g_queue_com_head) == STATUS_OK) {
		// 2. 判断前导和版本
		if (xQueueGet(g_queue_com_head, 0) != PRECODE_0 ||
				xQueueGet(g_queue_com_head, 1) != PRECODE_1 ||
				xQueueGet(g_queue_com_head, 4) != SDCCP_VER) {
			// 腾出一个字节空间
			bQueueRemove(g_queue_com_head);
		} else {
			// 3. 校验
			int i;

			for (i = 0; i < sizeof(S_PACKAGE_HEAD); i++) {
				((u8 *)&head)[i] = xQueueGet(g_queue_com_head, i);
			}

			if (btMDCrc8(0, &head, sizeof(S_PACKAGE_HEAD) - 1) == head.CRC8) {
				//收到正确帧头
				//清空帧头队列
				vQueueClear(g_queue_com_head);
				// 返回帧头
				return &head;
			} else {
				// 移除假前导
				bQueueRemove(g_queue_com_head);
				bQueueRemove(g_queue_com_head);
			}
		}
	}

	return NULL;
}


static int send_block(void *buf, u32 len)
{
	u8 *data = (u8 *)buf;
	u32 i = 0 ;

	/*
	for (i = 0 ; i < len ; i++){
		UART_WriteByte(TEST_UART_INSTANCE, data[i]);
	}*/

	vUartSend(HW_COM, buf, len);

	return SDCCP_OK;
}

void send_buf2com(void *buf, u32 len)
{
	send_block(buf, len);
}

static void creat_package_buffer(byte_buffer_t *bb, S_PACKAGE *p_package)
{
	bb_new_from_buf(bb, com_server_buf, SERVER_BUF_SIZE_COM);
	bb_skip(bb, sizeof(S_PACKAGE_HEAD));
	bb_put(bb, p_package->Msg.Type);
	bb_put_short(bb, p_package->Msg.Senquence);
}

static void send_package_buffer(byte_buffer_t *bb, S_PACKAGE *p_package)
{
	u32 crc_32 = 0;
	CREAT_HEADER(p_package->Head, bb_get_pos(bb) + 4, p_package->Head.Type, 0);
	bb_put_bytes_at(bb, (u8 *)&p_package->Head, sizeof(S_PACKAGE_HEAD), 0);

	crc_32 = ulMDCrc32(0, bb_buffer(bb), bb_get_pos(bb));
	bb_put_int(bb, crc_32);

	send_block(bb_buffer(bb), bb_get_pos(bb));
}

// 同步获取数据, 可设置超时时间
// 注: 尝试匹配帧头, 当检测到完整帧头时建议重新开始获取一个新的帧
static int recv_block(void *buf, u32 len, u32 timeout)
{
	u32 index = 0 ;
	u32 begin = rt_tick_get();
	u8 *data = (u8 *)buf;

	for (index = 0; index < len; index++) {
		while (bQueueRead((*p_com_queue), data[index]) != RT_TRUE) {
			// 及时分析数据
			if (SDCCP_CHECK_TIME_OUT(begin, timeout)) {
				return SDCCP_TIMEOUT;
			}
		}

		// 解析到了一个正确帧头, 说明之前可能有数据丢失情况, 需要重新获取一个新的包
		//if( try2match_com_head(data[index]) != NULL) return SDCCP_RETRY;
		begin = rt_tick_get();
	}

	return SDCCP_OK;
}

void send_base_request(const S_MSG *msg, S_MSG_TEST_BASE_REQUEST *request)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_REQUEST;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, request->Type);

	send_package_buffer(&bb, &package);
}

// send_ack 可能在处理buffer前反馈, 因此使用局部buffer, 避免冲突
/*static void send_ack(const S_MSG *msg, byte ack_value)
{
	S_MSG_ACK ack;
	S_PACKAGE package;
	byte_buffer_t bb;
	unsigned char ack_buf[20] = {0};

	bb_new_from_buf(&bb, ack_buf, MYSIZEOF(ack_buf));

	package.Head.Type = FA_ACK;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;

	bb_skip(&bb, MYSIZEOF(S_PACKAGE_HEAD));
	bb_put(&bb, package.Msg.Type);
	bb_put_short(&bb, package.Msg.Senquence);

	ack.AckValue = ack_value;
	bb_put(&bb, ack.AckValue);

	send_package_buffer(&bb, &package);
}*/

void send_test_online_status(byte online_status)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	S_MSG_TEST_ONLINE_STATUS_REPORT report = {.OnlineStatus = online_status};

	package.Head.Type = FA_POST;
	package.Msg.Type = MSG_TEST_ONLINE_STATUS;
	package.Msg.Senquence = 0;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, report.OnlineStatus);

	send_package_buffer(&bb, &package);
}

void send_base_response(const S_MSG *msg, S_MSG_TEST_BASE_RESPONSE *response)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, response->Type);
	bb_put(&bb, response->RetCode);
	bb_put_int(&bb, response->Value);

	send_package_buffer(&bb, &package);
}

void send_ble_mac_response(const S_MSG *msg, S_MSG_TEST_BLE_MAC_RESPONSE *mac_response)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, mac_response->response.Type);
	bb_put(&bb, mac_response->response.RetCode);
	bb_put_int(&bb, mac_response->response.Value);
	bb_put_bytes(&bb, mac_response->MAC, 6);

	send_package_buffer(&bb, &package);
}


static void m_read_mac(char *mac)
{
	char buf[32] = {0};
    FILE *file = NULL;
    file = fopen("/tmp/s_mac","r");   
    if(file){
        fread(buf,32,1,file);
    }
    fclose(file);
    char a1[10] = {0};

    strcpy(mac,buf);

    printf("s_mac: %s  mac: %s\n",buf,mac);
	return ;  

}



static void send_device_product_info(const S_MSG * msg)
{
	S_PACKAGE package;
	S_MSG_TEST_GET_PRODUCT_INFO_RESPONSE response;
	memset(&response, 0 , sizeof(response));
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = MSG_TEST_GET_PRODUCT_INFO;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	response.RetCode = MSG_TEST_ERROR_OK;

    ReadDevInfo();

	//vDevCfgInit();  //读出来
	//response.info = g_xDevInfoReg.xDevInfo;

     /*
       printf("  -> DEV_ID: %s\n", g_sys_info.DEV_ID);
	printf("  -> SN_ID: %s\n", g_sys_info.SN);
	printf("  -> UUID: %s\n", g_sys_info.HW_ID);
	printf("  -> PROD_DATE: %s\n", g_sys_info.PROD_DATE);
	printf("  -> REG_STATUS: %d\n", g_sys_info.REG_STATUS);
	printf("  -> TEST_REMAIN: %d\n", g_sys_info.TEST_REMAIN);
	printf("  -> DEV_MODEL: 0x%x\n", g_sys_info.DEV_MODEL);		
	printf("  -> SYS_DATE : %s\n", g_sys_info.SYS_DATE);
	printf("  -> RUNTIME: %d minutes\n", g_sys_info.RUNTIME);
	printf("  -> DESC: %s\n", g_sys_info.DESC);
	printf("  -> HW_VER: %s\n", g_sys_info.HW_VER);
	printf("  -> SW_VER: %s\n", g_sys_info.SW_VER);
    */

    mdBYTE ip[4] = {192,168,8,10};
    memcpy(response.info.xSN.szSN,g_sys_info.SN, sizeof(response.info.xSN.szSN));
    memcpy(response.info.xPN.szPN,g_sys_info.DEV_ID, sizeof(response.info.xPN.szPN));
    memcpy(response.info.hwid.hwid,g_sys_info.HW_ID, sizeof(response.info.hwid.hwid));
    memcpy(response.info.xHwVer.hw_ver ,g_sys_info.HW_VER,sizeof(response.info.xHwVer));
    memcpy(response.info.xSwVer.sw_ver ,g_sys_info.SW_VER,sizeof(response.info.xSwVer));
    memcpy(response.info.xIp.szIp ,ip,sizeof(response.info.xIp.szIp));

    //vGetDevUUID(g_sys_info.HW_ID, response.info.xNetMac.mac);
    m_read_mac(response.info.xNetMac.mac);
     
	bb_put(&bb, response.RetCode);
	bb_put_bytes(&bb , response.info.xSN.szSN , sizeof(response.info.xSN));
	bb_put_bytes(&bb ,response.info.xHwVer.hw_ver, sizeof(response.info.xHwVer));
	bb_put_bytes(&bb , response.info.xSwVer.sw_ver, sizeof(response.info.xSwVer));
	bb_put_bytes(&bb ,response.info.xOEM.szOEM , sizeof(response.info.xOEM));
	bb_put_bytes(&bb ,response.info.xIp.szIp , sizeof(response.info.xIp));
	bb_put_bytes(&bb ,response.info.xNetMac.mac , sizeof(response.info.xNetMac));
	bb_put_short(&bb , response.info.usYear);
	bb_put_short(&bb , response.info.usMonth);
	bb_put_short(&bb , response.info.usDay);
	bb_put_short(&bb , response.info.usHour);
	bb_put_short(&bb , response.info.usMin);
	bb_put_short(&bb , response.info.usSec);
	bb_put_bytes(&bb , response.info.hwid.hwid, sizeof(response.info.hwid));
	bb_put_bytes(&bb , response.info.xPN.szPN, sizeof(response.info.xPN));

	send_package_buffer(&bb, &package);
}


void send_adc_get_response(const S_MSG *msg, S_MSG_TEST_GET_ADC_RESPONSE *adc_response)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, adc_response->RetCode);
	bb_put_int(&bb, adc_response->xAdcInfo.usChannel);
	bb_put_int(&bb, adc_response->xAdcInfo.usRange);
	bb_put_int(&bb, adc_response->xAdcInfo.usEngVal);
	bb_put_int(&bb, adc_response->xAdcInfo.usMeasureVal);

	send_package_buffer(&bb, &package);
}


static i32 process_ack(const S_MSG *msg, byte_buffer_t *bb)
{
	return SDCCP_OK;
}

//Lite 接收到的POST信息包含 MSG_NOTICE MSG_UPDATE
static i32 process_post(const S_MSG *msg, byte_buffer_t *bb)		//APP 主动推送过来的数据, 根据情况确定是否需要回复ACK
{
	return SDCCP_OK;
}

//Lite 接收到的请求有 MSG_HISTORY_DATA
static i32 process_request(const S_MSG *msg, byte_buffer_t *bb)	//APP 主动请求,需回复
{
	S_MSG_TEST_RTC_RESPONSE response;

	switch (msg->Type) {
	case MSG_TEST_GET_PRODUCT_INFO: {
		send_device_product_info(msg);
		break;
	}

    /*case MSG_TEST_SDCARD:{
		vTestSdCard(msg, bb);
		break;
	}*/

	case MSG_TEST_GPRS:{
		vTestGprs(msg, bb);
		break;
	}

    case MSG_TEST_LORA:{
        vTestLora(msg,bb);
        break;
    }

	case MSG_TEST_ZIGBEE:{
		vTestZigbee(msg, bb);
		break;
	}
	
	case MSG_TEST_RTC: {
		vTestRtc(msg, bb);
		break;
	}

	case MSG_TEST_SPIFLASH: {
		vTestSpiFlash(msg, bb);
		break;
	}

	case MSG_TEST_NET: {
		vTestNet(msg, bb);
		break;
	}

	case MSG_TEST_TTLInput:{
		vTestTTLInput(msg, bb);
		break;
	}

	/*case MSG_TEST_ON_OFF:{
		vTestOnOffInput(msg, bb);
		break;
	}*/

	case MSG_TEST_ADC:{
		vTestGetAdc(msg, bb);
		break;
	}

	case MSG_TEST_GET_TEST_ADC:{  //获取校验过后的ADC值
		vTestGetTestAdcValue(msg, bb);
		break;
	}

/*
	case MSG_TEST_VOL:{
		vTestVol(msg, bb);
		break;
	}*/

	case MSG_TEST_UART:{
		vTestUart(msg, bb);
		break;
	}

	case MSG_TEST_SetCheck:{
		vTestSetCheckDefault(msg, bb);
		break;
	}
	case MSG_TEST_TTL_OUTPUT_RELAY:{
		vTestRelays(msg, bb);
		break;
	}

	/*
	case MSG_TEST_TTL_OUTPUT_RELAY:{  //通过测试底板进行测试
		vTestTTLOutputRelay(msg, bb);
		break;
	}*/

	case MSG_TEST_SET_PRODUCT_INFO: {
		set_product_info(msg, bb);
		break;
	}

	case MSG_TEST_SET_ADC_INFO:{
		vTestSetAdc(msg, bb);
		break;
	}

	case MSG_TEST_GET_CAL_VAL:{
		vTestGetCalValue(msg,bb); 
		break;
	}
	default:
		response.RetCode = MSG_TEST_ERROR_ERR;
		send_base_response(msg, &response);
	
	}

	return SDCCP_OK;
}

//Lite 版无此请求
static i32 process_response(const S_MSG *msg, byte_buffer_t *bb)	//APP 对request的回复
{
	return SDCCP_OK;
}
static i32 process_package(S_PACKAGE_HEAD *h)
{
	S_MSG msg;
	i32 ret = SDCCP_ERR;
	u32 crc_32 = 0;
	byte_buffer_t bb;
	S_PACKAGE_HEAD head = *h;
	ushort size = ntohs(head.Length);

	if (size > SERVER_BUF_SIZE_COM) {
		return SDCCP_ERR;
	}

	bb_new_from_buf(&bb, com_server_buf, size);

	//bb_clear(&bb);

	bb_put_bytes(&bb, (u8 *)&head, sizeof(S_PACKAGE_HEAD));
	ret = recv_block(bb_point(&bb), bb_remain(&bb), 2000);

	if (ret != SDCCP_OK) {
		goto END_;
	}

	crc_32 = bb_get_int_at(&bb, bb_limit(&bb) - 4);

	if (ulMDCrc32(0, bb_buffer(&bb), bb_limit(&bb) - 4) != crc_32) {
		ret = SDCCP_ERR;
		goto END_;
	}

	//index = MYSIZEOF(S_PACKAGE_HEAD);
	msg.Type = bb_get(&bb);
	msg.Senquence = bb_get_short(&bb);
	msg.Content = bb_point(&bb);

	switch (head.Type) {
	case FA_ACK:
		ret = process_ack(&msg, &bb);
		break;

	case FA_POST:
		ret = process_post(&msg, &bb);
		break;

	case FA_REQUEST:
		ret = process_request(&msg, &bb);
		break;

	case FA_RESPONSE:
		ret = process_response(&msg, &bb);
		break;
	}

END_:
	return ret;
}

i32 client_event_handler_com(S_PACKAGE_HEAD *h)
{

	switch (h->Type) {
	case FA_ACK:
	case FA_POST:
	case FA_REQUEST:
	case FA_RESPONSE:
		
		return process_package(h);

	default:
		return SDCCP_ERR;
	}
}



