/*
 * File      : session.c
 * This file is part of RT-Thread RTOS/WebNet Server
 * COPYRIGHT (C) 2011, Shanghai Real-Thread Technology Co., Ltd
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 */

#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mimetype.h"
#include "util.h"
#include "module.h"

#include "das_os.h"

#include "ws_vm.h"

static struct webnet_session *_session_list = 0;
static void _webnet_session_badrequest(struct webnet_session *session, int code);

/**
 * create a webnet session
 *
 * @param listenfd, the listen file descriptor
 *
 * @return the created web session
 */
struct webnet_session* webnet_session_create(int listenfd)
{
    struct webnet_session *session;

    /* create a new session */
    session = (struct webnet_session *)WEBNET_CALLOC(1, sizeof(struct webnet_session));
    if (session != RT_NULL) {
        socklen_t clilen;

        clilen = sizeof(struct sockaddr_in);
        session->socket = lwip_accept(listenfd, (struct sockaddr *)&(session->cliaddr), &clilen);
        if (session->socket < 0) {
            WEBNET_FREE(session);
            // add by jay 2016/11/24
            return RT_NULL;
        } else {
            /* keep this tecb in our list */
            session->next = _session_list;
            _session_list = session;
        }

        /* initial buffer length */
        session->buffer_length = WEBNET_SESSION_BUFSZ;
    }

    return session;
}

int webnet_session_read_http_head(struct webnet_session *session, char *buffer, int length)
{
    int read_count;

    /* Read some data */
    read_count = recv(session->socket, buffer, length, MSG_PEEK);
    if (read_count <= 0) {
        //webnet_session_close(session);
        return -1;
    }
    
    char *head = strstr(buffer, "\r\n\r\n");
    if (head) {
        length = (head - buffer + 4);
        read_count = read(session->socket, buffer, length);
    } else {
        printf("webnet_session_read_http_head error: %s\n", buffer);
        return -1;
    }

    return read_count;
}

/**
 * read data from a webnet session
 *
 * @param session, the web session
 * @param buffer, the buffer to save read data
 * @param length, the maximal length of data buffer to save read data
 *
 * @return the number of bytes actually read data
 */
int webnet_session_read(struct webnet_session *session, char *buffer, int length)
{
    int read_count;

    /* Read some data */
    read_count = lwip_read(session->socket, buffer, length);
    if (read_count <= 0) {
        //webnet_session_close(session);
        return -1;
    }

    return read_count;
}

/**
 * close a webnet session
 *
 * @param session, the web session
 */
void webnet_session_close(struct webnet_session *session)
{
    struct webnet_session *iter;

    /* Either an error or tcp connection closed on other
     * end. Close here */
    lwip_shutdown(session->socket, SHUT_RDWR);
    lwip_close(session->socket);

    if (session->iswebsk) {
        websocket_close(session);
        ws_vm_clear();
        session->iswebsk = RT_FALSE;
    }

    /* Free webnet_session */
    if (_session_list == session) _session_list = session->next;
    else {
        for (iter = _session_list; iter; iter = iter->next) {
            if (iter->next == session) {
                iter->next = session->next;
                break;
            }
        }
    }

    if (session->request != RT_NULL) {
        webnet_request_destory(session->request);
        session->request = RT_NULL;
    }
    WEBNET_FREE(session);
}

void webnet_session_close_all(void)
{
    struct webnet_session *iter;
    for (iter = _session_list; iter; iter = iter->next) {
        webnet_session_close(iter);
    }
}

/**
 * print formatted data to session
 *
 * @param session, the web session
 * @param fmt, the format string
 */
void webnet_session_printf(struct webnet_session *session, const char *fmt, ...)
{
    va_list args;
    int len = 0;

    va_start(args, fmt);
    rt_memset(session->buffer, 0, sizeof(session->buffer));
    //rt_vsprintf((char *)(session->buffer), fmt, args);
    len = rt_vsnprintf((char *)(session->buffer), session->buffer_length, fmt, args);
    va_end(args);

    lwip_send(session->socket, session->buffer, len, MSG_NOSIGNAL);
}

/**
 * write data to session
 *
 * @param session, the web session
 * @param data, the data will be write to the session
 * @param size, the size of data bytes
 *
 * @return the number of bytes actually written to the session
 */
int webnet_session_write(struct webnet_session *session, const rt_uint8_t *data, rt_size_t size)
{
    /* send data directly */
    lwip_send(session->socket, data, size, MSG_NOSIGNAL);

    return size;
}

void webnet_session_set_websocke_accept_header(struct webnet_session *session)
{
    const char *WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char accept_key[128] = "";
    static const char *fmt = \
        "HTTP/1.1 101 Websocket Protocol HandShake \r\n"
                             "Connection: Upgrade\r\n"
                             "Upgrade: websocket\r\n"
                             "Sec-WebSocket-Accept: %s\r\n\r\n";

    {
        SHA1_CONTEXT sha;
        sha1_init(&sha);
        sha1_write(&sha, (unsigned char *)session->request->websk_key, strlen(session->request->websk_key));
        sha1_write(&sha, (unsigned char *)WEBSOCKET_GUID, strlen(WEBSOCKET_GUID));
        sha1_final(&sha);
        base64_encode(sha.buf, accept_key, 20);
        session->buffer_offset = rt_sprintf((char *)session->buffer, fmt, accept_key);
        webnet_session_write(session, session->buffer, session->buffer_offset);
    }
}

void webnet_session_set_websocke_error_header(struct webnet_session *session)
{
    static const char *head = \
        "HTTP/1.1 404 Websocket Protocol Error \r\n"
                              "Upgrade: websocket\r\n"
                              "Connection: Upgrade\r\n";

    webnet_session_write(session, (const rt_uint8_t *)head, strlen(head));
}

/**
 * set the http response header
 *
 * @param session the web session
 * @param mimetype the mime type of http response
 * @param code the http response code
 * @param title the code title string
 * @param length the length of http response content
 */
void webnet_session_set_header(struct webnet_session *session, const char *mimetype, int code, const char *title, int length)
{
    static const char *fmt = "HTTP/1.1 %d %s\r\n%s";
    static const char *content = "Content-Type: %s\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n";
    static const char *content_nolength = "Content-Type: %s\r\nConnection: %s\r\n\r\n";
    static const char *auth = "WWW-Authenticate: Basic realm=%s\r\n";

    char *ptr, *end_buffer;
    int offset;

    ptr = (char *)session->buffer;
    end_buffer = (char *)session->buffer + session->buffer_length;

    offset = rt_snprintf(ptr, end_buffer - ptr, fmt, code, title, WEBNET_SERVER);
    ptr += offset;

    if (code == 401) {
        offset = rt_snprintf(ptr, end_buffer - ptr, auth, session->request->host ? session->request->host : "");
        ptr += offset;
    }
    if (length >= 0) {
        offset = rt_snprintf(ptr, end_buffer - ptr, content, mimetype, length,
                             session->request->connection == WEBNET_CONN_CLOSE ? "close" : "Keep-Alive");
        ptr += offset;
    } else {
        offset = rt_snprintf(ptr, end_buffer - ptr, content_nolength, mimetype, "close");
        ptr += offset;
    }
    session->buffer_offset = ptr - (char *)session->buffer;

    /* invoke webnet event */
    if (webnet_module_handle_event(session, WEBNET_EVENT_RSP_HEADER) == WEBNET_MODULE_CONTINUE) {
        /* write to session */
        webnet_session_write(session, session->buffer, session->buffer_offset);
    }
}

static void _webnet_session_badrequest(struct webnet_session *session, int code)
{
    const char *title;
    static const char *fmt = "<html><head><title>%d %s</title></head><body>%d %s</body></html>\r\n";

    title = "Unknown";
    switch (code) {
    case 304:
        title = "Not Modified";
        break;
    case 400:
        title = "Bad Request";
        break;
    case 401:
        title = "Authorization Required";
        break;
    case 403:
        title = "Forbidden";
        break;
    case 404:
        title = "Not Found";
        break;
    case 405:
        title = "Method Not Allowed";
        break;
    case 500:
        title = "Internal Server Error";
        break;
    case 501:
        title = "Not Implemented";
        break;
    case 505:
        title = "HTTP Version Not Supported";
        break;
    }
#ifdef WEBNET_USING_KEEPALIVE
    if (code >= 400) {
        session->request->connection = WEBNET_CONN_CLOSE;
    }
#endif

    webnet_session_set_header(session, "text/html", code, title, 0);

    webnet_session_printf(session, fmt, code, title, code, title);
}

/**
 * set the file descriptors
 *
 * @param readset, the file descriptors set for read
 * @param writeset, the file descriptors set for write
 *
 * @return the maximal file descriptor
 */
int webnet_sessions_set_fds(fd_set *readset, fd_set *writeset)
{
    int maxfdp1 = 0;
    struct webnet_session *session;

    for (session = _session_list; session; session = session->next) {
        if (maxfdp1 < session->socket + 1) maxfdp1 = session->socket + 1;

        FD_SET(session->socket, readset);
    }

    return maxfdp1;
}

/**
 * handle the file descriptors request
 *
 * @param readset, the file descriptors set for read
 * @param writeset, the file descriptors set for write
 */
void webnet_sessions_handle_fds(fd_set *readset, fd_set *writeset)
{
    int read_length;
    char read_buffer[WEBNET_SESSION_BUFSZ * 2];
    struct webnet_session *session;

    /* Go through list of connected session and process data */
    for (session = _session_list; session;) {
        struct webnet_session *session_next = session->next;

        if (FD_ISSET(session->socket, readset)) {
            memset(read_buffer, 0, sizeof(read_buffer));
            /* This socket is ready for reading. */
            if (!session->iswebsk) {
                read_length = webnet_session_read_http_head(session, read_buffer, WEBNET_SESSION_BUFSZ * 2 - 1);
            } else {
                read_length = webnet_session_read(session, read_buffer, WEBNET_SESSION_BUFSZ * 2 - 1);
            }
            if (read_length <= 0) {
                webnet_session_close(session);
                break;
            } else {
                if (!session->iswebsk) {
                    struct webnet_request *request = RT_NULL;

                    /* made a \0 terminator */
                    read_buffer[read_length] = 0;

                    /* destory old request */
                    if (session->request != RT_NULL) {
                        webnet_request_destory(session->request);
                        session->request = RT_NULL;
                    }

                    /* create request */
                    request = webnet_request_create();
                    if (request != RT_NULL) {
                        session->request = request;
                        request->session = session;

                        /* parse web request */
                        webnet_request_parse(request, read_buffer, read_length);
                        //printf("buffer_offset = %d\n", session->buffer_offset);
                        if (!session->iswebsk) {
                            if (request->result_code == 200) {
                                /* handle url */
                                if (webnet_module_handle_uri(session) == WEBNET_MODULE_CONTINUE) {
                                    /* send file */
                                    webnet_module_system_dofile(session);
                                } else {
                                    if (request->result_code != 200) _webnet_session_badrequest(session, request->result_code);
                                }
                            } else {
                                _webnet_session_badrequest(session, request->result_code);
                            }
                        } else {
                            if (request->result_code == 200 && session->request->websk_key) {
                                webnet_session_set_websocke_accept_header(session);
                                session->websk.state = WEB_SK_CONNECTED;
                            } else {
                                webnet_session_set_websocke_error_header(session);
                                session->websk.state = WEB_SK_CLOSED;
                            }
                        }
                    }
                    if (!session->iswebsk) {
                        /* close this session */
                        webnet_session_close(session);
                        session = NULL;
                    }
                } else {
                    websocket_handler_data(session, (const rt_uint8_t *)read_buffer, read_length);
                    if (WEB_SK_CLOSED == session->websk.state) {
                        webnet_session_close(session);
                        session = NULL;
                    }
                }

                if (session && session->iswebsk && WEB_SK_CONNECTED == session->websk.state) {
                    websocket_set(session);
                }

                if (!session_next) break;
            }
        }

        session = session_next;
    }
}
