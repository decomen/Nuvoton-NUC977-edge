
#include "board.h"

#include "mdtypedef.h"
#include "dlt645_1997.h"
#include "stdlib.h"

#define DLT645_1997_debug        rt_kprintf

static S_DLT645_1997_A_t gDlt645_1997Addr = {
	{0xaa,0xaa,0xaa,0xaa,0xaa,0xaa}
};

static rt_mq_t s_dlt645_1997_queue[BOARD_UART_MAX] = {0};
static pDlt645_1997SendDataFun prvDlt645_1997SendData = NULL;

void dlt645_1997_init(int index, pDlt645_1997SendDataFun sendfunc)
{
    if ((s_dlt645_1997_queue[index] = rt_mq_create("dlt645_1997", sizeof(S_DLT645_1997_Result_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("dlt645_1997 rt_mq_create dlt645_1997 falied..\n");
        return ;
    }
    
    prvDlt645_1997SendData = sendfunc;
}


static byte CheckSum(pbyte buf, int len)
{
	byte sum = 0;
	int i = 0;
	for(i = 0 ; i < len; i++){
		sum+=buf[i];
	}
	return sum;
}


static pbyte vCreatePaket(S_DLT645_1997_A_t xAddr, S_DLT645_1997_C_t xCon ,  mdBYTE *pData, mdBYTE xDataLen,  mdBYTE *pPacketLen)
{
	static mdBYTE buf[64] = {0};
	int index = 0;   
	mdBYTE check = 0;

	buf[index] = DLT645_1997_PRE;
	index++;

	
	memcpy(&buf[index],&xAddr,6);
	index +=6;
	

	buf[index] = DLT645_1997_MID;
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

	buf[index] = DLT645_1997_EOM;
	index++;

	*pPacketLen = index;

	return buf;	
}

/* void vDlt645_1997RequestCheckTime(S_DLT645_1997_A_t xaddr, S_DLT645_1997_Time_t *ptime)    //广播对时
{
	S_DLT645_1997_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;

	xCon.C_1997_CODE = C_1997_CODE_CHECK_TIME;
	xCon.DIR = C_1997_DIR_HOST;
	xCon.ACK = C_1997_ACK_OK;
	xCon.FCK = C_1997_FCK_0;

	ptime->MM += 0x33;
	ptime->YY += 0x33;
	ptime->DD += 0x33;
	ptime->hh += 0x33;
	ptime->mm += 0x33;
	ptime->ss += 0x33;

	pbuf = vCreatePaket(xaddr, xCon, (mdBYTE*)ptime, sizeof(S_DLT645_1997_Time_t), &xPacketLen);
	if (prvDlt645_1997SendData) prvDlt645_1997SendData(0, pbuf ,xPacketLen);

}





void vDlt645_1997ReadAddr()    //读取设备通讯地址
{
	S_DLT645_1997_A_t xaddr = {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};
	//S_DLT645_1997_A_t xaddr = gDlt645_1997AddrReal;
	S_DLT645_1997_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;

	xCon.C_CODE = C_CODE_READ_ADDR;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;
	
	pbuf = vCreatePaket(xaddr, xCon, NULL, 0, &xPacketLen);
	if (prvDlt645_1997SendData) prvDlt645_1997SendData(3, pbuf ,xPacketLen);

}


void vDlt645_1997ResponAddr()    //从机反馈设备地址(用于测试)
{
	S_DLT645_1997_A_t xaddr = {0x12,0x34,0x56,0x78,0x90,0x1A};
	S_DLT645_1997_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;

	xCon.C_CODE = C_CODE_READ_ADDR;
	xCon.DIR = C_DIR_SLAVE;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;
	
	pbuf = vCreatePaket(xaddr, xCon, xaddr.addr, 6, &xPacketLen);
	if (prvDlt645_1997SendData) prvDlt645_1997SendData(0, pbuf ,xPacketLen);

}



void vDlt645_1997SetBaud(S_DLT645_1997_A_t xaddr, S_DLT645_1997_BAUD_t baud)     //设置通讯波特率 默认2400
{
	S_DLT645_1997_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	mdBYTE baud_Z = 0;

	xCon.C_CODE = C_CODE_SET_BAUD;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;

	baud_Z = (mdBYTE)baud;
	
	pbuf = vCreatePaket(xaddr, xCon, &baud_Z, 1, &xPacketLen);
	if (prvDlt645_1997SendData) prvDlt645_1997SendData(0, pbuf ,xPacketLen);

}*/

rt_bool_t bDlt645_1997ReadData(int index, S_DLT645_1997_A_t xaddr, mdUINT16 DataMarker, int timeout, S_DLT645_1997_Result_t *result)
{   
	S_DLT645_1997_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	mdUINT32 datamarker = 0;

	xCon.C_1997_CODE = C_1997_CODE_READ_DATA;
	xCon.DIR = C_1997_DIR_HOST;
	xCon.ACK = C_1997_ACK_OK;
	xCon.FCK = C_1997_FCK_0;

	datamarker = (mdUINT16)DataMarker + 0x3333;
	
	pbuf = vCreatePaket(xaddr, xCon, (mdBYTE*)&datamarker, 2, &xPacketLen);
	if (s_dlt645_1997_queue[index]) {
	    rt_mq_recv(s_dlt645_1997_queue[index], result, sizeof(S_DLT645_1997_Result_t), RT_WAITING_NO);
	}
	if (prvDlt645_1997SendData) prvDlt645_1997SendData(index, pbuf ,xPacketLen);

	if (s_dlt645_1997_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_dlt645_1997_queue[index], result, sizeof(S_DLT645_1997_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if ((0 == memcmp(&result->addr, &xaddr, sizeof(S_DLT645_1997_A_t))) && result->op == DataMarker) {
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
	
	if(len == 4){
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



static mdBOOL vPraseData(mdBYTE *pData, mdBYTE len, mdUINT16 DataMark, float *pFdata)
{
	float floatData = 0;

	mdBOOL status = mdFALSE;
	
	
	if(len <= 0){
		DLT645_1997_debug("无数据\r\n");
		return mdFALSE;
	}

	if((DataMark >= ENERGY_1997_DATA_MARK_B0) && (DataMark <= ENERGY_1997_DATA_MARK_J4)){
		DLT645_1997_debug("收到电能量相关数据:len = %d\r\n",len);
		if(len == 4){
		    floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 100.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_1997_DATA_MARK_00) && (DataMark <= VAR_1997_DATA_MARK_02)){
		DLT645_1997_debug("收到电压数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 10.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_1997_DATA_MARK_04) && (DataMark <= VAR_1997_DATA_MARK_06)){
		DLT645_1997_debug("收到电流数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 100.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_1997_DATA_MARK_08) && (DataMark <= VAR_1997_DATA_MARK_0B)){
		DLT645_1997_debug("收到瞬时有功功率数据:len = %d\r\n",len);
		if(len == 3){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 10000.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_1997_DATA_MARK_0D) && (DataMark <= VAR_1997_DATA_MARK_10)){
		DLT645_1997_debug("收到瞬时无功功率数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 100.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= VAR_1997_DATA_MARK_17) && (DataMark <= VAR_1997_DATA_MARK_1A)){
		DLT645_1997_debug("收到功率因素数据:len = %d\r\n",len);
		if(len == 2){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 1000.0f);
			status = mdTRUE;
		}
	}else if((DataMark >= ENERGY_1997_DATA_MARK_K0) && (DataMark <= ENERGY_1997_DATA_MARK_K3)){
		DLT645_1997_debug("收到最大需量:len = %d\r\n",len);
		if(len == 3){
			floatData = DataToDouble(pData,len);
			*pFdata = (floatData / 10000.0f);
			status = mdTRUE;
		}
	}
	return status;
}

static void Dlt645_1997_ParsePack(int index, S_DLT645_1997_Package_t *pPack)
{
	mdBYTE err_code = 0;
	mdUINT16 DataMark = 0;
	mdBYTE baud = 0;
	float fdata = 0;
	int i = 0;
	
	if((pPack->xCon.DIR != C_1997_DIR_SLAVE) || (pPack->xCon.ACK != C_1997_ACK_OK)){
		if(pPack->btLen == 1){
			memcpy(&err_code,pPack->pData,1);
		}
		DLT645_1997_debug("通讯异常，控制码:%x , 错误码:%x\r\n", pPack->xCon.C_1997_CODE , err_code);
		return;
	}

	switch(pPack->xCon.C_1997_CODE){
		case C_1997_CODE_CHECK_TIME:
			break;
		case C_1997_CODE_SET_BAUD:
			if(pPack->btLen == 1){
				memcpy(&baud,pPack->pData,1);
				DLT645_1997_debug("波特率设置成功:%x\r\n",baud);
			}
			break;
		case C_1997_CODE_READ_DATA:
			if(pPack->btLen >= 2){
				memcpy(&DataMark,pPack->pData,2);
				DataMark = DataMark - 0x3333;
				if(vPraseData(pPack->pData + 2, pPack->btLen - 2, DataMark,&fdata) == mdTRUE){
					DLT645_1997_debug("收到数据项：0x%x , fdata = %f\r\n",DataMark,fdata);
	                if (s_dlt645_1997_queue[index]) {
    					S_DLT645_1997_Result_t result = { .index = index, .addr = pPack->xAddr, .op = DataMark, .val = fdata };
    					rt_mq_send(s_dlt645_1997_queue[index], &result, sizeof(S_DLT645_1997_Result_t));
					}
				}
			}
			break;
		default:
			DLT645_1997_debug("收到其他命令\r\n");
			break;
	}

	
}

//每收到一包数据，调用该函数
mdBOOL Dlt645_1997_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen)
{

	mdBYTE check = 0;
	S_DLT645_1997_Package_t pack;
	int i = 0;
	for(i = 0 ; i < usBytesLen; i++){
		if(pBytes[i] == DLT645_1997_PRE){
			break;
		}
	}

       pBytes += i;
       usBytesLen -= i;
	
	if(pBytes[0] != DLT645_1997_PRE || pBytes[7] != DLT645_1997_MID || pBytes[usBytesLen-1] != DLT645_1997_EOM){
		return mdFALSE;
	}

	check = CheckSum(pBytes,usBytesLen - 2);
	if(check != pBytes[usBytesLen-2]){
		return mdFALSE;
	}


	memcpy(&pack,pBytes, 10);
	pack.pData = &pBytes[10];

	Dlt645_1997_ParsePack(index, &pack);

}

