
#include <board.h>

// 注意:如果有固件升级, 固件必须放在第一个数据段
// 升级包, 固定文件名
#define UPDATE_IMG_PATH         "/update.img"
#define UPDATE_SD_IMG_PATH      "/sd/update.img"

// 三个前导魔数,确保是升级包
#define IMG_MAGIC_NUM1      (0x526F2678)
#define IMG_MAGIC_NUM2      (0x1B5C4DEE)
#define IMG_MAGIC_NUM3      (0x67FAB5CD)

// 固件, 固定文件名
#define IMG_FIRMWARE_PATH   "#-firm-#-ware-#"

#pragma pack(1)

// 单个 IMG 结构
typedef struct {
    char szPath[128];       //路径
    char szDesc[256];       //描述
    rt_uint32_t ulLength;      //大小
    rt_uint32_t ulCRC32;       //文件校验(只校验文件内容pBuffer)
    //void *pBuffer;
} S_IMG_PACK;

#pragma pack()

static int create_dir( const char *path )
{
    struct stat buf;
    char dir_path[256] = {0};
    int i, len;
    
    len = strlen(path);
    
    rt_strncpy( dir_path, path, len );

    for( i=0; i<len; i++ ) {
        if (dir_path[i] == '/' && i > 0) {
            dir_path[i]='\0';
            if( stat( dir_path, &buf ) < 0) {
                mkdir( dir_path, 0755 );
            }
            dir_path[i]='/';
        }
    }

    return 0;
}

static rt_bool_t __update_check( const char *path )
{
    S_IMG_PACK xUpdateImg;
    struct stat buf;
    rt_bool_t bRet = RT_FALSE;
    int src_fd = open( path, O_RDONLY, 0);

    if( src_fd >= 0 && DFS_STATUS_OK == fstat( src_fd, &buf ) ) {
        rt_kprintf( "check update file : %s\n", path );
        // 判断最小长度
        if( buf.st_size > 12 + sizeof(S_IMG_PACK) ) {
            rt_uint32_t ulMagics[3] = {0};

            read( src_fd, ulMagics, sizeof(ulMagics) );
            //校验魔数
            if( ulMagics[0] == IMG_MAGIC_NUM1 && ulMagics[1] == IMG_MAGIC_NUM2 && ulMagics[2] == IMG_MAGIC_NUM3 ) {
                while( 1 ) {
                    // 读取首端img
                    int len = read( src_fd, &xUpdateImg, sizeof(S_IMG_PACK) );
                    // 基本校验(长度,名称)
                    if( len >= 0 && len >= sizeof(S_IMG_PACK) ) {
                        if( strcmp( IMG_FIRMWARE_PATH, xUpdateImg.szPath ) != 0 ) {
                                #define UPDATE_CHECK_BUF_SZ   (4096)
                                rt_uint8_t *block_ptr;
                                int remain = xUpdateImg.ulLength;
                                rt_uint32_t crc = 0;

                                block_ptr = rt_malloc( UPDATE_CHECK_BUF_SZ );
                                if (block_ptr == RT_NULL) {
                                    break ;
                                }

                                while( remain > 0 ) {
                                    int readlen = (remain > UPDATE_CHECK_BUF_SZ ? UPDATE_CHECK_BUF_SZ : remain);
                                    len = read( src_fd, block_ptr, readlen );
                                    if( len > 0) {
                                        crc = ulRTCrc32( crc, block_ptr, len );
                                    } else {
                                        break;
                                    }
                                    remain -= len;
                                }
                                rt_free(block_ptr);
                                if( crc != xUpdateImg.ulCRC32 ) {
                                    rt_kprintf( "update file check error : %s\n", path );
                                    goto __end;
                                }
                        } else {
                            lseek( src_fd, xUpdateImg.ulLength, SEEK_CUR );
                        }
                    } else {
                        break;
                    }
                }
            }
            bRet = RT_TRUE;
        }
    } else {
        rt_kprintf( "no update file : %s\n", path );
    }

__end:
    
    if( src_fd >= 0 ) close( src_fd );

    if( !bRet ) {
        unlink( path );
    } else {
        rt_kprintf( "update file check ok : %s\n", path );
    }

    return bRet;
}

static rt_uint32_t __file_crc( const char *path )
{
#define CRC_CHECK_BUF_SZ   (4096)
    int fd = open( path, O_RDONLY, 0666 );
    if( fd >= 0 ) {
        rt_uint8_t *buffer = rt_malloc( CRC_CHECK_BUF_SZ );
        rt_uint32_t crc = 0;
        while( 1 ) {
            int len = read( fd, buffer, CRC_CHECK_BUF_SZ );
            if( len <= 0 ) break;
            crc = ulRTCrc32( crc, buffer, len );
        }
        close(fd);
        rt_free(buffer);

        return crc;
    }

    return 0;
}

static rt_bool_t __update_do( const char *path )
{
    S_IMG_PACK xUpdateImg;
    struct stat buf;
    rt_bool_t bRet = RT_FALSE;
    int src_fd = open( path, O_RDONLY, 0);
    if( src_fd >= 0 && DFS_STATUS_OK == fstat( src_fd, &buf ) ) {
        // 判断最小长度
        if( buf.st_size > 12 + sizeof(S_IMG_PACK) ) {
            rt_uint32_t ulMagics[3] = {0};

            read( src_fd, ulMagics, sizeof(ulMagics) );
            //校验魔数
            if( ulMagics[0] == IMG_MAGIC_NUM1 && ulMagics[1] == IMG_MAGIC_NUM2 && ulMagics[2] == IMG_MAGIC_NUM3 ) {
            
                while( 1 ) {
                    // 读取首端img
                    int len = read( src_fd, &xUpdateImg, sizeof(S_IMG_PACK) );
                    // 基本校验(长度,名称)
                    if( len >= 0 && len >= sizeof(S_IMG_PACK) ) {
                        if( strcmp( IMG_FIRMWARE_PATH, xUpdateImg.szPath ) != 0 ) {
                                if( __file_crc( xUpdateImg.szPath ) == xUpdateImg.ulCRC32 ) {
                                    rt_kprintf( "skip file : %s\n", xUpdateImg.szPath );
                                    lseek( src_fd, xUpdateImg.ulLength, SEEK_CUR );
                                    WDOG_FEED();
                                } else {
                                    rt_kprintf( "update file : %s\n", xUpdateImg.szPath );
                                    WDOG_FEED();
                                    rt_uint8_t *block_ptr;
                                    int remain = xUpdateImg.ulLength;
                                    #define UPDATE_UPDATE_BUF_SZ   (512)

                                    block_ptr = rt_malloc( UPDATE_UPDATE_BUF_SZ );
                                    if (block_ptr == RT_NULL) {
                                        break ;
                                    }

                                    create_dir( xUpdateImg.szPath );
                                    
                                    int dst_fd = open( xUpdateImg.szPath, O_CREAT | O_TRUNC | O_RDWR, 0666 );
                                    
                                    if( dst_fd < 0 ) {
                                        rt_free(block_ptr);
                                        break ;
                                    }

                                    while( remain > 0 ) {
                                        int readlen = (remain > UPDATE_UPDATE_BUF_SZ ? UPDATE_UPDATE_BUF_SZ : remain);
                                        len = read( src_fd, block_ptr, readlen );
                                        if( len > 0) {
                                            write( dst_fd, block_ptr, len );
                                        } else {
                                            break;
                                        }
                                        remain -= len;
                                    };
                                    close( dst_fd );
                                    rt_free(block_ptr);
                                }
                        } else {
                            lseek( src_fd, xUpdateImg.ulLength, SEEK_CUR );
                        }
                    } else {
                        break;
                    }
                }
            }
            bRet = RT_TRUE;
        }
    }
    if( src_fd >= 0 ) close( src_fd );
    unlink( path );
    return bRet;
}
    
rt_bool_t bDoUpdate( void )
{
    return ( (__update_check( UPDATE_IMG_PATH ) && __update_do( UPDATE_IMG_PATH )) || 
             (__update_check( UPDATE_SD_IMG_PATH ) && __update_do( UPDATE_SD_IMG_PATH )) );
}

void _hclient_download_handle(hclient_download_e status, int total, int pos)
{
    switch (status) {
    case HCLIENT_DOWN_READY:
        rt_kprintf("开始下载\n");
        break;
    case HCLIENT_DOWN_ING:
        rt_kprintf("正在下载:%d/%d\n", total, pos);
        break;
    case HCLIENT_DOWN_SUCCESS:
        rt_kprintf("下载完成:%d/%d\n", total, pos);
        break;
    case HCLIENT_DOWN_FAIL:
        rt_kprintf("下载失败\n");
        break;
    }
}

void update_hclient_download(const char *url)
{
    hclient_session_t *session = hclient_create(8*1024);
    if (session) {
        hclient_err_e err = hclient_download(session, url, "/update.img", 5000, _hclient_download_handle);
        if (HCLIENT_ERR_OK == err) {
            rt_kprintf("download img ok! reset system!\n");
            vDoSystemReset();
        } else {
            rt_kprintf("download img fail.!\n");
        }
        hclient_destroy(session);
    }
}

void _check_version(const char *data)
{
    cJSON *json = cJSON_Parse(data);
    if (json) {
        int status = cJSON_GetInt(json, "status", 0);
        rt_kprintf("update_hclient_query status:%d, message:'%s'\n", status, cJSON_GetString(json, "message", ""));
        if (1 == status) {
            cJSON *json_data = cJSON_GetObjectItem(json, "data");
            if (json_data) {
                int version_type = cJSON_GetInt(json_data, "version_type", -1);
                const char *version_title = cJSON_GetString(json_data, "version_title", NULL);
                double version_no = cJSON_GetDouble(json_data, "version_no", -1);
                const char *file_path = cJSON_GetString(json_data, "file_path", NULL);
                rt_kprintf("update check->type:%d,title:%s,no:%d,path='%s'\n", version_type, _STR(version_title), (int)(version_no*100), _STR(file_path));
                if(version_type >= 0 && version_no > 0) {
                    int ver = (int)(version_no*100);
                    if (file_path && file_path[0] && ver != SW_VER_VERCODE) update_hclient_download(file_path);
                }
            }
        }
        cJSON_Delete(json);
    }
}

rt_bool_t update_hclient_query(void)
{
    rt_bool_t ret = RT_FALSE;
    hclient_session_t *session = hclient_create(512);
    if (session) {
        cJSON *json = cJSON_CreateObject();
        if (json) {
            char strbuf[33] = {0};
            sprintf(strbuf, "%d.%02d", SW_VER_MAJOR, SW_VER_MINOR);
            cJSON_AddStringToObject(json, "product_model", PRODUCT_MODEL);
            cJSON_AddStringToObject(json, "software_no", strbuf);
            sprintf(strbuf, "%d.%02d", HW_VER_VERCODE/100, HW_VER_VERCODE%100);
            cJSON_AddStringToObject(json, "device_no", strbuf);
            memset(strbuf, 0, sizeof(strbuf));
            memcpy(strbuf, g_xDevInfoReg.xDevInfo.xSN.szSN, sizeof(DevSN_t));
            cJSON_AddStringToObject(json, "sn", strbuf);
            memset(strbuf, 0, sizeof(strbuf));
            hclient_get_sign(json, strbuf, "software_no", "device_no", NULL);
            cJSON_AddStringToObject(json, "sign", strbuf);
            {
                char *szJson = cJSON_PrintUnformatted(json);
                if (szJson) {
                    hclient_err_e err = hclient_post(session, HTTP_SERVER_HOST"/get_version.action", szJson, 5000);
                    rt_free(szJson);
                    if (HCLIENT_ERR_OK == err) {
                        ret = RT_TRUE;
                        if (session->rsp.data && session->rsp.data[0]) {
                            //elog_v("http_update", session->rsp.data);
                            _check_version(session->rsp.data);
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

