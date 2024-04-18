#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dm_lib.h"
#include "dm_crc8.h"
#include "dm_crc32.h"
#include "dm_md5.h"
#include "dm_aes.h"
#include "os_platform.h"

#include "rtdef.h"
#include "das_os.h"

static int32_t oat_pick_head(struct Head *pkg, const char *buf);


//comm 
static int32_t bigstore(void)
{
	int32_t num = 0x12345678;
	uint8_t cdat = *(uint8_t*)(&num);

	if(cdat == 0x12){
	#if DEBUG_CJY_OAT
	    dm101_debug("host is bigend\n");
	#endif
		return 0;
	}else
		return -1;
}

int32_t change_order( uint8_t *data, uint8_t len)
{
	#if DEBUG_CJY_OAT
	    dm101_debug("change_order\n");
	#endif
	uint8_t tmp[8];
	memcpy(tmp, data, len);
	switch (len){
		case 8:
			data[7] = tmp[len -8];
			data[6] = tmp[len -7];
			data[5] = tmp[len -6];
			data[4] = tmp[len -5];
		case 4:
			data[3] = tmp[len -4];
			data[2] = tmp[len -3];			
		case 2:
			data[1] = tmp[len -2];
			data[0] = tmp[len -1];
		default:
			break;
		}
	return 0;
}

static int32_t change_order_Head( struct Head *pkg)
{
	if(bigstore() == 0)
		return 0;
	else if(pkg != NULL){
		change_order((uint8_t *)&pkg->sn, sizeof(pkg->sn));
		change_order((uint8_t *)&pkg->msgcode, sizeof(pkg->msgcode));
		change_order((uint8_t *)&pkg->payload_length, sizeof(pkg->payload_length));
		change_order((uint8_t *)&pkg->timestamp, sizeof(pkg->timestamp));
		change_order((uint8_t *)&pkg->src_id, sizeof(pkg->src_id));
		change_order((uint8_t *)&pkg->dest_id, sizeof(pkg->dest_id));
		change_order((uint8_t *)&pkg->token, sizeof(pkg->token));
		change_order((uint8_t *)&pkg->code, sizeof(pkg->code));
		
		return 1;
	}

}

static int32_t change_order_dm_crc32( struct Dm101pkg *dm101pkg)
{
	if(bigstore() == 0){
		return 0;
	}else if(dm101pkg != NULL){
		change_order((uint8_t *)&dm101pkg->dm_crc32, sizeof(dm101pkg->dm_crc32));		
		return 1;
	}

}

static void make_aec_key( char*key, char *auth,  char* timestamp)
{
	DM_MD5Context context;
	uint8_t token_buf_len = AUTH_LEN + TIME_STAMP_LEN;
	uint8_t token_buf[AUTH_LEN + TIME_STAMP_LEN] ;

	memcpy(token_buf, auth, AUTH_LEN);
	memcpy(token_buf + AUTH_LEN, timestamp , TIME_STAMP_LEN);
	
	#if DEBUG_CJY_OAT
	{
		int i;	
		
		for(i = 0,dm101_debug("key =0x"); i < 16;i++){
			dm101_debug("%02x", (uint8_t)token_buf[i]);
		}
		dm101_debug("\n");
	}
	#endif
	
	DM_MD5_Init(&context);
	DM_MD5_Update(&context, token_buf, token_buf_len);
	DM_MD5_Final(  &context, key);

	#if DEBUG_CJY_OAT
	{
		int i;
		for(i = 0,dm101_debug("key =0x"); i < 16;i++){
			dm101_debug("%02x", (uint8_t)key[i]);
		}
		dm101_debug("\n");
	}
	#endif
}

static int32_t DM_AES_Encrypt_cjy(unsigned char *pPlainText, unsigned int *nDataLen, const unsigned char *pIV)
{
	DM_AES_Init(pIV);
	//PKCS7
	PKCS7_EnPadding(pPlainText,*nDataLen,nDataLen);
	DM_AES_Encrypt(pPlainText, pPlainText, *nDataLen, pIV);
}

static int32_t DM_AES_Decrypt_cjy(unsigned char *pPlainText, unsigned int *nDataLen, const unsigned char *pIV)
{
	DM_AES_Init(pIV);
	DM_AES_Decrypt(pPlainText, pPlainText, *nDataLen, pIV);

	//PKCS7
	PKCS7_DePadding(pPlainText,*nDataLen,nDataLen);
}

uint32_t fomat_server_info(char *buf, struct Server_info server_info)
{
	if(buf == NULL)
		return -1;
	sprintf(buf,"{\
	\"id\":\"%d\",\
	\"name\":\"%s\",\
	\"addr\":\"%s\",\
	\"port\":%d\
	}"
	,server_info.id, server_info.name, server_info.addr, server_info.port);
}


char *show_head(const struct Head *pkg, const char *buf , char *showbuf)
{
	struct Head head;
	//dm101_debug("show_head\n");
	if(pkg != NULL){
		head = *pkg;
		//memcpy(&head, pkg, sizeof(head));
	}else if(buf != NULL){
		oat_pick_head(&head, buf);
		//dm101_debug("show_head pkg.payload_length = %d\n", head.payload_length);
	}else{
		dm101_debug("arg1 arg2 are NULL\n");
		return NULL;
	}
    if (showbuf) {
        int ofs = sprintf(showbuf,"\tmagic = %c%c%c%c \n",head.magic[0],head.magic[1],head.magic[2],head.magic[3]);
    	ofs += sprintf(showbuf+ofs,"\tver = %d \n",head.ver);
    	ofs += sprintf(showbuf+ofs,"\ttype = %d \n",head.type);
    	ofs += sprintf(showbuf+ofs,"\tsn = %d \n",head.sn);
    	ofs += sprintf(showbuf+ofs,"\tmsgcode = 0x%x \n",head.msgcode);
    	ofs += sprintf(showbuf+ofs,"\tformat = %d \n",head.format);
    	ofs += sprintf(showbuf+ofs,"\tmflag = %d \n",head.mflag);
    	ofs += sprintf(showbuf+ofs,"\tplayload_length = %d \n",head.payload_length);
    	ofs += sprintf(showbuf+ofs,"\ttimestamp = %ld \n",head.timestamp);
    	ofs += sprintf(showbuf+ofs,"\tsrc_id = %lx \n",head.src_id);
    	ofs += sprintf(showbuf+ofs,"\tdest_id = %lx \n",head.dest_id);
    	ofs += sprintf(showbuf+ofs,"\ttoken = 0x%lx \n",head.token);
    	ofs += sprintf(showbuf+ofs,"\tcode = 0x%x \n",head.code);
    	ofs += sprintf(showbuf+ofs,"\treserve[0] = 0x%x \n",head.reserve[0]);
    	ofs += sprintf(showbuf+ofs,"\tdm_crc8 = 0x%x \n",head.dm_crc8);
    } else {
        dm101_debug("\tmagic = %c%c%c%c \n",head.magic[0],head.magic[1],head.magic[2],head.magic[3]);
    	dm101_debug("\tver = %d \n",head.ver);
    	dm101_debug("\ttype = %d \n",head.type);
    	dm101_debug("\tsn = %d \n",head.sn);
    	dm101_debug("\tmsgcode = 0x%x \n",head.msgcode);
    	dm101_debug("\tformat = %d \n",head.format);
    	dm101_debug("\tmflag = %d \n",head.mflag);
    	dm101_debug("\tplayload_length = %d \n",head.payload_length);
    	dm101_debug("\ttimestamp = %ld \n",head.timestamp);
    	dm101_debug("\tsrc_id = %lx \n",head.src_id);
    	dm101_debug("\tdest_id = %lx \n",head.dest_id);
    	dm101_debug("\ttoken = 0x%lx \n",head.token);
    	dm101_debug("\tcode = 0x%x \n",head.code);
    	dm101_debug("\treserve[0] = 0x%x \n",head.reserve[0]);
    	dm101_debug("\tdm_crc8 = 0x%x \n",head.dm_crc8);
    }
	//sprintf(showbuf, "magic = %c%c%c%c \n",head.magic[0],head.magic[1],head.magic[2],head.magic[3]);
	return showbuf;
}


static int32_t head_format_oat(struct Dm101pkg dm101pkg, char *buf, char *auth)
{
	if(NULL == buf ) 
		return -1;
	memcpy((void *)dm101pkg.head.magic, MAGIC, MAGIC_LEN);
	dm101pkg.head.ver = MSG_VERSION;

	dm101pkg.head.timestamp = get_utc_time();

	//dm101_debug( "%s",show_head(&cjypkg.head, NULL, NULL) );
	//utc 转换网络字节序
	
	//dm101_debug("pkg.payload_length = %ld\n", pkg.payload_length);	
	change_order_Head(&dm101pkg.head);//转换为网络字节序
	//dm101_debug("pkg.payload_length = %ld\n", pkg.payload_length);

	//create token
	if(auth != NULL){
		DM_MD5Context context;
		uint32_t token_buf_len = SRC_ID_LEN + TIME_STAMP_LEN + AUTH_LEN + MSGCODE_LEN;
		char token_buf[SRC_ID_LEN + TIME_STAMP_LEN + AUTH_LEN + MSGCODE_LEN];
		
		memcpy((uint8_t*)token_buf, (uint8_t*)&dm101pkg.head.src_id, SRC_ID_LEN);
		memcpy((uint8_t*)token_buf+SRC_ID_LEN, (uint8_t*)&dm101pkg.head.timestamp, TIME_STAMP_LEN);
		memcpy((uint8_t*)token_buf+SRC_ID_LEN+TIME_STAMP_LEN, (uint8_t*)auth, AUTH_LEN);
		memcpy((uint8_t*)token_buf+SRC_ID_LEN+TIME_STAMP_LEN+AUTH_LEN, (uint8_t*)&dm101pkg.head.msgcode, MSGCODE_LEN);

		DM_MD5_Init(&context);
		DM_MD5_Update(&context, token_buf, token_buf_len);
		DM_MD5_Final_16(  &context, (unsigned char*)&dm101pkg.head.token);
	}
	

	memcpy((uint8_t *)buf, (uint8_t *)dm101pkg.head.magic, MAGIC_LEN);
	memcpy((uint8_t *)buf + OFFSIZE_VAR, (uint8_t*)&dm101pkg.head.ver, sizeof(dm101pkg.head.ver));
	memcpy((uint8_t *)buf + OFFSIZE_TYPE, (uint8_t *)&dm101pkg.head.type, sizeof(dm101pkg.head.type));

	memcpy((uint8_t *)buf + OFFSIZE_SN, (uint8_t *)&dm101pkg.head.sn, sizeof(dm101pkg.head.sn));
	memcpy((uint8_t *)buf + OFFSIZE_MSGCODE, (uint8_t *)&dm101pkg.head.msgcode, sizeof(dm101pkg.head.msgcode));
	memcpy((uint8_t *)buf + OFFSIZE_FORMAT, (uint8_t *)&dm101pkg.head.format, sizeof(dm101pkg.head.format));
	memcpy((uint8_t *)buf + OFFSIZE_MFLAG, (uint8_t *)&dm101pkg.head.mflag, sizeof(dm101pkg.head.mflag));
	memcpy((uint8_t *)buf + OFFSIZE_PAYLOAD_LEN, (uint8_t *)&dm101pkg.head.payload_length, sizeof(dm101pkg.head.payload_length));
	memcpy((uint8_t *)buf + OFFSIZE_TIME_STAMP, (uint8_t *)&dm101pkg.head.timestamp, sizeof(dm101pkg.head.timestamp));	
	memcpy((uint8_t *)buf + OFFSIZE_SRC_ID, (uint8_t *)&dm101pkg.head.src_id, sizeof(dm101pkg.head.src_id));
	memcpy((uint8_t *)buf + OFFSIZE_DEST_ID, (uint8_t *)&dm101pkg.head.dest_id, sizeof(dm101pkg.head.dest_id));
	memcpy((uint8_t *)buf + OFFSIZE_TOKEN, (uint8_t *)&dm101pkg.head.token, sizeof(dm101pkg.head.token));
	memcpy((uint8_t *)buf + OFFSIZE_CODE, (uint8_t *)&dm101pkg.head.code, sizeof(dm101pkg.head.code));
	memcpy((uint8_t *)buf + OFFSIZE_RESERVE, (uint8_t *)&dm101pkg.head.reserve, sizeof(dm101pkg.head.reserve));
    //create crc
    dm101pkg.head.dm_crc8 = dm_crc8(0, (void *)buf, HEAD_LEN - CRC8_LEN);
    //dm101_debug("\tdm_crc8 = %d\n", cjypkg.head.dm_crc8);
	memcpy((uint8_t *)buf + OFFSIZE_CRC8, (uint8_t *)&dm101pkg.head.dm_crc8, sizeof(dm101pkg.head.dm_crc8));

	change_order_Head(&dm101pkg.head);//转换为主机序
    //show_head(&dm101pkg.head, NULL, NULL);

	//add playload_buf
	//memcpy((void *)buf + HEAD_LEN, (void *)&cjypkg.pload_buf, sizeof(cjypkg.head.payload_length));

	return 0;
}

static int32_t oat_pick_head(struct Head *pkg, const char *buf)
{
	if(NULL == buf ){
		dm101_debug("oat_pick_head buf =NULL\n");
		return -1;
	}
	if(NULL== pkg){
		dm101_debug("oat_pick_head pkg =NULL\n");
		return -1;
	}
	
	memcpy((uint8_t *)pkg->magic, (uint8_t *)buf, MAGIC_LEN);
	memcpy((uint8_t *)&pkg->ver, (uint8_t *)buf + OFFSIZE_VAR, sizeof(pkg->ver));
	memcpy((uint8_t *)&pkg->type, (uint8_t *)buf + OFFSIZE_TYPE, sizeof(pkg->type));

	memcpy((uint8_t *)&pkg->sn, (uint8_t *)buf + OFFSIZE_SN, sizeof(pkg->sn));
	memcpy((uint8_t *)&pkg->msgcode, (uint8_t *)buf + OFFSIZE_MSGCODE, sizeof(pkg->msgcode));
	memcpy((uint8_t *)&pkg->format, (uint8_t *)buf + OFFSIZE_FORMAT, sizeof(pkg->format));
	memcpy((uint8_t *)&pkg->mflag, (uint8_t *)buf + OFFSIZE_MFLAG, sizeof(pkg->mflag));
	memcpy((uint8_t *)&pkg->payload_length, (uint8_t *)buf + OFFSIZE_PAYLOAD_LEN, sizeof(pkg->payload_length));
	memcpy((uint8_t *)&pkg->timestamp, (uint8_t *)buf + OFFSIZE_TIME_STAMP, sizeof(pkg->timestamp));
	memcpy((uint8_t *)&pkg->src_id, (uint8_t *)buf + OFFSIZE_SRC_ID, sizeof(pkg->src_id));
	memcpy((uint8_t *)&pkg->dest_id, (uint8_t *)buf + OFFSIZE_DEST_ID, sizeof(pkg->dest_id));
	memcpy((uint8_t *)&pkg->token, (uint8_t *)buf + OFFSIZE_TOKEN, sizeof(pkg->token));
	memcpy((uint8_t *)&pkg->code, (uint8_t *)buf + OFFSIZE_CODE, sizeof(pkg->code));
	memcpy((uint8_t *)&pkg->reserve, (uint8_t *)buf + OFFSIZE_RESERVE, sizeof(pkg->reserve));
	memcpy((uint8_t *)&pkg->dm_crc8, (uint8_t *)buf + OFFSIZE_CRC8, sizeof(pkg->dm_crc8));

	change_order_Head(pkg);
	//dm101_debug("oat_pick_head pkg.payload_length = %ld\n", pkg->payload_length);

	//verfier crc
    uint8_t headdm_crc8 = dm_crc8(0, (void *)buf, HEAD_LEN - CRC8_LEN);
	if(USE_CRC == 1 && headdm_crc8 != pkg->dm_crc8){
		return -1;
	}

	return 0;
}



//c->s
static int32_t get_base_info(void *infobuf, const uint32_t infobuf_limit, uint32_t *info_len)
{
	struct Base_info base_info;
    memset(&base_info, 0, sizeof(base_info));
    dm101_fill_base_info(&base_info);
	
	sprintf(infobuf ,"{\"type\":\"%s\",\"ver\":\"%s\",\"name\":\"%s\"}" , base_info.type, base_info.ver, base_info.name);

	*info_len = strlen(infobuf);
	
	return 0;
}

static int32_t make_data_buf(uint8_t *outbuf , uint32_t outbuf_limit, const struct Data_pkg *pkg ,uint32_t *totalsize)
{
	int32_t i;
    int32_t tmp_len = 0;
	int8_t tmp_buf[100];
	
    if(outbuf == NULL || pkg == NULL || pkg->data == NULL || NULL == totalsize){
		dm101_debug("outbuf or pkg or pkg->data or totalsize is null\n");
	    return -1;
    }
    memset(outbuf, 0, outbuf_limit);

	//make time
	sprintf(outbuf + tmp_len, "{\"timestamp\":%ld, \"data\":[", pkg->timestamp);
	tmp_len = strlen(outbuf);
	
	//make data
	for(i = 0; i < pkg->data_cnt; i++){
	    sprintf(tmp_buf, "{\"sid\":\"%s\",\"id\":\"%s\", \"value\":%.*f, \"status\":%d},",\
			pkg->data[i].sid, pkg->data[i].id, pkg->data[i].pi,(isnan(pkg->data[i].value) ? 0 : pkg->data[i].value), pkg->data[i].status);
		tmp_len = strlen(outbuf);
        //dm101_debug("tmp_len: %d,outbuf_limit: %d\r\n",tmp_len,outbuf_limit);
		if(outbuf_limit - tmp_len < strlen(tmp_buf)) {
			dm101_debug("outof tmp_buf outbuf_limit - tmp_len = %d,size = %d\n", outbuf_limit - tmp_len, (int)strlen(tmp_buf));
			goto OUTBUF;
		}
		memcpy(outbuf + tmp_len, tmp_buf, strlen(tmp_buf));
	}
	if( outbuf[strlen(outbuf) - 1] == ',')
		outbuf[strlen(outbuf) - 1] = 0;
	
	tmp_len = strlen(outbuf);
	if(outbuf_limit - tmp_len < 3)
        goto OUTBUF;
	
	sprintf(outbuf + tmp_len, "]}");
	*totalsize = strlen(outbuf);

	return 0;
	
OUTBUF:
	dm101_debug("out of outbuf\n");
	return -1;	
	
}


//s->c
static int32_t set_base_info(uint8_t *outbuf, uint8_t *info , uint32_t *totalsize)
{
	if(NULL == outbuf || NULL == info || NULL == totalsize){
		dm101_debug("outbuf or info or totalsize is null\n");
		return -1;
	}

	sprintf(outbuf, "{\"name\":\"\%s\"}", info);
	*totalsize = strlen(outbuf);
	
	return 0;
}


static int32_t update_msg(uint8_t *outbuf, struct Update_data *update_data, uint32_t *totalsize)
{
	if(NULL == outbuf || NULL == update_data || NULL == totalsize){
		dm101_debug("outbuf or update_data or totalsize is null\n");
		return -1;
	}

	sprintf(outbuf, "{\"type\":\"%s\",\"ver\":\"%s\",\"url\":\"%s\",\"hash\":\"%s\"}",
	update_data->type, update_data->ver, update_data->url, update_data->hash );

	
	return 0;
}

static int32_t force_sys_time(uint8_t *outbuf, uint32_t *totalsize)
{
	if(NULL == outbuf || NULL == totalsize){
        dm101_debug("outbuf is null\n");
	}
	    
	sprintf(outbuf, "{\"integer\":%ld}", get_utc_time());
	*totalsize = strlen(outbuf);
	
	return 0;
}


int32_t set_dat_config()
{
	//add something
}


int32_t get_dat_config()
{
	//add something
}

int32_t set_dat_group()
{
	//add something
}

int32_t get_dat_group()
{
	//add something
}


static int32_t set_dat_report_rule(uint8_t *outbuf, struct Report_rule *report_rule, uint32_t *totalsize)
{
	if(outbuf == NULL|| report_rule == NULL){
		dm101_debug("outbuf or report_rule is null\n");
		return -1;
	}

	sprintf(outbuf, "{\"enabled\":%d, \"interval\":%d}", report_rule->enabled, report_rule->interval );
	*totalsize = strlen(outbuf);
	
	return 0;
}


//ack
static int32_t ack_msg(struct Dm101pkg *dm101pkg, uint8_t *payload_buf, uint32_t payload_len)
{
	if(dm101pkg == NULL)
		return -1;

	return 0;
}


//send read package 
int32_t showoatbuf(char *tag, char *buf, int32_t len)
{
	int i;
    dm101_debug("%s oat data debug\n",tag);
	for( i = 0; i < len;i++){
		dm101_debug("%d = 0x%02x\n",i , (uint8_t)buf[i]);
	}
	return 0;
}

int64_t  send_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg)
{
    if (!context->sendbuf) return -1;
	char *sendbuf = context->sendbuf;
	dm101pkg->head.src_id = context->cfg.srdid;
    struct Head head = dm101pkg->head;
	uint32_t sendcnt = 0;

	char *auth = context->cfg.auth;
	if(context->cfg.crypt)
		dm101pkg->head.mflag |= F_CRYPT;
	
	//dm101_debug("context->auth=%s\n", context->cfg.auth);
	if(head.payload_length > 0 && dm101pkg->pload_buf != NULL){
		//添加加密算法
		if(dm101pkg->head.mflag & F_CRYPT && auth != NULL ){
			dm101pkg->head.payload_length = PKCS7_EnPadding_totalsize(dm101pkg->head.payload_length);
			//dm101_debug("get PKCS7_EnPadding_totalsize = %d\n", dm101pkg->head.payload_length);
		}
	}

	head_format_oat(*dm101pkg, sendbuf, auth);//转换为网络字节序，并格式化头
#if DEBUG_CJY_OAT
    showoatbuf("send_pkg", sendbuf, HEAD_LEN);
#endif

	/*if(context->postdata( context->postargs, (void*)sendbuf, HEAD_LEN, F_WRITE) < 0){
		dm101_debug("write head error\n");
		return -1;
	}*/
	sendcnt += HEAD_LEN;
	
	if(head.payload_length > 0 && dm101pkg->pload_buf != NULL){
		#if DEBUG_CJY
		if(dm101pkg->head.format == FORMAT_JSON){
			dm101_debug("send_pkg dm101pkg->pload_buf = %s\n", dm101pkg->pload_buf);
		}
		#endif
		//添加加密算法
		if((sendbuf[OFFSIZE_MFLAG] & F_CRYPT) && (auth != NULL) ){
			uint8_t key[16];
			make_aec_key(key, auth, sendbuf+OFFSIZE_TIME_STAMP);
			DM_AES_Encrypt_cjy(dm101pkg->pload_buf, &head.payload_length, key);
		}		
		#if DEBUG_CJY_OAT
    	showoatbuf("send_pkg", dm101pkg->pload_buf, dm101pkg->head.payload_length);		
		#endif
		
		/*if(context->postdata( context->postargs, (void*)dm101pkg->pload_buf, head.payload_length, F_WRITE) < 0){
			dm101_debug("write pload_buf error\n");
			return -1;
		}*/
		sendcnt += head.payload_length;
	    	//生成CRC32，并调节字节序
		dm101pkg->dm_crc32 = dm_crc32(0, (void*)dm101pkg->pload_buf, head.payload_length);
		change_order_dm_crc32(dm101pkg);
        memcpy(&context->sendbuf[sendcnt], (void*)&dm101pkg->dm_crc32, sizeof(dm101pkg->dm_crc32));
        sendcnt += sizeof(dm101pkg->dm_crc32);
	}
	if(context->postdata( context->postargs, (void*)context->sendbuf, sendcnt,F_WRITE) < 0){
		dm101_debug("write data error\n");		
			return -1;
	}
	//dm101_debug("send_pkg, len = %d\n", sendcnt);

	return 0;
}

int32_t read_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg)
{
	char head_buf[HEAD_LEN];
	uint32_t pldm_crc32;
	uint32_t recvcnt = 0;

	char *auth = context->cfg.auth;
	if(context->getdata( context->getargs, (void*)head_buf, HEAD_LEN , F_READ,1000) < 0){
		dm101_debug("read head error\n");
		return -1;
	}
	recvcnt += HEAD_LEN;
	
#if DEBUG_CJY_OAT
    showoatbuf("read_pkg",head_buf, HEAD_LEN);
#endif

	oat_pick_head( &dm101pkg->head, head_buf);//取出head并装换为host字节序

#if DEBUG_CJY_OAT
	show_head( &dm101pkg->head, NULL , NULL);
	//dm101_debug("read head debug:\n %s\n",showbuf);
#endif

	if(dm101pkg->head.payload_length > 0) {
		dm101pkg->pload_buf = context->recvbuf;
		if(context->getdata( context->getargs, (void*)dm101pkg->pload_buf, dm101pkg->head.payload_length,F_READ,1000) < 0){
			dm101_debug("read payload error\n");
			return -1;
		}
		recvcnt += dm101pkg->head.payload_length;
		
		#if DEBUG_CJY_OAT
    	showoatbuf("read_pkg",dm101pkg->pload_buf, dm101pkg->head.payload_length);		
		dm101_debug("before crypt crypt dm101pkg->head.payload_length= %d,DEBUG_CJY_OAT dm101pkg->pload_buf = %s\n",dm101pkg->head.payload_length,dm101pkg->pload_buf);
		#endif
		
		if(context->getdata( context->getargs, (void*)&dm101pkg->dm_crc32, sizeof(dm101pkg->dm_crc32),F_READ,1000) < 0){
			dm101_debug("read dm_crc32 error\n");
			return -1;
		}
		
		#if DEBUG_CJY_OAT
    	showoatbuf("read_pkg",(void*)&dm101pkg->dm_crc32, sizeof(dm101pkg->dm_crc32));
		#endif
		
		recvcnt += sizeof(dm101pkg->dm_crc32);
		//提取CRC32调节字节序
		pldm_crc32 = dm_crc32(0, dm101pkg->pload_buf, dm101pkg->head.payload_length);
		change_order_dm_crc32(dm101pkg);
	    if( USE_CRC == 1 && pldm_crc32 != dm101pkg->dm_crc32){
			dm101_debug("verifier dm_crc32 error,src32 = 0x%x,makedm_crc32 = 0x%x\n",dm101pkg->dm_crc32,pldm_crc32);
		    return -1;
	    }
		
		//添加解密算法
		if(dm101pkg->head.mflag &F_CRYPT && auth != NULL){
			uint8_t key[16];
			//dm101_debug("before Decrypt dm101pkg->pload_buf=%s",dm101pkg->pload_buf);
			make_aec_key(key, auth, head_buf+OFFSIZE_TIME_STAMP);
			DM_AES_Decrypt_cjy(dm101pkg->pload_buf, &dm101pkg->head.payload_length, key);	
		}
		//加上字符串截止符
		dm101pkg->pload_buf[dm101pkg->head.payload_length] = 0;
		
		#if DEBUG_CJY_OAT
    	showoatbuf("read_pkg", dm101pkg->pload_buf, dm101pkg->head.payload_length);		
		dm101_debug("after crypt dm101pkg->head.payload_length= %d,dm101pkg->pload_buf = %s\n",dm101pkg->head.payload_length,dm101pkg->pload_buf);
		#endif
		
	}

	//dm101_debug("recv size = %d\n",recvcnt);
	
	return recvcnt;
}

int32_t dm101_get_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg)
{
	return read_pkg(context, dm101pkg);
}



int32_t dm101_make_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg,
	int32_t code, void *text, uint32_t text_len)
{
	int32_t ret;
	static uint32_t sn;
	
	memset((void*)dm101pkg, 0, sizeof(struct Dm101pkg));
	dm101pkg->head.msgcode = code;
	dm101pkg->pload_buf = &context->sendbuf[HEAD_LEN];
	dm101pkg->head.sn = sn++;
	memset((void*)dm101pkg->pload_buf, 0, DM101_SENDBUF_LEN);
    switch (dm101pkg->head.msgcode){
    case CODE_SERVER_FIND:
    case CODE_HAERT:	
    case CODE_REMOTE_REBOOT:
    case CODE_REQ_TIME:
	    dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_NONE;		
		break;		
    case CODE_REPORT_INFO:
	    dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_JSON;
	    get_base_info(dm101pkg->pload_buf, DM101_SENDBUF_LEN, \
			&dm101pkg->head.payload_length);
		break;
		
    case CODE_SET_INFO:
		dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_JSON;
	    ret = set_base_info(dm101pkg->pload_buf, (uint8_t*)text, \
			&dm101pkg->head.payload_length);
		break;

    case CODE_OFFLINE:
	    dm101pkg->head.type = F_MSG_NOACK;
	    dm101pkg->head.format = FORMAT_NONE;		
		break;
    case CODE_UPDATE:
	    dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_JSON;
		ret = update_msg(dm101pkg->pload_buf, (struct Update_data*)text, &dm101pkg->head.payload_length);
		break;

    case CODE_SYS_TIME:
	    dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_JSON;
	    ret = force_sys_time(dm101pkg->pload_buf, &dm101pkg->head.payload_length);
		break;

    case CODE_SET_CONFIG:
		break;		
    case CODE_GET_CONFIG:
		break;		
    case CODE_SET_GROUP_DATA:
		break;		
    case CODE_GET_GROUP_DATA:
		break;
    case CODE_SET_REPORT_RULE:
	    dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_JSON;
		ret = set_dat_report_rule(dm101pkg->pload_buf, (struct Report_rule *)text, &dm101pkg->head.payload_length);
		break;		
    case CODE_REPORT_DATA:
	    dm101pkg->head.type = F_MSG_NEED_ACK;
	    dm101pkg->head.format = FORMAT_JSON;
	    ret = make_data_buf(dm101pkg->pload_buf, DM101_SENDBUF_LEN, \
			(const struct Data_pkg*)text, &dm101pkg->head.payload_length );
		break;		
			
    default:
		ret = -1;
    }
	
	return ret;
}

uint32_t make_ack_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg, int32_t ackcode, void *text, uint32_t text_len)
{
	if(dm101pkg->head.type != F_MSG_NEED_ACK){
		return -1;
	}

    dm101pkg->head.type = F_MSG_ACK ;
	dm101pkg->head.code = ackcode;
	dm101pkg->pload_buf = &context->sendbuf[HEAD_LEN];
	if(text !=NULL && text_len > 0){
		memcpy(dm101pkg->pload_buf, text, text_len);
	}
	switch (dm101pkg->head.msgcode){
    //client ack		
	case CODE_SET_INFO:
    case CODE_UPDATE:
    case CODE_REMOTE_REBOOT:
    case CODE_SYS_TIME:
    case CODE_SET_REPORT_RULE:
        dm101pkg->head.format = FORMAT_NONE;
		break;

	//server ack	
	case CODE_SERVER_FIND:
	case CODE_REQ_TIME:
		dm101pkg->head.format = FORMAT_JSON;
		break;
	case CODE_REPORT_INFO:
	case CODE_HAERT:
	case CODE_REPORT_DATA:
		dm101pkg->head.format = FORMAT_NONE;
		break;
    //undevelop
	case CODE_SET_CONFIG:	
    case CODE_GET_CONFIG:		
    case CODE_SET_GROUP_DATA:	
    case CODE_GET_GROUP_DATA:
	//no need ack
	case CODE_OFFLINE:
    default:
    	return -1;
        break;
    }
	
	return 0;
}


struct Dm101_context *create_dm101_context(void)
{
	struct Dm101_context *context = NULL;
	context = (struct Dm101_context *)dm101_malloc(sizeof(struct Dm101_context));
	if (context != NULL) {
		memset((void *)context, 0, sizeof(struct Dm101_context));
        context->recvbuf = dm101_malloc(DM101_RECVBUF_LEN + HEAD_LEN);
        context->sendbuf = dm101_malloc(DM101_SENDBUF_LEN + HEAD_LEN);
        if ((NULL == context->recvbuf) || (NULL == context->sendbuf)) {
			rt_kprintf("context->recvbuf malloc failed\r\n");
            goto _distory;
        }
	}else {
		rt_kprintf("context malloc failed\r\n");
	}

	return context;

_distory:
    distory_dm101_context(context);
    return NULL;
}

uint32_t distory_dm101_context(struct Dm101_context *context)
{
	dm101_free(context->recvbuf);
	dm101_free(context->sendbuf);
	dm101_free(context);
	context = NULL;
	return 0;
}

const struct dm101_cfg c_dm101_default_cfg = { 
    .magic = {MAGIC_0, MAGIC_1, MAGIC_2, MAGIC_3 }, 
    .ver = MSG_VERSION
};

struct Dm101_context *dm101_context_init(
    struct dm101_cfg *cfg,
	//void *post_get_init,
	void *(*post_get_init)(void *argv),
	void *post_get_init_args,
	void (*post_get_reset)(void *argv),
	void (*post_get_close)(void *argv),
	int (*postdata)(void *args,void *buf, uint32_t count,int8_t rw_flag),
	int (*getdata)(void *args,void *buf, uint32_t count,int8_t rw_flag, uint32_t timeout))
{
	void *ret;
	struct Dm101_context *context;

	context = create_dm101_context();
	if(context == NULL || post_get_init == NULL || postdata == NULL||\
		getdata == NULL){
		
		return NULL;
	}
	context->post_get_init = post_get_init;
	context->post_get_close = post_get_close;
	context->getdata = getdata;
	context->postdata = postdata;
	if (cfg) {
	    context->cfg = *cfg;
	} else {
	    context->cfg = c_dm101_default_cfg;
	}

	ret = context->post_get_init(post_get_init_args);
	if(ret == NULL){\
		dm101_debug("post_get_init error\n");
		return NULL;
	}
	context->getargs = ret;
	context->postargs = context->getargs;
	context->closeargs = context->getargs;
	context->resetargs = context->getargs;
    dm101_debug("dm101_context_init ok\n");
	return context;
	
}

uint32_t dm101_context_reset(struct Dm101_context *context)
{
	context->post_get_reset(context->closeargs);

	return 0;
}

uint32_t dm101_context_close(struct Dm101_context *context)
{
	context->post_get_close(context->closeargs);
	distory_dm101_context(context);
	
	return 0;
}


