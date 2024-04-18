
#include "board.h"
#include "httpclient.h"
#include <arpa/inet.h>

static void __hclient_set_header(hclient_session_t *session, const char *url, int type, const char *accept, int keep_alive, int content_length);

//#define hclient_dbg(_fmt,...)   rt_kprintf( "[hclient]" _fmt, ##__VA_ARGS__ )
#define hclient_dbg(_fmt,...)

#define _str_begin_with(s, t)   (strncasecmp(s, t, strlen(t)) == 0)

void hclient_init(void)
{

}

hclient_session_t *hclient_create(int buffer_length)
{
    hclient_session_t *session = rt_calloc(1, sizeof(hclient_session_t)+buffer_length);
    if(session) {
        session->buffer_length = buffer_length;
        session->socket = -1;
    }
    return session;
}

void hclient_destroy(hclient_session_t *session)
{
    if (session) {
        hclient_session_close(session);
        rt_free(session);
    }
}

hclient_err_e hclient_post(hclient_session_t *session, const char *url, const char *data, int timeout)
{
    hclient_err_e err = HCLIENT_ERR_MEM;
    if (session) {
        int content_length = strlen(data);
        session->timeout = timeout;
        err = hclient_connect_host(session, url);
        if (HCLIENT_ERR_OK == err) {
            __hclient_set_header(session, url, 1, "application/json", 0, content_length);
            //rt_kprintf("%s\n", data);
            if (content_length <= 0 || hclient_session_write(session, data, content_length) > 0) {
                err = hclient_wait_few_data_rsp(session);
            } else {
                err = HCLIENT_ERR_SND;
            }
        }
    }
    return err;
}

hclient_err_e hclient_download(hclient_session_t *session, const char *url, const char *path, int timeout, hclient_download_handle handle)
{
    hclient_err_e err = HCLIENT_ERR_MEM;
    if (session) {
        session->timeout = timeout;
        err = hclient_connect_host(session, url);
        if (HCLIENT_ERR_OK == err) {
            __hclient_set_header(session, url, 0, "*/*", 1, 0);
            err = hclient_download_data(session, path, handle);
        }
    }
    return err;
}

// downlaod
hclient_err_e hclient_download_data(hclient_session_t *session, const char *path, hclient_download_handle handle)
{
    fd_set readset;
    hclient_err_e err = HCLIENT_ERR_RCV;
    char *ptr = (char *)session->buffer;
    int read_len = 0;

    session->buffer_offset = 0;
    if (handle) handle(HCLIENT_DOWN_READY, 0, 0);
    read_len = hclient_session_read(session, ptr, session->buffer_length - session->buffer_offset);
    if( read_len <= 0 ) {
        err = HCLIENT_ERR_IO;
        hclient_dbg( "download first data -> close with read %d.\n", read_len );
        if (handle) handle(HCLIENT_DOWN_FAIL, 0, 0);
        return HCLIENT_ERR_RCV;
    } else {
        ptr[read_len] = '\0';
        ptr += read_len;
        session->buffer_offset += read_len;
    }
    hclient_rsp_parse(session, (char *)session->buffer, session->buffer_offset);
    if (200 == session->rsp.code) {
        int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666);
        int remain = session->rsp.length;
        if (fd < 0) {
            return HCLIENT_ERR_FILE;
        }
        if (session->buffer_offset > 0 && session->rsp.data) {
            int first_len = session->buffer_offset - ((char *)session->rsp.data - (char *)session->buffer);
            write(fd, session->rsp.data, first_len);
            remain -= first_len;
            if (handle) handle(HCLIENT_DOWN_ING, session->rsp.length, session->rsp.length-remain);
        }
        while (remain > 0) {
            rt_thddog_feed("hclient_session_read");
            read_len = hclient_session_read(session, (char *)session->buffer, session->buffer_length);
            if (read_len > 0) {
                write(fd, session->buffer, remain>read_len?read_len:remain);
                remain -= (remain>read_len?read_len:remain);
                if (handle) handle(HCLIENT_DOWN_ING, session->rsp.length, session->rsp.length-remain);
            } else {
                break;
            }
        }

        close(fd);

        // 上传被中断, 删除文件
        if (remain > 0) {
            err = HCLIENT_ERR_FILE;
            if (handle) handle(HCLIENT_DOWN_FAIL, 0, 0);
            unlink(path);
        } else {
            err = HCLIENT_ERR_OK;
            if (handle) handle(HCLIENT_DOWN_SUCCESS, session->rsp.length, session->rsp.length);
        }
    }
    
    return err;
}


// few data rsp
hclient_err_e hclient_wait_few_data_rsp(hclient_session_t *session)
{
    hclient_err_e err = HCLIENT_ERR_RCV;
    char *ptr = (char *)session->buffer;

    session->buffer_offset = 0;
    while (session->buffer_offset < session->buffer_length) {
        int read_len = hclient_session_read(session, ptr, session->buffer_length - session->buffer_offset);
        if (0 == read_len) {
            hclient_dbg("read close.\n");
            err = HCLIENT_ERR_OK;
            break;
        } else if (read_len == -1) {
            hclient_dbg("read err.\n");
            err = HCLIENT_ERR_IO;
            break;
        } else if (read_len == -2) {
            hclient_dbg("read timeout.\n");
            err = HCLIENT_ERR_TIMEOUT;;
            break;
        } else {
            ptr[read_len] = '\0';
            ptr += read_len;
            session->buffer_offset += read_len;
        }
    }

    if (HCLIENT_ERR_OK == err || (HCLIENT_ERR_TIMEOUT == err && session->buffer_offset > 0)) {
        hclient_rsp_parse(session, (char *)session->buffer, session->buffer_offset);
        if (200 == session->rsp.code) {
            err = HCLIENT_ERR_OK;
        }
    }
    
    return err;
}

static char _s_host_cache[HCLIENT_HOST_LENGTH];
static struct sockaddr_in _s_peer_cache;

hclient_err_e hclient_connect_host(hclient_session_t *session, const char *url)
{
    fd_set readset, writeset;
    int host_start = 0;
    int host_end = strlen(url);
    struct sockaddr_in peer;
    hclient_err_e err = HCLIENT_ERR_IO;
    struct timeval timeout;
    int select_ret = 0;
    char *ptr = NULL;
    unsigned short port = 80;
    
    timeout.tv_sec = session->timeout/1000;
    timeout.tv_usec = (session->timeout-(timeout.tv_sec*1000))*1000;

    if(_s_host_cache[0] && _str_begin_with(url, _s_host_cache)) {
        peer = _s_peer_cache;
    } else {
        if (_str_begin_with(url, HCLIENT_HTTP_PRE)) {
            host_start = HCLIENT_HTTP_PRE_LEN;
            host_end = host_start;
            while (url[host_end] && url[host_end] != '/') host_end++;
        }
        for (int i = host_start; i < host_end; i++) {
            session->host[i-host_start] = url[i];
        }
        session->host[host_end-host_start] = '\0';
        ptr = strchr(session->host, ':');
        if (ptr) {
            *ptr++ = '\0';
            port = atoi(ptr);
        }
        
        if (!inet_aton(session->host, (struct in_addr *)&peer.sin_addr)) {
            struct hostent* host = lwip_gethostbyname(session->host);
            if (host && host->h_addr_list && host->h_addr) {
                memcpy(&peer.sin_addr, host->h_addr, sizeof(struct in_addr));
            } else {
                hclient_dbg("gethostbyname fail.\n");
                goto __END;
            }
        }
        
        strcpy(_s_host_cache, session->host);
        _s_peer_cache = peer;
        if (ptr) {
            ptr[-1] = ':';
        }
    }

    session->socket = lwip_socket( AF_INET, SOCK_STREAM, 0 );
    if (session->socket < 0) {
        hclient_dbg("connect socket error:%d !\n", lwip_get_error(session->socket));
        goto __END;
    }
	timeout.tv_sec = 4; timeout.tv_usec = 0;
	if (setsockopt(session->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        hclient_dbg("set SO_SNDTIMEO fail.\n");
        goto __END;
    }
    hclient_dbg("start connect\n" );
    peer.sin_family      = AF_INET;
    peer.sin_port        = htons(port);

	rt_thddog_feed("lwip_connect");
    if (lwip_connect(session->socket, (struct sockaddr *)&peer, sizeof(struct sockaddr)) >= 0) {
        hclient_dbg("new conn !\n");
    } else {
        if (EINPROGRESS == lwip_get_error(session->socket)) {
            hclient_dbg("connect timeout!\n");
        } else {
        	hclient_dbg("connect error:%d !\n", lwip_get_error(session->socket));
        }
        goto __END;
    }

	if (lwip_fcntl(session->socket, F_SETFL, O_NONBLOCK) < 0) {
        hclient_dbg("set O_NONBLOCK fail.\n");
        goto __END;
    }
	hclient_dbg("connect ok.\n");
    err = HCLIENT_ERR_OK;

__END:
    hclient_dbg("end!\n");
    return err;
}

void hclient_rsp_parse(hclient_session_t *session, char *buffer, int length)
{
    char *ch;
    char *rsp_buffer;
    char *rsp_length;
    char *rsp_code;

    rsp_code = RT_NULL;
    rsp_length = RT_NULL;
    rsp_buffer = buffer;

    /* web request begin with method */
    if (_str_begin_with(rsp_buffer, "HTTP/1")) {
        ch = strchr(rsp_buffer, ' ');
        if (ch == RT_NULL) {
            /* Bad Response */
            session->rsp.code = 400;
            return;
        }
        *ch++ = '\0';
        rsp_buffer = ch;
        rsp_code = rsp_buffer;
        ch = strchr(rsp_buffer, ' ');
        *ch++ = '\0';
        rsp_buffer = ch;
        session->rsp.code = atoi(rsp_code);
    } else {
        /* Not implemented, does not support HTTP 0.9 protocol */
        session->rsp.code = 400;
        return;
    }

    if (session->rsp.code != 200) {
        return;
    }
    
    ch = strstr(rsp_buffer, "\r\n");
    if (ch == RT_NULL) {
        /* Bad Response */
        session->rsp.code = 400;
        return ;
    }
    *ch++ = '\0'; *ch++ = '\0';
    rsp_buffer = ch;

    for (;;) {
        if (_str_begin_with(rsp_buffer, "\r\n")) {
            if (rsp_length != RT_NULL) {
                rsp_buffer += 2;
                session->rsp.length = atoi(rsp_length);
                session->rsp.data = rsp_buffer;
            }
            return;
        }

        if (*rsp_buffer == '\0') {
            return;
        }

        if (_str_begin_with(rsp_buffer, "Content-Length:")) {
            /* get content length */
            rsp_buffer += 15;
            while (*rsp_buffer == ' ') rsp_buffer++;
            rsp_length = rsp_buffer;
        } else if (_str_begin_with(rsp_buffer, "Content-Type:")) {
            /* get content type */
            rsp_buffer += 13;
            while (*rsp_buffer == ' ') rsp_buffer++;
            /*if (str_begin_with(rsp_buffer, "multipart/form-data")) {
                if ((ch = strstr(rsp_buffer, "boundary="))) {
                    request->multiPartBoundary = (ch + strlen("boundary="));
                }
            } else {
                request->multiPartBoundary = RT_NULL;
            }*/
        }
        
        ch = strstr(rsp_buffer, "\r\n");
        if (ch == RT_NULL) {
            /* Bad Response */
            session->rsp.code = 400;
            return;
        }
        /* terminal field */
        *ch++ = '\0'; *ch++ = '\0';
        rsp_buffer = ch;
    }
}

void hclient_session_close(hclient_session_t *session)
{
    if (session && session->socket >= 0) {
        lwip_shutdown(session->socket, SHUT_RDWR);
        lwip_close(session->socket);
        session->socket = -1;
    }
}

// -2 timeout
// -1 read err
// 0 close
int hclient_session_read(hclient_session_t *session, void *buffer, int length)
{
    fd_set readset;
    struct timeval timeout;
    int read_count;
    int select_ret = 0;
    
    timeout.tv_sec = session->timeout/1000;
    timeout.tv_usec = (session->timeout-(timeout.tv_sec*1000))*1000;
    FD_ZERO(&readset);
    FD_SET(session->socket, &readset);

    select_ret = lwip_select(session->socket+1, &readset, 0, 0, &timeout);
    if (0 == select_ret) {
        return -2;
    } else if (select_ret < 0) {
        return -1;
    }
    
    if (FD_ISSET(session->socket, &readset)) {
        read_count = lwip_read(session->socket, buffer, length);
        if (read_count == 0) {
            return 0;
        } else if (read_count < 0) {
            return -1;
        }
        return read_count;
    }

    return -1;
}

int hclient_session_printf(hclient_session_t *session, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    rt_memset(session->buffer, 0, sizeof(session->buffer));
    rt_vsprintf((char *)(session->buffer), fmt, args);
    va_end(args);

    return lwip_send(session->socket, session->buffer, rt_strlen((char *)(session->buffer)), MSG_NOSIGNAL);
}

int hclient_session_write(hclient_session_t *session, const void *data, size_t size)
{
    return lwip_send(session->socket, data, size, MSG_NOSIGNAL);
}

// type:  0,get  1,post
static void __hclient_set_header(hclient_session_t *session, const char *url, int type, const char *accept, int keep_alive, int content_length)
{
    session->buffer_offset = 0;
    char *ptr, *end_buffer;
    int offset;

    ptr = (char *)session->buffer;
    end_buffer = (char *)session->buffer + session->buffer_length;

    /* get/post url HTTP/1.1 */
    offset = rt_snprintf(ptr, end_buffer - ptr, "%s %s HTTP/1.1\r\n", type==0?"GET":"POST", url);
    ptr += offset;
    offset = rt_snprintf(ptr, end_buffer - ptr, "Host: %s\r\n", session->host);
    ptr += offset;
    offset = rt_snprintf(ptr, end_buffer - ptr, "Accept: %s\r\nConnection: %s\r\n", accept, keep_alive?"Keep-Alive":"Close");
    ptr += offset;
    if (content_length > 0) {
        offset = rt_snprintf(ptr, end_buffer - ptr, "Content-Length: %d\r\n", content_length);
        ptr += offset;
    }
    offset = rt_snprintf(ptr, end_buffer - ptr, "User-Agent: RTU/%d.%02d (Power by Jay) \r\n\r\n", SW_VER_MAJOR, SW_VER_MINOR );
    ptr += offset;

    session->buffer_offset = ptr - (char *)session->buffer;
    hclient_session_write(session, session->buffer, session->buffer_offset);
    //rt_kprintf("%s\n", session->buffer);
}

#include "cJSON.h"
#include "cc_md5.h"

void hclient_get_sign(cJSON *json, char *sign, ...)
{
	va_list     ap;
	char        *key;
	CC_MD5_CTX  md5;

	memset(&md5, 0, sizeof(md5));
	CC_MD5Init(&md5);
	va_start(ap, sign);
    CC_MD5Update(&md5, (unsigned char *)"32949238UI7789IasdO0_", 21);
	while ((key = va_arg(ap, char *))) {
	    const char *value = cJSON_GetString(json, (const char *)key, 0);
	    if (value) {
	        //hclient_dbg("%s,%s\n", key, value);
            CC_MD5Update(&md5, (unsigned char *)value, strlen(value));
        }
	}
	va_end(ap);
	CC_MD5Final(&md5);
    rt_sprintf(sign, 
           "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
           md5.digest[0], md5.digest[1], md5.digest[2], md5.digest[3],
           md5.digest[4], md5.digest[5], md5.digest[6], md5.digest[7],
           md5.digest[8], md5.digest[9], md5.digest[10], md5.digest[11],
           md5.digest[12], md5.digest[13], md5.digest[14], md5.digest[15]);
}

