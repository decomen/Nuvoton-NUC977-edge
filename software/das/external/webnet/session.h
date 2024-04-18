/*
 * File      : session.h
 * This file is part of RT-Thread RTOS/WebNet Server
 * COPYRIGHT (C) 2011, Shanghai Real-Thread Technology Co., Ltd
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 */
#ifndef __WEBNET_SESSION_H__
#define __WEBNET_SESSION_H__

#include <rtdef.h>
#include <webnet.h>
#include <request.h>
#include "rtu_reg.h"

#include "websocket.h"
#include "sha1.h"
#include "base64.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#define WEBNET_SESSION_BUFSZ	8192
#define WEBNET_SESSION_CHKSZ 	8192

struct webnet_session
{
    struct webnet_session *next;

	/* socket information */
    int socket;
    struct sockaddr_in cliaddr;

    rt_bool_t   iswebsk;
    websocket_t websk;

	/* webnet request */
	struct webnet_request* request;

	/* session buffer */
	rt_uint16_t buffer_length;
	rt_uint16_t buffer_offset;
	rt_uint8_t  buffer[WEBNET_SESSION_BUFSZ];
};

struct webnet_session* webnet_session_create(int listenfd);

int webnet_session_read(struct webnet_session *session, char *buffer, int length);
void webnet_session_close(struct webnet_session *session);
void webnet_session_close_all(void);

void webnet_session_printf(struct webnet_session *session, const char* fmt, ...);
int webnet_session_write(struct webnet_session *session, const rt_uint8_t* data, rt_size_t size);
void webnet_session_set_header(struct webnet_session *session, const char* mimetype, int code, const char* status, int length);

int webnet_sessions_set_fds(fd_set *readset, fd_set *writeset);
void webnet_sessions_handle_fds(fd_set *readset, fd_set *writeset);

void webnet_session_set_websocke_accept_header(struct webnet_session *session);
void webnet_session_set_websocke_error_header(struct webnet_session *session);

void vDoSystemReset(void);

#endif

