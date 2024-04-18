
#include "board.h"

#include "mdtypedef.h"
#include "dlt645.h"
#include "stdlib.h"

#define DLT645_debug        rt_kprintf

static S_DLT645_A_t gDlt645Addr = {
	{0xaa,0xaa,0xaa,0xaa,0xaa,0xaa}
};

static rt_mq_t s_dlt645_queue[BOARD_UART_MAX] = {0};
static pDlt645SendDataFun prvDlt645SendData = NULL;

void dlt645_init(int index, pDlt645SendDataFun sendfunc)
{
    if ((s_dlt645_queue[index] = rt_mq_create("dlt645", sizeof(S_DLT645_Result_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("dlt645 rt_mq_create dlt645 falied..\n");
        return ;
    }
    
    prvDlt645SendData = sendfunc;
}

/*static S_DLT645_A_t gDlt645AddrReal = {
	{0x96 ,0x58 ,0x72 ,0x10, 0x16 ,0x04}
};
const static S_DLT645_A_t gBroadcastAddr = {
	{0x99,0x99,0x99,0x99,0x99,0x99}
};*/

/*
const unsigned int g_ENERGY_DATA_MARKER_E[] = {
ENERGY_DATA_MARK_00,ENERGY_DATA_MARK_01,ENERGY_DATA_MARK_02,ENERGY_DATA_MARK_03,ENERGY_DATA_MARK_04,ENERGY_DATA_MARK_05,ENERGY_DATA_MARK_06,ENERGY_DATA_MARK_07,ENERGY_DATA_MARK_08,
ENERGY_DATA_MARK_20,ENERGY_DATA_MARK_21,ENERGY_DATA_MARK_22,ENERGY_DATA_MARK_23,ENERGY_DATA_MARK_24,ENERGY_DATA_MARK_25,ENERGY_DATA_MARK_26,ENERGY_DATA_MARK_27,
ENERGY_DATA_MARK_40,ENERGY_DATA_MARK_41,ENERGY_DATA_MARK_42,ENERGY_DATA_MARK_43,ENERGY_DATA_MARK_44,ENERGY_DATA_MARK_45,ENERGY_DATA_MARK_46,ENERGY_DATA_MARK_47,
ENERGY_DATA_MARK_60,ENERGY_DATA_MARK_61,ENERGY_DATA_MARK_62,ENERGY_DATA_MARK_63,ENERGY_DATA_MARK_64,ENERGY_DATA_MARK_65,ENERGY_DATA_MARK_66,ENERGY_DATA_MARK_67
};

const unsigned int g_VAR_DATA_MARKER_E[] = {
VAR_DATA_MARK_00,VAR_DATA_MARK_01,VAR_DATA_MARK_02,VAR_DATA_MARK_03,VAR_DATA_MARK_04,VAR_DATA_MARK_05,VAR_DATA_MARK_06,VAR_DATA_MARK_07,
VAR_DATA_MARK_08,VAR_DATA_MARK_09,VAR_DATA_MARK_0A,VAR_DATA_MARK_0B,VAR_DATA_MARK_0C,VAR_DATA_MARK_0D,VAR_DATA_MARK_0E,VAR_DATA_MARK_0F,
VAR_DATA_MARK_10,VAR_DATA_MARK_11,VAR_DATA_MARK_12,VAR_DATA_MARK_13,VAR_DATA_MARK_14,VAR_DATA_MARK_15,VAR_DATA_MARK_16,VAR_DATA_MARK_17,
VAR_DATA_MARK_18,VAR_DATA_MARK_19,VAR_DATA_MARK_1A,VAR_DATA_MARK_1B,VAR_DATA_MARK_1C,VAR_DATA_MARK_1D,VAR_DATA_MARK_1E,VAR_DATA_MARK_1F,

};*/

static byte CheckSum(pbyte buf, int len)
{
	byte sum = 0;
	int i = 0;
	for(i = 0 ; i < len; i++){
		sum+=buf[i];
	}
	return sum;
}
//pDlt645Debug DLT645_debug = NULL;

/*static void prvDlt645SendData(mdBYTE *pData  , mdBYTE len)
{
	
}*/

static pbyte vCreatePaket(S_DLT645_A_t xAddr, S_DLT645_C_t xCon ,  mdBYTE *pData, mdBYTE xDataLen,  mdBYTE *pPacketLen)
{
	static mdBYTE buf[64] = {0};
	int index = 0;   
	mdBYTE check = 0;

	buf[index] = DLT645_PRE;
	index++;

	
	memcpy(&buf[index],&xAddr,6);
	index +=6;
	

	buf[index] = DLT645_MID;
	index++;

	memcpy(&buf[index],&xCon,1);
	index++;

	buf[index] = xDataLen;
	index++;
	
	
	if((xDataLen > 0) && (pData!=NULL)){
		memcpy(&buf[index],pData,xDataLen);
		index += xDataLen;
	}

	check = CheckSum(buf,index);
	buf[index] = check;
	index++;

	buf[index] = DLT645_EOM;
	index++;

	*pPacketLen = index;

	return buf;	
}

void vDlt645RequestCheckTime(S_DLT645_A_t xaddr, S_DLT645_Time_t *ptime)    //广播对时
{
	S_DLT645_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;

	xCon.C_CODE = C_CODE_CHECK_TIME;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;

	ptime->MM += 0x33;
	ptime->YY += 0x33;
	ptime->DD += 0x33;
	ptime->hh += 0x33;
	ptime->mm += 0x33;
	ptime->ss += 0x33;

	pbuf = vCreatePaket(xaddr, xCon, (mdBYTE*)ptime, sizeof(S_DLT645_Time_t), &xPacketLen);
	if (prvDlt645SendData) prvDlt645SendData(0, pbuf ,xPacketLen);

}


void vDlt645ReadAddr()    //读取设备通讯地址
{
	S_DLT645_A_t xaddr = {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};
	//S_DLT645_A_t xaddr = gDlt645AddrReal;
	S_DLT645_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;

	xCon.C_CODE = C_CODE_READ_ADDR;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;
	
	pbuf = vCreatePaket(xaddr, xCon, NULL, 0, &xPacketLen);
	if (prvDlt645SendData) prvDlt645SendData(3, pbuf ,xPacketLen);

}


void vDlt645ResponAddr()    //从机反馈设备地址(用于测试)
{
	S_DLT645_A_t xaddr = {0x12,0x34,0x56,0x78,0x90,0x1A};
	S_DLT645_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;

	xCon.C_CODE = C_CODE_READ_ADDR;
	xCon.DIR = C_DIR_SLAVE;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;
	
	pbuf = vCreatePaket(xaddr, xCon, xaddr.addr, 6, &xPacketLen);
	if (prvDlt645SendData) prvDlt645SendData(0, pbuf ,xPacketLen);

}


void vDlt645SetBaud(S_DLT645_A_t xaddr, S_DLT645_BAUD_t baud)     //设置通讯波特率 默认2400
{
	S_DLT645_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	mdBYTE baud_Z = 0;

	xCon.C_CODE = C_CODE_SET_BAUD;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;

	baud_Z = (mdBYTE)baud;
	
	pbuf = vCreatePaket(xaddr, xCon, &baud_Z, 1, &xPacketLen);
	if (prvDlt645SendData) prvDlt645SendData(0, pbuf ,xPacketLen);

}

rt_bool_t bDlt645ReadData(int index, S_DLT645_A_t xaddr, mdUINT32 DataMarker, int timeout, S_DLT645_Result_t *result)
{   
	S_DLT645_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	mdUINT32 datamarker = 0;

	xCon.C_CODE = C_CODE_READ_DATA;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;

	datamarker = (mdUINT32)DataMarker + 0x33333333;
	
	pbuf = vCreatePaket(xaddr, xCon, (mdBYTE*)&datamarker, 4, &xPacketLen);
	if (s_dlt645_queue[index]) {
	    rt_mq_recv(s_dlt645_queue[index], result, sizeof(S_DLT645_Result_t), RT_WAITING_NO);
	}
	if (prvDlt645SendData) prvDlt645SendData(index, pbuf ,xPacketLen);

	if (s_dlt645_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_dlt645_queue[index], result, sizeof(S_DLT645_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if ((0 == memcmp(&result->addr, &xaddr, sizeof(S_DLT645_A_t))) && result->op == DataMarker) {
                return RT_TRUE;
            }
    	}
	}

	return RT_FALSE;
}


static double DataToDouble(mdBYTE *pData, mdBYTE len) //数据转换,不考虑符号
{
	char datas[20] = {0};
	int i = 0;
	for(i = 0 ; i < len; i++){
		pData[i] = pData[i] - 0x33;
	}

	if(len == 8){
		sprintf(datas,"%02x%02x%02x",pData[7],pData[6],pData[5]);
	}else if(len == 4){
		sprintf(datas,"%02x%02x%02x%02x",pData[3],pData[2],pData[1],pData[0]);
	}else if(len == 3){
		sprintf(datas,"%02x%02x%02x",pData[2],pData[1],pData[0]);
	}else if(len == 2){
		sprintf(datas,"%02x%02x",pData[1],pData[0]);
	}else if(len == 1){
		sprintf(datas,"%02x",pData[0]);
	}
	return atof(datas);
}

// 组合有功、无功电能最高位是符号位，0正1负。取值范围：0.00～799999.99。

static double __DataToDouble(mdBYTE *pData, mdBYTE len) //数据转换,考虑符号符号位
{
	char datas[20] = {0};
	int i = 0;
	for(i = 0 ; i < len; i++){
		pData[i] = pData[i] - 0x33;
	}

	if(len == 4){
		if(pData[3] && 0x80){
			sprintf(datas,"-%02x%02x%02x%02x",pData[3]&0x7f,pData[2],pData[1],pData[0]);
		}else {
			sprintf(datas,"%02x%02x%02x%02x",pData[3]&0x7f,pData[2],pData[1],pData[0]);
		}	
	}else if(len == 3){
		if(pData[2] && 0x80){
			sprintf(datas,"-%02x%02x%02x",pData[2]&0x7f,pData[1],pData[0]);
		}else {
			sprintf(datas,"%02x%02x%02x",pData[2]&0x7f,pData[1],pData[0]);
		}	
		
	}else if(len == 2){
		if(pData[1] && 0x80){
			sprintf(datas,"-%02x%02x",pData[1]&0x7f,pData[0]);
		}else {
			sprintf(datas,"%02x%02x",pData[1]&0x7f,pData[0]);
		}	
	}else if(len == 1){
		if(pData[0] && 0x80){
			sprintf(datas,"-%02x",pData[0]&0x7f);
		}else {
			sprintf(datas,"%02x",pData[0]&0x7f);
		}
	}
	return atof(datas);
}


// 组合有功、无功电能最高位是符号位，0正1负。取值范围：0.00～799999.99。

/*

注1: 三相三线电表电压A相为Uab，B相为0，C相为Ucb；电流A相为Ia，B相为0，C相为Ic；功率因数A相为Uab与Ia的夹角余弦，B相为0，C相为Ucb与Ic的夹角余弦；相角A相为Uab与Ia的夹角，B相为0，C相为Ucb与Ic的夹角。
注3: 表内温度最高位0表示零上，1表示零下。取值范围：0.0～799.9。
注4: 相角测量范围是0～360度。
注5: 当前有功需量、当前无功需量、当前视在需量是最近一段时间的平均功率。

注2: 瞬时功率及当前需量最高位表示方向，0正，1负，三相三线B相为0。取值范围：0.0000～79.9999。
6: 电流最高位表示方向，0正，1负，取值范围：0.000～799.999。功率因数最高位表示方向，0正，1负，取值范围：0.000～1.000。

*/

static mdBOOL vPraseData(mdBYTE *pData, mdBYTE len, mdUINT32 DataMark, float *pFdata)
{
	float floatData = 0;

	mdBOOL status = mdFALSE;
	
	
	if(len <= 0){
		DLT645_debug("无数据\r\n");
		return mdFALSE;
	}

	if((DataMark >= ENERGY_DATA_MARK_A0) && (DataMark <= ENERGY_DATA_MARK_67)){
		DLT645_debug("收到电能量相关数据:len = %d\r\n",len);
		if(len == 4){
			if(((DataMark >= ENERGY_DATA_MARK_A0) && (DataMark <= ENERGY_DATA_MARK_A4)) || ((DataMark >= ENERGY_DATA_MARK_D0) && (DataMark <= ENERGY_DATA_MARK_D4)) || 
				((DataMark <= ENERGY_DATA_MARK_E0) && (DataMark >= ENERGY_DATA_MARK_E4))){
				floatData = __DataToDouble(pData,len); //组合有功、无功电能最高位是符号位，0正1负。取值范围：0.00～799999.99。 考虑符号位
			}else {
				floatData = DataToDouble(pData,len);
			}
			*pFdata = (floatData / 100.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_DATA_MARK_00) && (DataMark <= VAR_DATA_MARK_03)){
		DLT645_debug("收到电压数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 10.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_DATA_MARK_04) && (DataMark <= VAR_DATA_MARK_07)){
		DLT645_debug("收到电流数据:len = %d\r\n",len);
		if(len == 3){
			floatData = __DataToDouble(pData,len);
			*pFdata = (floatData / 1000.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_DATA_MARK_08) && (DataMark <= VAR_DATA_MARK_16)){
		DLT645_debug("收到瞬时功率数据:len = %d\r\n",len);
		if(len == 3){
			floatData = __DataToDouble(pData,len);
			*pFdata = (floatData / 10000.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_DATA_MARK_17) && (DataMark <= VAR_DATA_MARK_1B)){
		DLT645_debug("收到功率因素数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 1000.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_DATA_MARK_1C) && (DataMark <= VAR_DATA_MARK_1F)){
		DLT645_debug("收到相角数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 10.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_DATA_MARK_20) && (DataMark <= VAR_DATA_MARK_23)){
		DLT645_debug("收到需量数据:len = %d\r\n",len);
		if(len == 8){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 10000.0f);
			status = mdTRUE;
		}
	}
	return status;
}

static void Dlt645_ParsePack(int index, S_DLT645_Package_t *pPack)
{
	mdBYTE err_code = 0;
	mdUINT32 DataMark = 0;
	mdBYTE baud = 0;
	float fdata = 0;
	int i = 0;
	
	if((pPack->xCon.DIR != C_DIR_SLAVE) || (pPack->xCon.ACK != C_ACK_OK)){
		if(pPack->btLen == 1){
			memcpy(&err_code,pPack->pData,1);
		}
		DLT645_debug("通讯异常，控制码:%x , 错误码:%x\r\n", pPack->xCon.C_CODE , err_code);
		return;
	}

	switch(pPack->xCon.C_CODE){
		case C_CODE_CHECK_TIME:
			break;
		case C_CODE_READ_ADDR:
			if(pPack->btLen == 6){
				memcpy(gDlt645Addr.addr,pPack->pData,6);
				DLT645_debug("读地址成功:");
				for( i = 0 ; i < 6 ;i++){
					DLT645_debug("%x",gDlt645Addr.addr[i]);
				}
				DLT645_debug("\r\n");
			}
			break;
		case C_CODE_SET_BAUD:
			if(pPack->btLen == 1){
				memcpy(&baud,pPack->pData,1);
				DLT645_debug("波特率设置成功:%x\r\n",baud);
			}
			break;
		case C_CODE_READ_DATA:
			if(pPack->btLen >= 4){
				memcpy(&DataMark,pPack->pData,4);
				DataMark = DataMark - 0x33333333;
				if(vPraseData(pPack->pData + 4, pPack->btLen - 4, DataMark,&fdata) == mdTRUE){
					DLT645_debug("收到数据项：0x%x , fdata = %f\r\n",DataMark,fdata);
	                if (s_dlt645_queue[index]) {
    					S_DLT645_Result_t result = { .index = index, .addr = pPack->xAddr, .op = DataMark, .val = fdata };
    					rt_mq_send(s_dlt645_queue[index], &result, sizeof(S_DLT645_Result_t));
					}
				}
			}
			break;
		default:
			DLT645_debug("收到其他命令\r\n");
			break;
	}

	
}

//每收到一包数据，调用该函数
mdBOOL Dlt645_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen)
{

	mdBYTE check = 0;
	S_DLT645_Package_t pack;
	int i = 0;
	for(i = 0 ; i < usBytesLen; i++){
		if(pBytes[i] == DLT645_PRE){
			break;
		}
	}

   pBytes += i;
   usBytesLen -= i;
	
	if(pBytes[0] != DLT645_PRE || pBytes[7] != DLT645_MID || pBytes[usBytesLen-1] != DLT645_EOM){
		return mdFALSE;
	}

	check = CheckSum(pBytes,usBytesLen - 2);
	if(check != pBytes[usBytesLen-2]){
		return mdFALSE;
	}


	memcpy(&pack,pBytes, 10);
	pack.pData = &pBytes[10];

	Dlt645_ParsePack(index, &pack);
}

