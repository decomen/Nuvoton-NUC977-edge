/*
 * File      : request.c
 * This file is part of RT-Thread RTOS/WebNet Server
 * COPYRIGHT (C) 2011, Shanghai Real-Thread Technology Co., Ltd
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 * 2011-08-05     Bernard      fixed the query issue
 * 2011-08-18     Bernard      fixed chrome query issue
 */

#include "request.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "util.h"

static void _webnet_request_decode_url(char *decoded, char *token, int len)
{
    char	*ip,  *op;
    int		num, i, c;

    RT_ASSERT(decoded);
    RT_ASSERT(token);

    op = decoded;
    for (ip = token; *ip && len > 0; ip++, op++) {
        if (*ip == '+') {
            *op = ' ';
        } else if (*ip == '%' && isxdigit(ip[1]) && isxdigit(ip[2])) {

/*
 *			Convert %nn to a single character
 */
            ip++;
            for (i = 0, num = 0; i < 2; i++, ip++) {
                c = tolower(*ip);
                if (c >= 'a' && c <= 'f') {
                    num = (num * 16) + 10 + c - 'a';
                } else {
                    num = (num * 16) + c - '0';
                }
            }
            *op = (char)num;
            ip--;

        } else {
            *op = *ip;
        }
        len--;
    }
    *op = '\0';
}

/**
 * parse a query
 */
static void _webnet_request_parse_query(struct webnet_request *request)
{
    char *ptr;
    rt_uint32_t index;

    if ((request->query == RT_NULL)/*|| (*request->query == '\0')*/) {
        request->query = RT_NULL;
        return; /* no query */
    }

    /* copy query */
    request->query = rt_strdup(request->query);
    request->field_query_copied = RT_TRUE;

    /* get the query counter */
    ptr = request->query;
    request->query_counter = 1;
    while (*ptr) {
        if (*ptr == '&') {
            while ((*ptr == '&') && (*ptr != '\0')) ptr++;
            if (*ptr == '\0') break;

            request->query_counter++;
        }
        ptr++;
    }
    if (request->query_counter == 0) return; /* no query */

    /* allocate query item */
    request->query_items = (struct webnet_query_item *)WEBNET_MALLOC(sizeof(struct webnet_query_item)
                                                                 * request->query_counter);
    if (request->query_items == RT_NULL) {
        request->result_code = 500;
        return;
    }

    ptr = request->query;
    for (index = 0; index < request->query_counter; index++) {
        request->query_items[index].name = ptr;
        request->query_items[index].value = RT_NULL;

        /* get value or goto next item */
        while ((*ptr != '&') && (*ptr != '\0')) {
            /* get value */
            if (*ptr == '=') {
                *ptr = '\0'; ptr++;
                request->query_items[index].value = ptr;
            } else
                ptr++;
        }

        if (*ptr == '\0') break;
        if (*ptr == '&') {
            /* make a item */
            *ptr = '\0'; ptr++;
            while (*ptr == '&' && *ptr != '\0') ptr++;
            if (*ptr == '\0') break;
        }
    }
    for (index = 0; index < request->query_counter; index++) {
        ptr = request->query_items[index].value;
        if (ptr) _webnet_request_decode_url(ptr, ptr, strlen(ptr));
    }
}

/**
 * copy string from request and the field_copied set to TRUE
 */
static void _webnet_request_copy_str(struct webnet_request *request)
{
    if (request->path != RT_NULL) request->path = rt_strdup(request->path);
    if (request->host != RT_NULL) request->host = rt_strdup(request->host);
    if (request->cookie != RT_NULL) request->cookie = rt_strdup(request->cookie);
    if (request->user_agent != RT_NULL) request->user_agent = rt_strdup(request->user_agent);
    if (request->authorization != RT_NULL) request->authorization = rt_strdup(request->authorization);
    if (request->accept_language != RT_NULL) request->accept_language = rt_strdup(request->accept_language);
    if (request->referer != RT_NULL) request->referer = rt_strdup(request->referer);
    if (request->multiPartBoundary != RT_NULL) {
        rt_size_t len = (rt_strlen(request->multiPartBoundary) + 16);
        char *tmp = (char *)WEBNET_MALLOC(len);
        if (tmp) sprintf(tmp, "--%s", request->multiPartBoundary);
        request->multiPartBoundary = tmp;
    }
    if (request->upgrade != RT_NULL) request->upgrade = rt_strdup(request->upgrade);
    if (request->websk_key != RT_NULL) request->websk_key = rt_strdup(request->websk_key);

    /* not copy query, which is copied in parse query function */
    /* if (request->query != RT_NULL) request->query = rt_strdup(request->query); */

    request->field_copied = RT_TRUE;
}

/**
 * to check whether a query on the http request.
 */
rt_bool_t webnet_request_has_query(struct webnet_request *request, const char *name)
{
    rt_uint32_t index;

    for (index = 0; index < request->query_counter; index++) {
        if (strncmp(request->query_items[index].name, name, strlen(name)) == 0) return RT_TRUE;
    }

    return RT_FALSE;
}

/**
 * get query value according to the name
 */
const char* webnet_request_get_query(struct webnet_request *request, const char *name)
{
    rt_uint32_t index;

    for (index = 0; index < request->query_counter; index++) {
        if (strncmp(request->query_items[index].name, name, strlen(name)) == 0) return request->query_items[index].value;
    }

    return RT_NULL;
}

/**
 * parse web request
 */
void webnet_request_parse(struct webnet_request *request, char *buffer, int length)
{
    char *ch;
    char *request_buffer;
    char *content_length;

    RT_ASSERT(request != RT_NULL);
    RT_ASSERT(request->session != RT_NULL);

    content_length = RT_NULL;
    request_buffer = buffer;

    /* web request begin with method */
    if (str_begin_with(request_buffer, "GET ")) {
        request->method = WEBNET_GET; request_buffer += 4;
    } else if (str_begin_with(request_buffer, "PUT ")) {
        request->method = WEBNET_PUT; request_buffer += 4;
    } else if (str_begin_with(request_buffer, "POST ")) {
        request->method = WEBNET_POST; request_buffer += 5;
    } else if (str_begin_with(request_buffer, "HEADER ")) {
        request->method = WEBNET_HEADER; request_buffer += 7;
    } else {
        /* Not implemented for other method */
        request->result_code = 501;
        return;
    }

    /* get path */
    request->path = request_buffer;
    if (
        str_begin_with(request->path, "/file/update0") ||
        str_begin_with(request->path, "/file/cfgrecover0") ||
        str_begin_with(request->path, "/file/cfgrecover_with_json") ||
        str_begin_with(request->path, "/file/var_ext_add_dev_with_json") ||
        str_begin_with(request->path, "/file/upload/") ||
        str_begin_with(request->path, "/ini/upload/")||
        str_begin_with(request->path, "/lua/upload/") ) {
        request->upload_path = 1;
    }
    ch = strchr(request_buffer, ' ');
    if (ch == RT_NULL) {
        /* Bad Request */
        request->result_code = 400;
        return;
    }
    *ch++ = '\0';
    request_buffer = ch;

    /* check path, whether there is a query */
    ch = strchr(request->path, '?');
    if (ch != RT_NULL) {
        *ch++ = '\0';
        while (*ch == ' ') ch++;
        /* set query */
        request->query = ch;
        
        /* copy query and parse parameter */
        _webnet_request_parse_query(request);
    }

    /* check protocol */
    if (!str_begin_with(request_buffer, "HTTP/1")) {
        /* Not implemented, webnet does not support HTTP 0.9 protocol */
        request->result_code = 501;
        return;
    }

    ch = strstr(request_buffer, "\r\n");
    if (ch == RT_NULL) {
        /* Bad Request */
        request->result_code = 400;
        return;
    }
    *ch++ = '\0'; *ch++ = '\0';
    request_buffer = ch;

    for (;;) {
        if (str_begin_with(request_buffer, "\r\n")) {
            /* get content length */
            if (content_length != RT_NULL) request->content_length = atoi(content_length);
            
            if (request->method == WEBNET_POST) { /* POST method */
                if (request->multiPartBoundary) {
                    _webnet_request_copy_str(request);
                    request->result_code = 200;
                    return;
                } else if( request->upload_path ) {
                    _webnet_request_copy_str(request);
                    request->result_code = 200;
                    request_buffer += 2;
                    if( request_buffer - buffer < length ) {
                        request->session->buffer_offset = (length - (request_buffer - buffer));
                        memcpy( request->session->buffer, request_buffer, request->session->buffer_offset );
                    }
                    return;
                }

                request->query = request_buffer += 2;

                /* check content */
                if (*request->query != '\0') _webnet_request_parse_query(request);
                else {
                    struct webnet_session *session;
                    int read_bytes;

                    session = request->session;
                    memset(session->buffer, 0, session->buffer_length);
                    read_bytes = webnet_session_read(session, (char *)session->buffer, session->buffer_length - 1);
                    session->buffer_offset = read_bytes;
                    if (read_bytes == request->content_length) {
                        request->query = (char *)session->buffer;
                        _webnet_request_parse_query(request);
                    } else {
                        request->query = RT_NULL;
                    }
                }
            }

            /* end of http request */
            request->result_code = 200;

            /* made a string field copy */
            _webnet_request_copy_str(request);
            return;
        }

        if (*request_buffer == '\0') {
            /* end of http request */
            request->result_code = 200;

            /* made a string field copy */
            _webnet_request_copy_str(request);
            return;
        }

        if (str_begin_with(request_buffer, "Host:")) {
            /* get host */
            request_buffer += 5;
            while (*request_buffer == ' ') request_buffer++;
            request->host = request_buffer;
        } else if (str_begin_with(request_buffer, "User-Agent:")) {
            /* get user agent */
            request_buffer += 11;
            while (*request_buffer == ' ') request_buffer++;
            request->user_agent = request_buffer;
        } else if (str_begin_with(request_buffer, "Accept-Language:")) {
            /* get accept language */
            request_buffer += 16;
            while (*request_buffer == ' ') request_buffer++;
            request->accept_language = request_buffer;
        } else if (str_begin_with(request_buffer, "Content-Length:")) {
            /* get content length */
            request_buffer += 15;
            while (*request_buffer == ' ') request_buffer++;
            content_length = request_buffer;
        } else if (str_begin_with(request_buffer, "Content-Type:")) {
            /* get content type */
            request_buffer += 13;
            while (*request_buffer == ' ') request_buffer++;
            if (str_begin_with(request_buffer, "multipart/form-data")) {
                if ((ch = strstr(request_buffer, "boundary="))) {
                    request_buffer = (ch + strlen("boundary="));
                    request->multiPartBoundary = request_buffer;
                }
            } else {
                request->multiPartBoundary = RT_NULL;
            }
        } else if (str_begin_with(request_buffer, "Referer:")) {
            /* get referer */
            request_buffer += 8;
            while (*request_buffer == ' ') request_buffer++;
            request->referer = request_buffer;
        }
#ifdef WEBNET_USING_KEEPALIVE
        else if (str_begin_with(request_buffer, "Connection:")) {
            /* set default connection to keep alive */
            request->connection = WEBNET_CONN_KEEPALIVE;

            /* get connection */
            request_buffer += 11;
            while (*request_buffer == ' ') request_buffer++;

            if (str_begin_with(request_buffer, "close")) request->connection = WEBNET_CONN_CLOSE;
            else if (str_begin_with(request_buffer, "Keep-Alive")) request->connection = WEBNET_CONN_KEEPALIVE;
        }
#endif
#ifdef WEBNET_USING_COOKIE
        else if (str_begin_with(request_buffer, "Cookie:")) {
            /* get cookie */
            request_buffer += 7;
            while (*request_buffer == ' ') request_buffer++;
            request->cookie = request_buffer;
        }
#endif
#ifdef WEBNET_USING_AUTH
        else if (str_begin_with(request_buffer, "Authorization: Basic")) {
            /* get authorization */
            request_buffer += 20;
            while (*request_buffer == ' ') request_buffer++;
            request->authorization = request_buffer;
        }
#endif
#ifdef WEBNET_USING_WEBSOCKET
        else if (str_begin_with(request_buffer, "Upgrade:")) {
            /* get Upgrade*/
            request_buffer += 8;
            while (*request_buffer == ' ') request_buffer++;
            request->upgrade = request_buffer;
            if (str_begin_with( request->upgrade, "websocket")) {
                request->session->iswebsk = RT_TRUE;
            }
        } else if (str_begin_with(request_buffer, "Sec-WebSocket-Key:")) {
            /* get Sec-WebSocket-Key*/
            request_buffer += 18;
            while (*request_buffer == ' ') request_buffer++;
            request->websk_key = request_buffer;
            request->session->websk.state = WEB_SK_HAND;
        }
#endif
        ch = strstr(request_buffer, "\r\n");
        if (ch == RT_NULL) {
            /* Bad Request */
            request->result_code = 400;
            return;
        }
        /* terminal field */
        *ch++ = '\0';
        *ch++ = '\0';

        request_buffer = ch;
    }
}

struct webnet_request* webnet_request_create()
{
    struct webnet_request *request;

    request = (struct webnet_request *)WEBNET_MALLOC(sizeof(struct webnet_request));
    if (request != RT_NULL) {
        rt_memset(request, 0, sizeof(struct webnet_request));
        request->field_copied = RT_FALSE;
        request->field_query_copied = RT_FALSE;
    }

    return request;
}

void webnet_request_destory(struct webnet_request *request)
{
    if (request != RT_NULL) {
        if (request->field_copied == RT_TRUE) {
            WEBNET_FREE(request->path);
            WEBNET_FREE(request->host);
            WEBNET_FREE(request->cookie);
            WEBNET_FREE(request->user_agent);
            WEBNET_FREE(request->authorization);
            WEBNET_FREE(request->accept_language);
            WEBNET_FREE(request->referer);
            WEBNET_FREE(request->multiPartBoundary);
            WEBNET_FREE(request->upgrade);
            WEBNET_FREE(request->websk_key);
        }
        if (request->field_query_copied == RT_TRUE) {
            WEBNET_FREE(request->query);
            WEBNET_FREE(request->query_items);
        }

        /* free request memory block */
        WEBNET_FREE(request);
    }
}

