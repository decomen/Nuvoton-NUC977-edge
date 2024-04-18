
#include "board.h"
//#include "mdqueue.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "hjt212.h"
#include "sdccp_net.h"
#include "net_helper.h"


static void Hex2Str(rt_uint8_t hex, char *buf);
static rt_uint16_t hjt212_GetCrc16(char *data_buf , rt_uint32_t len);

#include "hjt212_ini.cc"
const char *_hjt212_default_ini = def_hjt212_default_ini;

void hjt212_try_create_default_config_file(const char *path)
{
    if (das_string_startwith(path, HJT212_INI_CFG_PATH_PREFIX, 1)) {
        if (das_get_file_length(path) < 20) {
            das_write_text_file(path, _hjt212_default_ini, strlen(_hjt212_default_ini));
        }
    }
}

rt_bool_t hjt212_req_respons(rt_uint8_t index , eReRtn_t rtn); 	//请求应答 现场机-->上位机
rt_bool_t hjt212_req_result(rt_uint8_t index, eExeRtn_t rtn);       //返回操作执行结果 现场机-->上位机

rt_bool_t hjt212_login_verify_req(rt_uint8_t index); 	   //登陆注册 现场机-->上位机
rt_bool_t hjt212_report_real_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time);      //上传实时数据
rt_bool_t hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time); //上传分钟数据
rt_bool_t hjt212_report_hour_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time);
rt_bool_t hjt212_report_system_time(rt_uint8_t index);  //上传系统时间
rt_err_t _HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen); //解析函数

static eHJT212_RcvState_t   s_eRcvState[BOARD_TCPIP_MAX];
static HJT212_Cfg_t         *s_Hj212CfgData[BOARD_TCPIP_MAX];
static HJT212_Update_t      s_Hj212UpdateData[BOARD_TCPIP_MAX];
static rt_thread_t          s_hjt212_work_thread[BOARD_TCPIP_MAX];
//static eHJT212_VerifyState_t s_eVerifyState[BOARD_TCPIP_MAX];
rt_uint32_t ulHjt212HeartbeatTick[BOARD_TCPIP_MAX] = {0};


#if 1

unsigned short hjt212_GetCrc16(char *ptr,rt_uint32_t count)
{
	//新建一个16位变量，所有数位初始均为1。
	unsigned short lwrdCrc = 0xFFFF;
	unsigned short lwrdMoveOut;
	int i = 0;
	int lintDataLen;
	int j;
	rt_uint8_t x;
	lintDataLen = count;
	while (i < lintDataLen)
	{
	
		x = (rt_uint8_t)(lwrdCrc >> 8);
		
		lwrdCrc = (unsigned short)(((unsigned short)ptr[i]) ^ x);
		i++;
		j = 0;
		while (j < 8)
		{
			lwrdMoveOut = (unsigned short)(lwrdCrc & 0x0001);
			
			lwrdCrc = (unsigned short)(lwrdCrc>> 1);
			if (lwrdMoveOut == 1)
			{
			
				lwrdCrc = (unsigned short)(lwrdCrc^ 0xA001);
			}
			j++;
		}
		
	}
	//rt_kprintf("crc = %x\n",lwrdCrc);
	return lwrdCrc;
}

#else 

/*--------------------CRC16校验算法：国标--------------------*/
static rt_uint16_t hjt212_GetCrc16(rt_uint8_t *data_buf , rt_uint32_t len)
{
	rt_uint16_t i,crc=0xFFFF;
	rt_uint8_t j;
	for(i=0;i<len;i++){
		crc^=(rt_uint16_t)(data_buf[i]);
		//crc^=(INT16U)(data_buf[i])<<8;
		for(j=0;j<8;j++){
		  if((crc&0x0001)!=0){
		    crc>>=1;
		    crc^=0xA001;
		  }else{
		  	crc>>=1;
		  }
		}
	}
	return(crc);
}

#endif

static void __HJT212_ParsePack(rt_uint8_t index, HJT212_Package_t *pPack);

rt_err_t HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen) //解析任务
{
    ulHjt212HeartbeatTick[index] = rt_tick_get();
    
    {
        char *pBuffer = (char *)s_pCCBuffer[index][0];
        rt_base_t nPos = s_CCBufferPos[index][0];
        eHJT212_RcvState_t eRcvState = s_eRcvState[index];

        for (int n = 0; n < usBytesLen; n++) {
            pBuffer[nPos++] = pBytes[n];
            if (nPos >= HJT212_BUF_SIZE) nPos = 0;
            if (HJT212_R_S_HEAD == eRcvState) {
                if (2 == nPos) {
                    rt_uint16_t pre = 0; memcpy(&pre, pBuffer, 2);
                    if (HJT212_PRE == pre) {
                        eRcvState = HJT212_R_S_EOM;
                    } else {
                        nPos = 1;
                    }
                }
            } else if (HJT212_R_S_EOM == eRcvState) {
                if (nPos >= 4) {
						//rt_kprintf("HJT212_EOM : %x , eom:%x,nPos = %d\r\n",HJT212_EOM,*(rt_uint16_t *)&pBuffer[nPos - 2],nPos);
                    rt_uint16_t eom = 0; memcpy(&eom, &pBuffer[nPos - 2], 2);
                    if ((HJT212_EOM == eom) && nPos >= 18) {

						//rt_kprintf("HJT212_EOM \r\n");
                        HJT212_Package_t xPack;
						memset(&xPack,0,sizeof(HJT212_Package_t));
						
                        memcpy((char*)&xPack.usPre, &pBuffer[0], 6);
                        xPack.xMsg.pData = &pBuffer[6];
                        memcpy(xPack.btCheck, &pBuffer[nPos - 6], 6);
						
						rt_uint32_t ulLen = ((xPack.btLen[0] - '0') * 1000) + ((xPack.btLen[1] - '0') * 100) +
						((xPack.btLen[2] - '0') * 10) + (xPack.btLen[3] - '0');

						//rt_kprintf("ulen = %d\r\n",ulLen);

						rt_uint16_t xCheck = hjt212_GetCrc16((char *)&pBuffer[6], ulLen);
						char  usCheck[4] = {0};
						Hex2Str((rt_uint8_t)(xCheck >> 8), (char *)&usCheck[0]);
						Hex2Str((rt_uint8_t)(xCheck&0xff), (char *)&usCheck[2]);
						
                        //rt_uint32_t usCheck = hjt212_htonl(xPack.usCheck);
                        if ( ( (ulLen + 12) == nPos) &&  (strncmp((char*)xPack.btCheck , (char*)usCheck,4) == 0) ) {
							
                            __HJT212_ParsePack(index, &xPack);
                        }
                        eRcvState = HJT212_R_S_HEAD;
                        nPos = 0;
                    }
                }
            }
        }

        s_CCBufferPos[index][0] = nPos;
        s_eRcvState[index] = eRcvState;
    }

    return RT_EOK;
}



/*a = atoi(str);d =atof(strd);
    rt_kprintf("%d/n",a);
    rt_kprintf("%g/n",d);
*/

/*

6、给定一个字符串iios/12DDWDFF@122，获取 / 和 @ 之间的字符串，
先将 "iios/"过滤掉，再将非'@'的一串内容送到buf中

sscanf("iios/12DDWDFF@122","%*[^/]/%[^@]",buf);
rt_kprintf("%s\n",buf);
结果为：12DDWDFF


*/

static void __HJT212_ParsePack(rt_uint8_t index, HJT212_Package_t *pPack)
{

	rt_uint32_t ulLen = ((pPack->btLen[0] - '0') * 1000) + ((pPack->btLen[1] - '0') * 100) +
						((pPack->btLen[2] - '0') * 10) + (pPack->btLen[3] - '0');
	
	char  *pBuf = pPack->xMsg.pData;

	HJT212_Data_t xData;
	memset(&xData,0,sizeof(HJT212_Data_t));
	
	int ret = 0;
	char *p = NULL;

	p = strstr(pBuf,"QN=");
	if(p){
		ret = sscanf(p+strlen("QN="),"%" STR2(QN_LEN) "[^;]",xData.QN);
		//rt_kprintf("QN=%s\r\n",xData.QN);
	}
	
	
	p = strstr(pBuf,"CN=");
	if(p){
		ret = sscanf(p+strlen("CN="),"%" STR2(CN_LEN) "[^;]",xData.CN);
		rt_kprintf("CN=%s\r\n",xData.CN);
	}

	p = strstr(pBuf,"ST=");
	if(p){
		ret = sscanf(p+strlen("ST="),"%" STR2(ST_LEN) "[^;]",xData.ST);
		//rt_kprintf("ST=%s\r\n",xData.ST);
	}

	p = strstr(pBuf,"PW=");
	if(p){
		ret = sscanf(p+strlen("PW="),"%" STR2(PW_LEN) "[^;]",xData.PW);
		//rt_kprintf("PW=%s\r\n",xData.PW);
	}

	p = strstr(pBuf,"MN=");
	if(p){
		ret = sscanf(p+strlen("MN="),"%" STR2(MN_LEN) "[^;]",xData.MN);
		//rt_kprintf("MN=%s\r\n",xData.MN);
	}

	p = strstr(pBuf,"Flag=");
	if(p){
		ret = sscanf(p+strlen("Flag="),"%" STR2(FLAG_LEN) "[^;]",xData.Flag);
		//rt_kprintf("Flag=%s\r\n",xData.Flag);
	}

	p = strstr(pBuf,"CP=&&");
	if (p) {
		//rt_kprintf("CP= %s\r\nlen = %d\r\n",p,strlen(p));
		if (strlen(p) - 13 > 0) {
			xData.cp = rt_calloc(1, strlen(p));
			ret = sscanf(p+strlen("CP=&&"),"%1024[^&]",xData.cp);
			rt_kprintf("cp=%s\r\n",xData.cp);
		}
	}

	if(strncmp(s_Hj212CfgData[index]->PW, xData.PW, sizeof(xData.PW)) != 0){
		hjt212_req_respons(index, RE_RETURN_PASSWD_ERR);
		if (xData.cp != NULL) {
    		rt_free(xData.cp);
    		xData.cp = NULL;
    	}
		return;
	}

	rt_uint16_t ulCmd = atoi(xData.CN);
	switch(ulCmd){
		case CMD_REQUEST_SET_TIMEOUT_RETRY:{
			rt_kprintf("-->CMD_REQUEST_SET_TIMEOUT_RETRY\r\n");
			break;
		}
		case CMD_REQUEST_SET_TIMEOUT_ALARM:{
			rt_kprintf("-->CMD_REQUEST_SET_TIMEOUT_ALARM\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_TIME:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_TIME\r\n");
			/*一条完整请求通讯过程需要有三个步骤
				1. 上位机请求时间
				2. 现场机请求应答
				3.上传现场机时间
				4.返回操作执行结果
			*/
			hjt212_req_respons(index, RE_RETURN_SUCCESS);
			hjt212_report_system_time(index);
			hjt212_req_result(index, EXE_RETURN_SUCCESS);
			break;
		}
		case CMD_REQUEST_SET_TIME:{
			rt_kprintf("-->CMD_REQUEST_SET_TIME\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_THRESHOLD:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_THRESHOLD\r\n");
			break;
		}
		case CMD_REQUEST_SET_CONTAM_THRESHOLD:{
			rt_kprintf("-->CMD_REQUEST_SET_CONTAM_THRESHOLD\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_UPPER_ADDR:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_UPPER_ADDR\r\n");
			break;
		}
		case CMD_REQUEST_SET_UPPER_ADDR:{
			rt_kprintf("-->CMD_REQUEST_SET_TIMEOUT_ALARM\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_DAY_DATA_TIME:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_DAY_DATA_TIME\r\n");
			break;
		}
		case CMD_REQUEST_SET_DAY_DATA_TIME:{
			rt_kprintf("-->CMD_REQUEST_SET_DAY_DATA_TIME\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_REAL_DATA_INTERVAL:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_REAL_DATA_INTERVAL\r\n");
			break;
		}
		case CMD_REQUEST_SET_REAL_DATA_INTERVAL:{
			rt_kprintf("-->CMD_REQUEST_SET_REAL_DATA_INTERVAL\r\n");
			break;
		}
		
		case CMD_REQUEST_SET_PASSWD:{
			rt_kprintf("-->CMD_REQUEST_SET_PASSWD\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA\r\n");
			break;
		}
		case CMD_NOTICE_STOP_CONTAM_REAL_DATA:{
			rt_kprintf("-->CMD_NOTICE_STOP_CONTAM_REAL_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_RUNING_STATUS:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_RUNING_STATUS\r\n");
			break;
		}
		case CMD_NOTICE_STOP_RUNING_STATUS:{
			rt_kprintf("-->CMD_NOTICE_STOP_RUNING_STATUS\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_HISTORY_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_HISTORY_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_RUNING_TIME:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_RUNING_TIME\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_ALARM_RECORD:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_ALARM_RECORD\r\n");
			break;
		}

/*反控命令*/		
		case CMD_REQUEST_CHECK:{
			rt_kprintf("-->CMD_REQUEST_CHECK\r\n");
			break;
		}
		case CMD_REQUEST_SAMPLE:{
			rt_kprintf("-->CMD_REQUEST_SAMPLE\r\n");
			break;
		}
		case CMD_REQUEST_CONTROL:{
			rt_kprintf("-->CMD_REQUEST_CONTROL\r\n");
			break;
		}
		case CMD_REQUEST_SET_SAMPLE_TIME:{
			rt_kprintf("-->CMD_REQUEST_SET_SAMPLE_TIME\r\n");
			break;
		}
		case CMD_LOGIN_RESPONSE:{
			rt_kprintf("-->CMD_LOGIN_RESPONSE\r\n");
			break;
		}
        case CMD_DATA_RESPONSE:{
            rt_kprintf("-->CMD_DATA_RESPONSE\r\n");
            break;
        }
		default:
			break;
		
	}

	if (xData.cp != NULL) {
		rt_free(xData.cp);
		xData.cp = NULL;
	}
    
	rt_mq_send(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t));
}

static void Hex2Str(rt_uint8_t hex, char *buf)
{
	//rt_uint8_t sbuf[2] = {0};
	sprintf(buf,"%02x",hex);
	for(int i = 0 ; i < 2;i++){
		if( buf[i] >='a' && buf[i] <='z'){
			buf[i]= buf[i]-32; 
		}  
	}
	//memcpy(buf,sbuf,2);
}

//不考虑拆分包
static rt_uint32_t Hjt212Encode(char *QN,char *ST,char *CN,char *PW,char *MN,char *Flag,char *CP, char *sBuf)
{
	int index = 6 , i = 0;
	if(QN){
		sBuf[index++] = 'Q';
		sBuf[index++] = 'N';
		sBuf[index++] = '=';
		for(i = 0 ; i < QN_LEN; i++){
			if('\0' ==  QN[i]) break;
			sBuf[index++] = QN[i];
		}
		sBuf[index++] = ';';
	}

	if (ST){
	    sBuf[index++]='S';
	    sBuf[index++]='T';
	    sBuf[index++]='=';
	    for (i=0; i < ST_LEN; i++)
	    {
	      if ('\0' == ST[i]) break;
	      sBuf[index++]=ST[i];
	    }
	    sBuf[index++]=';';
	  }

	if (CN){
	    sBuf[index++]='C';
	    sBuf[index++]='N';
	    sBuf[index++]='=';
	    for (i=0; i < CN_LEN; i++)
	    {
	      if ('\0' == CN[i]) break;
	      sBuf[index++]=CN[i];
	    }
	    sBuf[index++]=';';
	  }

	 if(PW){
	    sBuf[index++]='P';
	    sBuf[index++]='W';
	    sBuf[index++]='=';
	    for (i=0; i < PW_LEN; i++)
	    {
	      if ('\0' == PW[i]) break;
	      sBuf[index++] = PW[i];
	    }
	    sBuf[index++]=';';
	  }

	 if(MN){
	    sBuf[index++]='M';
	    sBuf[index++]='N';
	    sBuf[index++]='=';
	    for (i=0; i < MN_LEN; i++)
	    {
	      if ('\0' == MN[i]) break;
	      sBuf[index++] = MN[i];
	    }
	    sBuf[index++]=';';
	  }

	 if(Flag){
	    sBuf[index++]='F';
	    sBuf[index++]='l';
	    sBuf[index++]='a';
	    sBuf[index++]='g';
	    sBuf[index++]='=';
		sBuf[index++]=Flag[0];
	    sBuf[index++]=';';
	  }

	 if (CP){
	    sBuf[index++]='C';
	    sBuf[index++]='P';
	    sBuf[index++]='=';
	    sBuf[index++]='&';
	    sBuf[index++]='&';
	    for (i=0;i<strlen(CP);i++)
	    {
	      if (CP[i]==0) break;
	      sBuf[index++]=CP[i];
	    }
	    sBuf[index++]='&';
	    sBuf[index++]='&';
	  }

	  sBuf[0]='#';                /*HEAD*/
	  sBuf[1]='#';

	  int length = index - 6;      /*DATA_LENTH*/
	  for (i=0; i<4; i++){
	      sBuf[5-i]='0'+length%10;
	      length/=10;
	  }

	  /*crc16校验*/
	  rt_uint16_t usCrc16 = hjt212_GetCrc16(&sBuf[6], index - 6);
	  rt_uint8_t  high = (rt_uint8_t)(usCrc16 >> 8);
	  rt_uint8_t  low = (rt_uint8_t)(usCrc16 & 0xff);
	  Hex2Str(high,&sBuf[index]);
	  Hex2Str(low,&sBuf[index+2]);

	  /*结束符*/
	  sBuf[index+4] = 0x0d;
	  sBuf[index+5] = 0x0a;
	  index += 6;

	  return index;
	  	
}


/*

要求现场设备每隔 15 分钟发送一次登录指令，中心机收到登录注册指令后作出回
应。约定如果两次（30 分钟）均没有收到回应，则重新复位通讯模块。

*/

rt_bool_t hjt212_login_verify_req(rt_uint8_t index) 	//登陆注册 现场机-->上位机
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (512)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        das_localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year+1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN + 1] = {0};
		sprintf(st,"%s", s_Hj212CfgData[index]->ST);
		
		char CN[CN_LEN + 1] = {0};
		sprintf(CN,"%d" , CMD_LOGIN);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(qn, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "5", "", buf);
		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		
		rt_free(buf);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_req_respons(rt_uint8_t index , eReRtn_t rtn) 	//请求应答 现场机-->上位机
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (512)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        das_localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year+1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN + 1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);
		
		char CN[CN_LEN + 1] = {0};
		sprintf(CN, "%d", CMD_REQUEST_RESPONSE);

		char cp[64] = {0};
		sprintf(cp,"QN=%s;QnRtn=%d",qn,rtn);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "0", cp, buf);
		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		 
		rt_free(buf);
	}
    return RT_TRUE;
}

 rt_bool_t hjt212_req_result(rt_uint8_t index, eExeRtn_t rtn)  //返回操作执行结果 现场机-->上位机
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (512)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        das_localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);


		char st[ST_LEN + 1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[CN_LEN + 1] = {0};
		sprintf(CN,"%d" , CMD_REQUES_RESULT);

		char cp[64] = {0};
		sprintf(cp,"QN=%s;ExeRtn=%d",qn,rtn);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "0", cp, buf);
		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		 
		rt_free(buf);
	}
    return RT_TRUE;
}



static rt_bool_t hjt212_real_data_format_cp(
    rt_bool_t first,
    char *szFid, 
    float value, 
    eRealDataFlag_t dataflag, 
	char *DataTime,
	int pi,
	char *sCp
)
{
	char tmp[50] = {0};

	if(DataTime && DataTime[0] && szFid && first){
		sprintf(tmp,"DataTime=%s;",DataTime);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));
	
	if(dataflag != FLAG_DISABLE){
		if(szFid){
            sprintf(tmp,"%s-Rtd=%.*f,",szFid,pi,value);
			strcat(sCp,tmp);
		}
		
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%s-Flag=%c",szFid,dataflag);
		strcat(sCp,tmp);
	}else {
		if(szFid){
            sprintf(tmp,"%s-Rtd=%.*f",szFid,pi,value);
			strcat(sCp,tmp);
		}
	}
	return RT_TRUE;
}

rt_bool_t hjt212_report_real_data_test(rt_uint8_t index)  //上传实时数据例子，跟示例文档发的数据一模一样
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024*2)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        das_localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {"20160928101320000"};
		//sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);


		char st[ST_LEN + 1] = {0};
		sprintf(st,"%d",ST_05);

		char CN[CN_LEN + 1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA);

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);
		
		char *sCp = rt_calloc(1, _CC_BUF_SZ);

	   // time_t now = time(RT_NULL);
	    //struct tm*  = localtime((&now);
	    sprintf(sCp,"DataTime=%s;","20160921085900");
		   
		{//此处读出配置信息，填充数据
			char *szfid[7] = {"Leq","130","111","112","108","109","110"}; 
			char *btValue = "0.000";
			eRealDataFlag_t dataflag = FLAG_N;
			
			for(int i = 0 ; i < 7; i++){
				//sprintf(szfid,"%d",40);
				hjt212_real_data_format_cp(0, szfid[i], 0, dataflag, "",5, sCp);	 //此函数可循环调用，每调用一次，将新的字符串将连接在后面
				if(i < 6){
					strcat(sCp,";");	 //最后一条数据不需要加";"号
				}
			}
		}

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, "123456", "YCDB0ZE0C00004", NULL, sCp, buf);
		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

static void hjt212_minutes_data_format_cp(
    rt_uint8_t index, 
    rt_bool_t  first,
    char *szFid,
	char *BeginTime,
	char *EndTime,
	char *DataTime,
	char *DATState,
	double Cou, 
	float Min, 
	float Avg, 
	float Max, 
	int pi,
	eRealDataFlag_t dataflag, 
	char *sCp)
{
	char tmp[50] = {0};
	
	if(BeginTime && BeginTime[0] && szFid){
		sprintf(tmp,"BeginTime=%s;",BeginTime);
		strcat(sCp,tmp);
		
	}
    memset(tmp, 0 ,sizeof(tmp));
	
	if(EndTime && EndTime[0] && szFid){
		sprintf(tmp,"EndTime=%s;",EndTime);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	if(DataTime && DataTime[0] && szFid && first){
		sprintf(tmp,"DataTime=%s;",DataTime);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

    if(DATState && DATState[0] && szFid && first && s_Hj212CfgData[index]->DATState_flag) {
		sprintf(tmp,"DATState=%s;",DATState);
		strcat(sCp,tmp);
	}
	memset(tmp, 0 ,sizeof(tmp));

    
	if(szFid && !s_Hj212CfgData[index]->no_cou){
		//strcat(sCp,",");	
		sprintf(tmp,"%s-Cou=%.*f",szFid, pi, Cou);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0, sizeof(tmp));

	if(szFid && !s_Hj212CfgData[index]->no_min){
		if(!s_Hj212CfgData[index]->no_cou){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Min=%.*f",szFid, pi, Min);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	if(szFid && !s_Hj212CfgData[index]->no_avg){
		if(!s_Hj212CfgData[index]->no_cou || !s_Hj212CfgData[index]->no_min){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Avg=%.*f",szFid, pi, Avg);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	if(szFid && !s_Hj212CfgData[index]->no_max){
		if(!s_Hj212CfgData[index]->no_cou || !s_Hj212CfgData[index]->no_min || !s_Hj212CfgData[index]->no_avg){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Max=%.*f",szFid, pi, Max);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

    if(szFid && (dataflag != FLAG_DISABLE) ){
        if(!s_Hj212CfgData[index]->no_cou || !s_Hj212CfgData[index]->no_min || !s_Hj212CfgData[index]->no_avg || !s_Hj212CfgData[index]->no_max){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Flag=%c",szFid,dataflag);
		strcat(sCp,tmp);
    }

	//strcat(sCp,";");	

}

rt_bool_t hjt212_report_real_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time)  //上传分钟数据
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024*2)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        das_localtime_r(&report_time, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN+1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[CN_LEN+1] = {0};
        if(s_Hj212CfgData[index]->REAL_CN > 0) {
		    sprintf(CN,"%d" , s_Hj212CfgData[index]->REAL_CN);
        }else {
            sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA);
        }

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);

        char mynid[64] = {0};
		char *sCp = rt_calloc(1, _CC_BUF_SZ);
        {
            struct tm lt;
            das_localtime_r(&report_time, &lt);
			char datatime[20] = {0};
		    if(s_Hj212CfgData[index]->DataTimeSec){
		        sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
            }else {
                sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,0);
            }
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_HJT212);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            // 分钟数据
                            char *newnid = node->xUp.szNid ? node->xUp.szNid : s_Hj212CfgData[index]->MN;
                            if(mynid[0] == '\0' || 0 == strcmp(newnid, mynid)) {
                                strcpy(mynid, newnid);
                                if (s_Hj212CfgData[index]->sharp_flag || (rt_tick_get() - node->xUp.xAvgUp.time >= s_Hj212CfgData[index]->ulPeriod) ) {
                                    if (node->xUp.xAvgUp.count > 0) {
                                        //var_double_t value = (node->xUp.xAvgUp.val_avg / node->xUp.xAvgUp.count);
                                        var_double_t value = 0;
                                        bVarManage_GetExtValue(node, node->xIo.btOutVarType, &value);
                                        //node->xUp.pi
                                        memset(buf, 0, _CC_BUF_SZ);
                                        hjt212_real_data_format_cp(first, node->xUp.szFid, (float)value, node->xIo.bErrFlag ? FLAG_D : FLAG_N, datatime,node->xUp.pi, buf);

                                        first = RT_FALSE;

                                        int sCPlen = strlen(sCp);
                                        if(sCPlen + strlen(buf) < _CC_BUF_SZ - ENCODE_LEN) {
                                            strcat(sCp, buf);
                                            strcat(sCp,";");
                                            *ppnode = node;
                                            
                                            node->xUp.xAvgUp.val_avg = 0;
                                            node->xUp.xAvgUp.val_min = 0;
                                            node->xUp.xAvgUp.val_max = 0;
                                            node->xUp.xAvgUp.count = 0;
                                            node->xUp.xAvgUp.time = rt_tick_get();
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
        }

        int sCPlen = strlen(sCp);
        if(sCPlen > 0) {
            sCp[sCPlen-1] = '\0';
    		//rt_uint8_t flag[0] = '1'; 
    		int len = 0;
    		if(s_Hj212CfgData[index]->no_qn == 1){
    		   len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, s_Hj212CfgData[index]->response_flag[0]?s_Hj212CfgData[index]->response_flag:NULL, sCp, buf);
    		}else {
              len = Hjt212Encode(qn, st, CN, s_Hj212CfgData[index]->PW, mynid, s_Hj212CfgData[index]->response_flag[0]?s_Hj212CfgData[index]->response_flag:NULL, sCp, buf);  
            }
            //rt_kprintf("hjt212 len = %d\n", len);
    		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		}
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

static rt_bool_t __hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time, int mins)  //上传分钟数据
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024*2)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        das_localtime_r(&report_time, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN+1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[CN_LEN+1] = {0};

        if(s_Hj212CfgData[index]->MIN_CN > 0){
		    sprintf(CN,"%d" , s_Hj212CfgData[index]->MIN_CN);
        }else {
		    sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA);
        }

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);

        char mynid[64] = {0};
        char *sCp = rt_calloc(1, _CC_BUF_SZ);
        {
            struct tm lt;
            das_localtime_r(&report_time, &lt);
			char datatime[20] = {0};
            if(s_Hj212CfgData[index]->DataTimeSec){
		        sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
            }else {
                sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,0);
            }
			char *BeginTime = datatime;
			char *EndTime = datatime;
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_HJT212);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            // 分钟数据
                            char *newnid = node->xUp.szNid ? node->xUp.szNid : s_Hj212CfgData[index]->MN;
                            if(mynid[0] == '\0' || 0 == strcmp(newnid, mynid)) {
                                VarAvg_t *avg = RT_NULL;
                                char *DATState = RT_NULL;
                                int time_diff = 60 * 1000;
                                strcpy(mynid, newnid);
                                if (mins == 1) {
                                    avg = &node->xUp.xAvgMin;
                                    DATState = DAT_STATE_1MIN;
                                    time_diff = 60 * 1000;
                                } else if (mins == 5) {
                                    avg = &node->xUp.xAvg5Min;
                                    DATState = DAT_STATE_5MIN;
                                    time_diff = 5 * 60 * 1000;
                                }
                                if (s_Hj212CfgData[index]->sharp_flag || (rt_tick_get() - avg->time >= rt_tick_from_millisecond(time_diff))) {
                                    if (avg->count > 0) {
                                        var_double_t value = (avg->val_avg / avg->count);

                                        memset(buf, 0, _CC_BUF_SZ);
                                        hjt212_minutes_data_format_cp(
                                            index, first, 
                                            node->xUp.szFid, RT_NULL, RT_NULL, datatime, DATState, 
                                            avg->val_avg, 
                                            avg->val_min, value, avg->val_max, 
                                            node->xUp.pi, 
                                            FLAG_N, buf);

                                        first = RT_FALSE;

                                        int sCPlen = strlen(sCp);
                                        if(sCPlen + strlen(buf) < _CC_BUF_SZ - ENCODE_LEN) {
                                            strcat(sCp, buf);
                                            strcat(sCp,";");
                                            *ppnode = node;
                                            
                                            avg->val_avg = 0;
                                            avg->val_min = 0;
                                            avg->val_max = 0;
                                            avg->count = 0;
                                            avg->time = rt_tick_get();
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
        }

        int sCPlen = strlen(sCp);
        if(sCPlen > 0) {
            sCp[sCPlen-1] = '\0';
    		//rt_uint8_t flag[0] = '1'; 
            int len = 0;
    		if(s_Hj212CfgData[index]->no_qn == 1){
    		   len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, s_Hj212CfgData[index]->response_flag[0]?s_Hj212CfgData[index]->response_flag:NULL, sCp, buf);
    		}else {
              len = Hjt212Encode(qn, st, CN, s_Hj212CfgData[index]->PW, mynid, s_Hj212CfgData[index]->response_flag[0]?s_Hj212CfgData[index]->response_flag:NULL, sCp, buf);  
            }
    		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		}
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time)  //上传分钟数据
{
    return __hjt212_report_minutes_data(index,  ppnode, report_time, 1);
}

rt_bool_t hjt212_report_5minutes_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time)  //上传分钟数据
{
    return __hjt212_report_minutes_data(index,  ppnode, report_time, 5);
}

rt_bool_t hjt212_report_hour_data(rt_uint8_t index, ExtData_t **ppnode, rt_time_t report_time)  //上传小时数据
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024*2)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        das_localtime_r(&report_time, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN + 1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[CN_LEN + 1] = {0};
        if(s_Hj212CfgData[index]->HOUR_CN > 0){
		    sprintf(CN,"%d" , s_Hj212CfgData[index]->HOUR_CN);
        }else {
		    sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA);
        }

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);

        char mynid[64] = {0};
		char *sCp = rt_calloc(1, _CC_BUF_SZ);
        {
            struct tm lt;
            das_localtime_r(&report_time, &lt);
			char datatime[20] = {0};
		    if(s_Hj212CfgData[index]->DataTimeSec){
		        sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
            }else {
                sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,0);
            }
			char *BeginTime = datatime;
			char *EndTime = datatime;
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_HJT212);
                    if (node) {
                        //var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            // 小时数据
                            char *newnid = node->xUp.szNid ? node->xUp.szNid : s_Hj212CfgData[index]->MN;
                            if(mynid[0] == '\0' || 0 == strcmp(newnid, mynid)) {
                                strcpy(mynid, newnid);
                                if (s_Hj212CfgData[index]->sharp_flag || (rt_tick_get() - node->xUp.xAvgHour.time >= rt_tick_from_millisecond(60 * 60 * 1000))) {
                                    if (node->xUp.xAvgHour.count > 0) {
                                        var_double_t value = (node->xUp.xAvgHour.val_avg / node->xUp.xAvgHour.count);

                                        memset(buf, 0, _CC_BUF_SZ);
                                        hjt212_minutes_data_format_cp(
                                            index, first,
                                            node->xUp.szFid, RT_NULL, RT_NULL, datatime, DAT_STATE_HOUR, 
                                            node->xUp.xAvgHour.val_avg, 
                                            node->xUp.xAvgHour.val_min, value, node->xUp.xAvgHour.val_max, 
                                            node->xUp.pi, 
                                            FLAG_N, buf);

                                        first = RT_FALSE;

                                        int sCPlen = strlen(sCp);
                                        if(sCPlen + strlen(buf) < _CC_BUF_SZ - ENCODE_LEN) {
                                            strcat(sCp, buf);
                                            strcat(sCp,";");
                                            *ppnode = node;

                                            node->xUp.xAvgHour.val_avg = 0;
                                            node->xUp.xAvgHour.val_min = 0;
                                            node->xUp.xAvgHour.val_max = 0;
                                            node->xUp.xAvgHour.count = 0;
                                            node->xUp.xAvgHour.time = rt_tick_get();
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
        }

        int sCPlen = strlen(sCp);
        if(sCPlen > 0) {
            sCp[sCPlen-1] = '\0';
    		//rt_uint8_t flag[0] = '1'; 
    		//int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, NULL, sCp, buf);
            int len = 0;
    		if(s_Hj212CfgData[index]->no_qn == 1){
    		    len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, s_Hj212CfgData[index]->response_flag[0]?s_Hj212CfgData[index]->response_flag:NULL, sCp, buf);
    		}else {
              len = Hjt212Encode(qn, st, CN, s_Hj212CfgData[index]->PW, mynid, s_Hj212CfgData[index]->response_flag[0]?s_Hj212CfgData[index]->response_flag:NULL, sCp, buf);  
            }
    		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		}
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_report_system_time(rt_uint8_t index)  //上传系统时间
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        das_localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN + 1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[CN_LEN + 1] = {0};
		sprintf(CN, "%d" , CMD_REQUEST_UPLOAD_TIME);

		char sCp[64] = {0};
		sprintf(sCp,"QN=%s;SystemTime=%04d%02d%02d%02d%02d%02d",qn,lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, NULL, sCp, buf);
		cc_net_send(index, 0, (const rt_uint8_t *)buf, len);
		rt_free(buf);
	}
    return RT_TRUE;
}


 rt_bool_t hjt212_request_get_system_time(rt_uint8_t index)  //上位机-->现场机 用于测试解析
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        das_localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[QN_LEN + 1] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[ST_LEN + 1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[CN_LEN + 1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_TIME);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "1", "\0", buf);
		
		//cc_net_send(index, buf, len);

		// rt_kprintf("len = %d, strlen = %d\r\n",len,strlen(buf));
		 rt_kprintf("%s",buf);
		 /*
		 for(int i = 0; i < len; i++ ) {
       		 bMDQueueWrite( s_xBleRcvQueue, buf[i] );
   		 }
		 OSQueueMsgSend( MSG_BLE_RCV_DATA);
                 */
		 
		rt_free(buf);
	}
    return RT_TRUE;
}


/*
static void prvTestTask(rt_uint8_t index)
{
	  rt_uint8_t *buf = NULL;
	  rt_uint16_t nReadLen = 0;
     _HJT212_PutBytes(index, buf, nReadLen);           
}
*/

static void prvHjt212DefaultCfg(int index)
{
	memcpy(s_Hj212CfgData[index]->ST , "31", ST_LEN);
	memcpy(s_Hj212CfgData[index]->PW , "123456", PW_LEN);
	memcpy(s_Hj212CfgData[index]->MN , "12345678901234", MN_LEN);
	if(g_host_cfg.szId[0]) {
	    strncpy(s_Hj212CfgData[index]->MN , g_host_cfg.szId, MN_LEN);
	}
    s_Hj212CfgData[index]->no_cou = 1;
    s_Hj212CfgData[index]->no_min = 1;
    s_Hj212CfgData[index]->no_max = 1;
    s_Hj212CfgData[index]->DATState_flag = 0;
}

rt_bool_t hjt212_open(rt_uint8_t index)
{
    hjt212_close(index);

    RT_KERNEL_FREE(s_Hj212CfgData[index]);
    s_Hj212CfgData[index] = RT_KERNEL_CALLOC(sizeof(HJT212_Cfg_t));

    RT_KERNEL_FREE(s_pCCBuffer[index][0]);
    s_pCCBuffer[index][0] = RT_KERNEL_CALLOC(HJT212_BUF_SIZE);
    s_CCBufferPos[index][0] = 0;
    s_eRcvState[index] = HJT212_R_S_HEAD;
    s_cc_reinit_flag[index][0] = RT_FALSE;

    prvHjt212DefaultCfg(index);
    {
        char buf[64] = "";
        ini_t *ini;
        sprintf(buf, HJT212_INI_CFG_PATH_PREFIX "%d" ".ini", index);
        ini = ini_load(buf);

        if (ini) {
            const char *str = ini_getstring(ini, "common", "st", "");
            if(str[0]) strncpy(s_Hj212CfgData[index]->ST, str, sizeof(s_Hj212CfgData[index]->ST));
            str = ini_getstring(ini, "common", "pw", "");
            if(str[0]) strncpy(s_Hj212CfgData[index]->PW, str, sizeof(s_Hj212CfgData[index]->PW));
            str = ini_getstring(ini, "common", "mn", "");
            if(str[0]) strncpy(s_Hj212CfgData[index]->MN, str, sizeof(s_Hj212CfgData[index]->MN));

            str = ini_getstring(ini, "common", "response_flag", "");
            if(str[0]) strncpy(s_Hj212CfgData[index]->response_flag, str, sizeof(s_Hj212CfgData[index]->response_flag));

            s_Hj212CfgData[index]->ulPeriod = ini_getint(ini, "common", "period", 1000);
#if NET_HAS_GPRS
            if(NET_IS_GPRS(index)) {
                if(s_Hj212CfgData[index]->ulPeriod < 3000) s_Hj212CfgData[index]->ulPeriod = 3000;
            } else {
                if(s_Hj212CfgData[index]->ulPeriod < 1000) s_Hj212CfgData[index]->ulPeriod = 1000;
            }
#else 
               if(s_Hj212CfgData[index]->ulPeriod < 1000) s_Hj212CfgData[index]->ulPeriod = 1000;
#endif
            
            s_Hj212CfgData[index]->REAL_CN = ini_getint(ini, "common", "REAL_CN", 0);
            s_Hj212CfgData[index]->MIN_CN =  ini_getint(ini, "common", "MIN_CN", 0);
            s_Hj212CfgData[index]->HOUR_CN = ini_getint(ini, "common", "HOUR_CN", 0);
            s_Hj212CfgData[index]->verify_flag = ini_getboolean(ini, "common", "verify_flag", 0);
            s_Hj212CfgData[index]->real_flag = ini_getboolean(ini, "common", "real_flag", 0);
            s_Hj212CfgData[index]->min_flag = ini_getboolean(ini, "common", "min_flag", 0);
            s_Hj212CfgData[index]->min_5_flag = ini_getboolean(ini, "common", "min_5_flag", 0);
            s_Hj212CfgData[index]->hour_flag = ini_getboolean(ini, "common", "hour_flag", 0);
            s_Hj212CfgData[index]->sharp_flag = ini_getboolean(ini, "common", "sharp_flag", 0);
            s_Hj212CfgData[index]->no_cou = ini_getboolean(ini, "common", "no_cou", 0);
            s_Hj212CfgData[index]->no_min = ini_getboolean(ini, "common", "no_min", 0);
            s_Hj212CfgData[index]->no_max = ini_getboolean(ini, "common", "no_max", 0);
            s_Hj212CfgData[index]->no_avg = ini_getboolean(ini, "common", "no_avg", 0);
            s_Hj212CfgData[index]->no_qn = ini_getboolean(ini, "common", "no_qn", 0);
            s_Hj212CfgData[index]->DATState_flag = ini_getboolean(ini, "common", "DATState_flag", 1);
            s_Hj212CfgData[index]->DataTimeSec = ini_getboolean(ini, "common", "DataTimeSec", 0);
            
            s_Hj212CfgData[index]->enable_heart = ini_getint(ini, "common", "enable_heart", 0);
            if(s_Hj212CfgData[index]->enable_heart > 0 && s_Hj212CfgData[index]->enable_heart < 10){
                s_Hj212CfgData[index]->enable_heart = 10;
            }

            s_Hj212UpdateData[index].last_min_time = das_get_time();
            s_Hj212UpdateData[index].last_min_5_time = das_get_time();
            s_Hj212UpdateData[index].last_hour_time = das_get_time();

            rt_kprintf("index: %d, enable_heart: %d, hjtcfg->real_flag: %d\n",index, s_Hj212CfgData[index]->enable_heart, s_Hj212CfgData[index]->real_flag);

            ini_free(ini);
        } else {
            rt_kprintf(" %s load failed\r\n", buf);
        }
    }

    BOARD_CREAT_NAME(szMq, "bjmq_%d", index);
    s_CCDataQueue[index] = rt_mq_create(szMq, sizeof(HJT212_Data_t), 3, RT_IPC_FLAG_PRIO);
    return RT_TRUE;
}

void hjt212_close(rt_uint8_t index)
{
    for (int i = 0; i < 5 * 10; i++) {
        if (s_hjt212_work_thread[index]) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
    }

    hjt212_exitwork(index);

    if (s_CCDataQueue[index]) {
        rt_mq_delete(s_CCDataQueue[index]);
        s_CCDataQueue[index] = RT_NULL;
    }
    RT_KERNEL_FREE(s_pCCBuffer[index][0]);
    s_pCCBuffer[index][0] = RT_NULL;
}

static void hjt212_work_task(void *parameter);

void hjt212_startwork(rt_uint8_t index)
{
    if(RT_NULL == s_hjt212_work_thread[index]) {
        BOARD_CREAT_NAME(szWork, "hjt212_%d", index);
        s_hjt212_work_thread[index] = \
            rt_thread_create(szWork,
                                    hjt212_work_task,
                                    (void *)(long)index,
                                    2048,
                                    20, 20);
        if (s_hjt212_work_thread[index] != RT_NULL) {
            rt_thddog_register(s_hjt212_work_thread[index], 30);
            rt_thread_startup(s_hjt212_work_thread[index]);
        }
    }
}

void hjt212_exitwork(rt_uint8_t index)
{
    s_cc_reinit_flag[index][0] = RT_TRUE;
}

static void hjt212_work_task(void *parameter)
{
    rt_uint8_t index = (int)(long)parameter;
    
    while(1) {
        HJT212_Data_t xData;
        //tcpip_cfg_t *pcfg = &g_tcpip_cfgs[index];
        HJT212_Cfg_t *hjtcfg = s_Hj212CfgData[index];
        HJT212_Update_t *hjtup = &s_Hj212UpdateData[index];
        //rt_uint32_t ulHeartbeatTick;
        int login_err_count = 0;

_START:
        rt_thddog_feed("net_waitconnect");
        net_waitconnect(index);
        {
            //var_int32_t up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index));
            //int period = up_interval>0?(up_interval/60):180;

            rt_kprintf("...start verify\n");
            rt_kprintf("hjt212_login_verify_req\n");
            rt_thddog_suspend("hjt212_login_verify_req");
            hjt212_login_verify_req(index);
// 暂时不需要校验过程
            if(hjtcfg->verify_flag) {
                while (1) {
                    //s_eVerifyState[index] = HJT212_VERIFY_REQ;
                    rt_kprintf("hjt212_login_verify_req\n");
                    rt_thddog_suspend("hjt212_login_verify_req");
                    hjt212_login_verify_req(index);
                    rt_thddog_suspend("rt_mq_recv verify");
                    if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t), 15* RT_TICK_PER_SECOND)) {
                        //if (CMD_LOGIN_RESPONSE == xData.eType) {
                            // 这里可以对时 xData.QN
                            break;
                        //}
                    } else {
                        rt_thddog_suspend("cc_net_disconnect");
                        cc_net_disconnect(index);
                        rt_thread_delay(5 * RT_TICK_PER_SECOND);
                        goto _START;
                    }
                }
            }
            s_cc_reinit_flag[index][0] = RT_FALSE;
            rt_kprintf("...hjt212_login_verify ok !\n");
            ulHjt212HeartbeatTick[index] = rt_tick_get();
            while (!s_cc_reinit_flag[index][0]) {
                ExtData_t *node = RT_NULL;
                //rt_thddog_feed("UpdateAvgValue");
                rt_thddog_feed("net_waitconnect UpdateAvgValue");
                net_waitconnect(index);
                rt_enter_critical();
                {
                    while (!s_cc_reinit_flag[index][0]) {
                        node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_HJT212);
                        if (node) {
                            var_double_t ext_value = 0;
                            if (node->xUp.bEnable) {
                                if (bVarManage_GetExtValue(node, node->xIo.btOutVarType, &ext_value)) {
                                    if(hjtcfg->real_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgUp, ext_value);
                                    }
                                    if(hjtcfg->min_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgMin, ext_value);
                                    }
                                    if(hjtcfg->min_5_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvg5Min, ext_value);
                                    }
                                    if(hjtcfg->hour_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgHour, ext_value);
                                    }
                                }
                            }
                        } else {
                            break;
                        }
                    }
                }
                rt_exit_critical();

                rt_thddog_suspend("rt_mq_recv wait");
                if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t), 5 * RT_TICK_PER_SECOND)) {
                    switch (xData.eType) {
                    case CMD_REQUEST_SET_REAL_DATA_INTERVAL:
                    {
                        rt_kprintf("recv CMD_REQUEST_SET_REAL_DATA_INTERVAL !\n");
                        break;
                    }
                    default: 
                    {
                        rt_kprintf("recv %d\n", xData.eType);
                        break;
                    }
                    }
                }
                rt_thddog_resume();

                // 每 15 分钟, 发送一次心跳
               // rt_kprintf("index: %d, enable_heart: %d, hjtcfg->real_flag: %d\n",index, hjtcfg->enable_heart, hjtcfg->real_flag);
               if(hjtcfg->enable_heart >= 10 ){
                    if (rt_tick_get() - ulHjt212HeartbeatTick[index] >= rt_tick_from_millisecond(hjtcfg->enable_heart * 1000)) {
	                    rt_thddog_suspend("hjt212_login_verify_req 20 sec heart");
                        rt_kprintf(">>>>>>>>>>>>>>> hjt212_login_verify_req %d sec heart\r\n\n",hjtcfg->enable_heart);
	                    hjt212_login_verify_req(index);
	                    rt_thddog_resume();
	                    ulHjt212HeartbeatTick[index] = rt_tick_get();
	                    // 不需要等待回复
	                    if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t), 20 * RT_TICK_PER_SECOND)) {
	                        //if (CMD_LOGIN_RESPONSE == xData.eType) {
	                            // 这里可以对时 xData.QN
	                            rt_kprintf(">>>>>>>>>>> wait the respon!\n\n");
	                            login_err_count = 0;
	                        //}
	                    } else {
	                        login_err_count++;
	                    }
	                    // 第一次 0, 第二次 15 分钟, 第三次 30 分钟
	                    if(login_err_count >= 3) {
	                        cc_net_disconnect(index);
	                        rt_thread_delay(5 * RT_TICK_PER_SECOND);
	                        goto _START;
	                    }
					}
                }

                // 根据配置上报数据
                {
                    //up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index));
                    if(hjtcfg->min_flag || hjtcfg->min_5_flag || hjtcfg->hour_flag || hjtcfg->real_flag) {
                        node = RT_NULL;
                        
                        int period =  s_Hj212CfgData[index]->ulPeriod / 1000/60;
                    
                        if ( (!hjtcfg->sharp_flag) || (period <= 0) ) {
                            if (hjtcfg->real_flag) {
                                rt_time_t report_real_time = time(NULL);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_real_data");
                                    //rt_kprintf("hjt212_report_real_data!\n\n");
                                    hjt212_report_real_data(index, &node, report_real_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                            }
                        }else {
                            rt_time_t report_real_time = time(NULL);
                            node = RT_NULL;
                             if (hjtcfg->real_flag && ( das_time_get_day_min(hjtup->last_real_time) / period) != (das_time_get_day_min(report_real_time)/period) ) {
                                rt_kprintf("sharp hjt212_report_real_data: %d!\n\n", period);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_real_data");
                                    //rt_kprintf("hjt212_report_real_data!\n\n");
                                    hjt212_report_real_data(index, &node, report_real_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                                hjtup->last_real_time = report_real_time;
                             }
                        }

                        
                        if (!hjtcfg->sharp_flag) {
                            node = RT_NULL;
                            if (hjtcfg->min_flag) {
                                rt_time_t report_minutes_time = time(NULL);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_minutes_data");
                                    hjt212_report_minutes_data(index, &node, report_minutes_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                            }
                            node = RT_NULL;
                            if (hjtcfg->min_5_flag) {
                                rt_time_t report_5minutes_time = time(NULL);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_minutes_data");
                                    hjt212_report_5minutes_data(index, &node, report_5minutes_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                            }
                            node = RT_NULL;
                            if (hjtcfg->hour_flag) {
                                rt_time_t report_hour_time = time(NULL);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_hour_data");
                                    hjt212_report_hour_data(index, &node, report_hour_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                            }
                        } else {
                            rt_time_t now_time = time(NULL);
                            node = RT_NULL;
                            if (hjtcfg->min_flag && 
                                das_time_get_day_min(hjtup->last_min_time) != das_time_get_day_min(now_time)) {
                                rt_kprintf("hjt212_report_minutes_data 1 mins : %u\n", now_time);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_minutes_data");
                                    hjt212_report_minutes_data(index, &node, now_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                                hjtup->last_min_time = now_time;
                            }
                            node = RT_NULL;
                            if (hjtcfg->min_5_flag && 
                                (das_time_get_day_min(hjtup->last_min_5_time) / 5) != (das_time_get_day_min(now_time) / 5)) {
                                rt_kprintf("hjt212_report_minutes_data 5 mins : %u\n", now_time);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_minutes_data");
                                    hjt212_report_5minutes_data(index, &node, now_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                                hjtup->last_min_5_time = now_time;
                            }
                            node = RT_NULL;
                            if (hjtcfg->hour_flag && 
                                das_time_get_hour(hjtup->last_hour_time) != das_time_get_hour(now_time)) {
                                rt_kprintf("hjt212_report_hour_data : %u\n", now_time);
                                while(1) {
                                    rt_thddog_suspend("hjt212_report_hour_data");
                                    hjt212_report_hour_data(index, &node, now_time);
                                    rt_thddog_resume();
                                    if(!node) break;
                                }
                                hjtup->last_hour_time = now_time;
                            }
                        }
                    }
                }
            }
            if (s_cc_reinit_flag[index][0]) goto _START;
        }
    }

    s_hjt212_work_thread[index] = RT_NULL;
    rt_thddog_unreg_inthd();
}



