
#include <board.h>
#include "rtu_md5.h"
#include "rtu_sha.h"
#include "rtu_des.h"
#include "rtu_reg.h"

#include "nv.h"

#define CFG_MAGIC               0xD6BCF4ED
#define REG_ID_LEN              (40)

#define REG_DEF_RANDOMCODE      (0x54DCF6A0)

//#define REG_TEST_OVER_TIME      (7*24*60*60)    //单位:秒

static const reg_t c_reg_default = { 0, 0 };

reg_t g_reg;
reg_info_t g_reg_info;
rt_uint8_t g_regid[REG_ID_LEN] = {0};
rt_bool_t g_isreg = RT_FALSE;
rt_bool_t g_istestover_poweron = RT_FALSE;

static int __sys_get_uuid(rt_uint32_t uuid[4])
{
#define DM_READ_ID_CTL      _IO('R',100)
#define DEV_NAME            "/dev/dm_wdg"  
    int fd = -1;
    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        printf("Cannot open %s!\n", DEV_NAME);
        return -1;
    }

    uint64_t id = 0;
    if (ioctl(fd, DM_READ_ID_CTL, &id)<0){	
        printf("---ioctl set fail\n");
        close(fd);
        return -1;
    }
    close(fd);

    int i= 0;
    for(i = 0 ; i < 4; i++) {
        id += i;
        uuid[i] =  ulRTCrc32(0, (void *)&id, 8);
    }
    return 0;
}

static void prvSetRegDefault( void )
{
    g_reg = c_reg_default;
}

static void prvSaveRegCfgToFs( void );

static void prvReadRegFromFs( void )
{
    uint32_t magic = 0;
    uint16_t usCheck = 0;
    uint8_t buffer[256] = {0};
    int _flag = 1;

    block_read(NV_DEV_FILE, KEY_INFO_OFFSET, buffer, sizeof(buffer));

    memcpy(&magic, (void const *)buffer, 4 );
    if (CFG_MAGIC == magic) {
        memcpy(&g_reg, (void const *)&buffer[4], sizeof(g_reg) );
        memcpy(&usCheck, (void const *)&buffer[4 + sizeof(g_reg)], 2 );
        if (das_crc16((uint8_t *)&g_reg, sizeof(g_reg)) != usCheck) {
            _flag = 0;
        }
    } else {
        _flag = 0;
    }

    if (0 == _flag) {
        if (board_cfg_read( REG_CFG_NAME, &g_reg, sizeof(g_reg))) {
            prvSaveRegCfgToFs();
        } else {
            prvSetRegDefault();
        }
    }
}

static void prvSaveRegCfgToFs( void )
{
    uint8_t buffer[256] = {0};
    uint16_t usCheck = 0;
    uint32_t magic = CFG_MAGIC;

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &magic, 4);
    memcpy(&buffer[4], &g_reg, sizeof(g_reg));
    usCheck = das_crc16((uint8_t *)&g_reg, sizeof(g_reg));
    memcpy(&buffer[4 + sizeof(g_reg)], &usCheck, 2);
    
    block_erase(KEY_INFO_OFFSET);
    block_write(NV_DEV_FILE, KEY_INFO_OFFSET, buffer, sizeof(buffer));
}

static void prvSetRegInfoDefault( void )
{
    g_reg_info.test_time = 0;
    g_reg_info.code = REG_DEF_RANDOMCODE;       //出厂固定随机码
}

static void prvReadRegInfoFromRom( void )
{
    uint32_t magic = 0;
    uint16_t usCheck = 0;
    uint8_t buffer[256] = {0};

    block_read(NV_DEV_FILE, REG_INFO_OFFSET, buffer, sizeof(buffer));

    memcpy(&magic, (void const *)buffer, 4 );
    if (CFG_MAGIC == magic) {
        memcpy(&g_reg_info, (void const *)&buffer[4], sizeof(g_reg_info) );
        memcpy(&usCheck, (void const *)&buffer[4 + sizeof(g_reg_info)], 2 );
        if (das_crc16((uint8_t *)&g_reg_info, sizeof(g_reg_info)) != usCheck) {
            prvSetRegInfoDefault();
        }
    } else {
        prvSetRegInfoDefault();
    }
    g_reg_info.code = REG_DEF_RANDOMCODE;
}

static void prvSaveRegInfoToRom( void )
{
    uint8_t buffer[256] = {0};
    uint16_t usCheck = 0;
    uint32_t magic = CFG_MAGIC;

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &magic, 4);
    memcpy(&buffer[4], &g_reg_info, sizeof(g_reg_info));
    usCheck = das_crc16((uint8_t *)&g_reg_info, sizeof(g_reg_info));
    memcpy(&buffer[4 + sizeof(g_reg_info)], &usCheck, 2);
    
    block_erase(REG_INFO_OFFSET);
    block_write(NV_DEV_FILE, REG_INFO_OFFSET, buffer, sizeof(buffer));
}

void reg_init( void )
{
    prvReadRegInfoFromRom();
    prvReadRegFromFs();
    reg_regetid( g_reg_info.code, g_regid );
    g_isreg = reg_check(g_reg.key);
    g_istestover_poweron = (!g_isreg && reg_testover());
}

int reg_regetid( uint32_t code, rt_uint8_t id[40] )
{
    rt_uint32_t uuid[4] = { 0 };
    __sys_get_uuid(uuid);
    char *phwid = (char *)uuid;
    char key1[] = { 0x5F, 0x2D, 0x25, 0x34, 0xAD, 0x58, 0xEB, 0xA0 };
    char key2[] = { 0x3F, 0x57, 0x35, 0xCC, 0x08, 0x58, 0x1B, 0x99 };
    char key3[] = "_k)-%l&h";
    char buf[41], des[40];
    sprintf(buf, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", phwid[0],phwid[1],phwid[2],phwid[3],phwid[4],phwid[5],
        phwid[6],phwid[7],phwid[8],phwid[9],phwid[10],phwid[11],phwid[12],phwid[13],phwid[14], phwid[15]);
    rt_kprintf("hwid = %s, code = %u\n", buf, code);
    sprintf( buf, "%02X%08X%02X%08X%02X%08X%02X%08X", 
        code&0xFF,~uuid[0],(code>>8)&0xFF,
        ~uuid[1], (code>>16)&0xFF,~uuid[2],
        (code>>24)&0xFF,~uuid[3]
    );
    
    rtu_des_encrypt(des, (unsigned char *)buf, strlen(buf), key1);
    rtu_des_decrypt(des, (unsigned char *)des, strlen(buf), key2);
    rtu_des_encrypt(id, (unsigned char *)des, strlen(buf), key3);

    return strlen(buf);
}

// 注意:该函数需要占用256以上字节栈空间, 特别注意调用
rt_bool_t reg_check( const char *key )
{
    if( key[0] ) {
        RTU_MD5_CTX md5;
        RTU_SHA1_CONTEXT sha;
        char mykey[40+1] = {0}, mykey1[80+1];
        int len = REG_ID_LEN;
        memcpy( mykey, g_regid, REG_ID_LEN );
        for( int i = 0; i < 40; i++ ) {
            if( i < 39 && g_regid[i] < 0x3F ) {
                char deskey[] = { 
                    g_regid[i]+~0x5F, ~g_regid[i]+0x2D, ~g_regid[i]+0x25, g_regid[i]+~0x34, 
                    g_regid[i]-~0xAD, g_regid[i]-0x58, ~g_regid[i]+~0xEB, ~g_regid[i]-~0xA0 
                };
                rtu_des_encrypt( mykey, (unsigned char *)mykey, REG_ID_LEN, deskey );
            } else if( i < 39 && g_regid[i] < 0x7F ) {
                rtu_MD5Init( &md5 );
                rtu_MD5Update( &md5, (unsigned char *)mykey, REG_ID_LEN );
                rtu_MD5Final( &md5 );
                mykey[0] = '\0';
                sprintf( mykey, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", 
                    md5.digest[0], ~md5.digest[1]&0xFF, ~md5.digest[8]&0xFF, md5.digest[9], 
                    md5.digest[2], ~md5.digest[3]&0xFF, md5.digest[10], md5.digest[11], 
                    md5.digest[4], ~md5.digest[5]&0xFF, md5.digest[12], md5.digest[13], 
                    md5.digest[6], md5.digest[7], md5.digest[14], md5.digest[15], 
                    md5.digest[8], md5.digest[3], md5.digest[5], md5.digest[1]);
            } else {
                rtu_sha1_init( &sha );
                rtu_sha1_write( &sha, (unsigned char *)mykey, REG_ID_LEN );
                rtu_sha1_final( &sha );
                mykey[0] = '\0';
                sprintf( mykey, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", 
                    sha.buf[0], ~sha.buf[10]&0xFF, sha.buf[4], sha.buf[15], 
                    sha.buf[3], sha.buf[7], ~sha.buf[8]&0xFF, sha.buf[16], 
                    ~sha.buf[6]&0xFF, sha.buf[5], sha.buf[11], ~sha.buf[17]&0xFF, 
                    sha.buf[9], sha.buf[2], ~sha.buf[13]&0xFF, sha.buf[18], 
                    sha.buf[19], ~sha.buf[1]&0xFF, sha.buf[14], sha.buf[12]);
            }
        }

        {
            char deskey[] = "s;*:.~<1";
            rtu_des_encrypt( mykey, (unsigned char *)mykey, REG_ID_LEN, deskey );

            mykey1[0] = '\0';
            for( int i = 0; i < 40; i++ ) {
                char tmp[3] = {0};
                sprintf(tmp,"%X", (uint8_t)mykey[i] & 0xFF);
                strcat(mykey1,tmp );
            }
        }

        return ( 0 == strcmp( mykey1, key ) );
    } 
    return RT_FALSE;
}

// 注意:该函数需要占用256以上字节栈空间, 特别注意调用
rt_bool_t reg_reg( const char *key )
{
    if( (g_isreg = reg_check(key)) ) {
        strncpy(g_reg.key, key, sizeof(g_reg.key));
        g_reg.reg_time = time(0);
    } else {
        prvSetRegDefault();
    }
    prvSaveRegCfgToFs();

    return g_isreg;
}

rt_bool_t reg_testover( void )
{
    return g_reg_info.test_time >= REG_TEST_OVER_TIME;

}

void reg_testdo( rt_time_t sec )
{
    if( !reg_testover() ) {
        g_reg_info.test_time += sec;
        prvSaveRegInfoToRom();
    }
}

rt_bool_t _check_reg(const char *data)
{
    rt_bool_t ret = RT_FALSE;
    cJSON *json = cJSON_Parse(data);
    if (json) {
        int status = cJSON_GetInt(json, "status", 0);
        rt_kprintf("reg_hclient_query status:%d, message:'%s'\n", status, cJSON_GetString(json, "message", ""));
        if (1 == status) {
            cJSON *json_data = cJSON_GetObjectItem(json, "data");
            if (json_data) {
                const char *sn = cJSON_GetString(json_data, "sn", NULL);
                const char *key = cJSON_GetString(json_data, "key", NULL);
                const char *randomcode = cJSON_GetString(json_data, "randomcode", RT_NULL);
                rt_kprintf("reg check->sn:%s,key:%s,randomcode:%s\n", _STR(sn), _STR(key), _STR(randomcode));
                if (sn && key && randomcode && strlen(randomcode) > 0) {
                    rt_uint32_t code = (rt_uint32_t)atol(randomcode);
                    if (code != 0 && code != g_reg_info.code) {
                        g_reg_info.code = REG_DEF_RANDOMCODE;
                        prvSaveRegInfoToRom();
                    }
                    ret = reg_reg(key);
                }
            }
        } else if (0 == status) {
            g_reg_info.code = REG_DEF_RANDOMCODE;
            prvSaveRegInfoToRom();
            ret = reg_reg("");
        }
        cJSON_Delete(json);
    }
    
    return ret;
}

// RT_TRUE 表明与服务器通信成功, 否则表示通信异常
rt_bool_t reg_hclient_query(void)
{
    rt_bool_t ret = RT_FALSE;
    hclient_session_t *session = hclient_create(512);
    if (session) {
        cJSON *json = cJSON_CreateObject();
        if (json) {
            char strbuf[128] = {0};
            cJSON_AddStringToObject(json, "product_model", PRODUCT_MODEL);
            memset(strbuf, 0, sizeof(strbuf));
            cJSON_AddStringToObject(json, "sn", g_sys_info.SN);
            rt_sprintf(strbuf, "%u", (uint32_t)g_reg_info.code);
            cJSON_AddStringToObject(json, "randomcode", strbuf);
            hclient_get_sign(json, strbuf, "sn", "randomcode", NULL);
            cJSON_AddStringToObject(json, "sign", strbuf);
            {
                char *szJson = cJSON_PrintUnformatted(json);
                if (szJson) {
                    hclient_err_e err = hclient_post(session, HTTP_SERVER_HOST"/cd_key.action", szJson, 5000);
                    rt_free(szJson);
                    if (HCLIENT_ERR_OK == err) {
                        if (session->rsp.data && session->rsp.data[0]) {
                            ret = RT_TRUE;
                            //elog_v("http_reg", session->rsp.data);
                            g_isreg = _check_reg(session->rsp.data);
                        }
                    }
                }
            }
            cJSON_Delete(json);
        }
        hclient_destroy(session);
    }
    return ret;
}

DEF_CGI_HANDLER(reggetid)
{
    rt_err_t err = RT_EOK;
    char* szRetJSON = RT_NULL;
    char id[128] = {0};
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        
        cJSON_AddNumberToObject( pItem, "ret", RT_EOK );

        for( int i = 0; i < 40; i++ ) {
            char tmp[3] = {0};
            sprintf( tmp, "%02X", g_regid[i] );
            strcat(id,tmp);
        }
        cJSON_AddStringToObject( pItem, "id", id );
        cJSON_AddNumberToObject( pItem, "reg", g_isreg );
        if( g_isreg ) {
            cJSON_AddStringToObject( pItem, "key", g_reg.key );
        } else {
            cJSON_AddNumberToObject( pItem, "tta", REG_TEST_OVER_TIME );
            cJSON_AddNumberToObject( pItem, "ttt", g_reg_info.test_time );
        }
        
        szRetJSON = cJSON_PrintUnformatted( pItem );
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

	WEBS_DONE(200);
}

DEF_CGI_HANDLER(doreg)
{
    rt_err_t err = RT_EOK;
    const char *szKey = CGI_GET_ARG("key"); 

    if( szKey ) {
        if( reg_reg(szKey) ) {
            rt_kprintf( "doreg ok!\n" );
        } else {
            err = RT_ERROR;
        }
    }

    WEBS_PRINTF( "{\"ret\":%d}", err);
	WEBS_DONE(200);
}

