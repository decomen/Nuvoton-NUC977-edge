
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#define HCLIENT_HTTP_PRE                "http://"
#define HCLIENT_HTTP_PRE_LEN            7
#define HCLIENT_HOST_LENGTH             (128)
#define HCLIENT_PORT                    (80)

typedef enum {
    HCLIENT_ERR_OK      = 0,
    HCLIENT_ERR_IO      = -1,
    HCLIENT_ERR_TIMEOUT = -2,
    HCLIENT_ERR_MEM     = -3,
    HCLIENT_ERR_SND     = -4,
    HCLIENT_ERR_RCV     = -5,
    HCLIENT_ERR_FILE    = -6,
} hclient_err_e;

typedef struct {
    int                 length;
    int                 code;
    char                *data;  //下载文件时不使用该data, 该data指向session中的buffer
} hclient_rsp_t;

typedef struct {
    struct sockaddr_in  hostaddr;
	char                host[HCLIENT_HOST_LENGTH];
    int                 socket;
    int                 timeout;

    hclient_rsp_t       rsp;
    
	rt_uint16_t         buffer_offset;
	rt_uint16_t         buffer_length;
	rt_uint8_t          buffer[1];
} hclient_session_t;

typedef enum {
    HCLIENT_DOWN_READY      = 0,
    HCLIENT_DOWN_ING        = 1,
    HCLIENT_DOWN_SUCCESS    = 2,
    HCLIENT_DOWN_FAIL       = 3,
} hclient_download_e;

typedef void ( *hclient_download_handle )(hclient_download_e status, int total, int pos);

void hclient_init(void);
hclient_session_t *hclient_create(int buffer_length);
void hclient_destroy(hclient_session_t *session);
hclient_err_e hclient_post(hclient_session_t *session, const char *url, const char *data, int timeout);
hclient_err_e hclient_download(hclient_session_t *session, const char *url, const char *path, int timeout, hclient_download_handle handle);
hclient_err_e hclient_download_data(hclient_session_t *session, const char *path, hclient_download_handle handle);
hclient_err_e hclient_wait_few_data_rsp(hclient_session_t *session);
hclient_err_e hclient_connect_host(hclient_session_t *session, const char *url);
void hclient_rsp_parse(hclient_session_t *session, char *buffer, int length);
void hclient_session_close(hclient_session_t *session);
int hclient_session_read(hclient_session_t *session, void *buffer, int length);
int hclient_session_printf(hclient_session_t *session, const char *fmt, ...);
int hclient_session_write(hclient_session_t *session, const void *data, size_t size);
void hclient_get_sign(cJSON *json, char *md5, ...);

#endif

