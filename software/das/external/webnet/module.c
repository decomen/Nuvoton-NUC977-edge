/*
 * File      : module.c
 * This file is part of RT-Thread RTOS/WebNet Server
 * COPYRIGHT (C) 2011, Shanghai Real-Thread Technology Co., Ltd
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 */

#include "module.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "mimetype.h"

#include "board.h"

int webnet_module_system_do_uploadfile(struct webnet_session *session, const char *path, rt_bool_t reboot, rt_bool_t reset);
int webnet_module_system_do_uploadtext(struct webnet_session *session, const char *path, rt_bool_t reboot, rt_bool_t reset);

static int _webnet_module_system_init(struct webnet_session *session, int event)
{
#ifdef WEBNET_USING_LOG
    webnet_module_log(session, event);
#endif

#ifdef WEBNET_USING_SSL
    webnet_module_ssl(session, event);
#endif

#ifdef WEBNET_USING_CGI
    webnet_module_cgi(session, event);
#endif

    webnet_module_ws(session, event);

    return WEBNET_MODULE_CONTINUE;
}

static int _webnet_module_system_uri_physical(struct webnet_session *session, int event)
{
    int result;
    result = WEBNET_MODULE_CONTINUE;

#ifdef WEBNET_USING_LOG
    webnet_module_log(session, event);
#endif

#ifdef WEBNET_USING_ALIAS
    result = webnet_module_alias(session, event);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#ifdef WEBNET_USING_AUTH
    result = webnet_module_auth(session, event);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#ifdef WEBNET_USING_CGI
    result = webnet_module_cgi(session, event);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

    if (str_begin_with(session->request->path, "/file/update0")) {
        printf("  >>>>>>>>>>>> begain uploadfile\r\n");
        result = webnet_module_system_do_uploadfile(session, "/media/update/update.pack", RT_FALSE, RT_FALSE);
        printf("file upload finish: %s\r\n", "/media/update/update.pack");
        printf("  >>>>>>>>>>>> begain update\r\n");
        my_system("/usr/fs/bin/start_update.sh");
        if (result == WEBNET_MODULE_FINISHED) return result;
    } else if (str_begin_with(session->request->path, "/file/cfgrecover_with_json")) {
        result = webnet_module_system_do_uploadfile(session, BOARD_CFG_PATH"board_json.cfg", RT_FALSE, RT_FALSE);
        if(200 == session->request->result_code) {
            if(cfg_recover_with_json(BOARD_CFG_PATH"board_json.cfg")) {
                vDoSystemReboot();
            }
        }
        if (result == WEBNET_MODULE_FINISHED) return result;
    } else if (str_begin_with(session->request->path, "/file/var_ext_add_dev_with_json")) {
        result = webnet_module_system_do_uploadfile(session, "/tmp/tmp.json", RT_FALSE, RT_FALSE);
        if(200 == session->request->result_code) {
            varmanage_add_dev_with_json(session, "/tmp/tmp.json");
        }
        if (result == WEBNET_MODULE_FINISHED) return result;
    } else if (str_begin_with(session->request->path, "/file/upload")) {
        char *filepath = &session->request->path[strlen("/file/upload")];
        if (0 == das_mkdir_p(filepath, 0755)) {
            result = webnet_module_system_do_uploadfile(session, filepath, RT_FALSE, RT_FALSE);
        }
        if (result == WEBNET_MODULE_FINISHED) return result;
    } else if (str_begin_with(session->request->path, "/ini/upload")) {
        char *filepath = &session->request->path[strlen("/ini/upload")];
        if (0 == das_mkdir_p(filepath, 0755)) {
            result = webnet_module_system_do_uploadtext(session, filepath, RT_FALSE, RT_FALSE);
        }
        if (result == WEBNET_MODULE_FINISHED) return result;
    } else if (str_begin_with(session->request->path, "/lua/upload")) {
        char *filepath = &session->request->path[strlen("/lua/upload")];
        if (0 == das_mkdir_p(filepath, 0755)) {
            result = webnet_module_system_do_uploadtext(session, filepath, RT_FALSE, RT_FALSE);
        }
        if (result == WEBNET_MODULE_FINISHED) return result;
    }

    return result;
}


int webnet_module_system_do_uploadtext(struct webnet_session *session, const char *path, rt_bool_t reboot, rt_bool_t reset)
{
    int fd; /* file descriptor */
    int remain, read_bytes;

    session->request->result_code = 200;

    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) {
        session->request->result_code = 500;
        return WEBNET_MODULE_FINISHED;
    }
    remain = session->request->content_length;

    if( session->buffer_offset > 0 ) {
        write(fd, session->buffer, session->buffer_offset);
        remain -= session->buffer_offset;
    }

    while (remain > 0) {
        rt_thddog_feed("webnet_module_system_do_uploadtext");
        memset(session->buffer, 0, session->buffer_length);
        read_bytes = webnet_session_read(session, (char *)session->buffer, session->buffer_length - 1);
        if (read_bytes > 0) {
            int r_sz = read_bytes;
            while (r_sz > 0) {
                int w_len = write(fd, &session->buffer[read_bytes - r_sz], r_sz);
                if (w_len <= 0) break;
                r_sz -= w_len;
            }
            remain -= read_bytes;
        } else {
            break;
        }
    }

    close(fd);

    // 上传被中断, 删除文件
    if (remain > 0) {
        unlink(path);
    }

    if (200 == session->request->result_code) {
        webnet_session_set_header(session, "text/html", 200, "OK", -1);

        if (reboot) {
            extern void vDoSystemReboot(void);
            vDoSystemReboot();
        } else if (reset) {
            extern void vDoSystemReset(void);
            vDoSystemReset();
        }
    }

    return WEBNET_MODULE_FINISHED;
}

/* do upload file */
int webnet_module_system_do_uploadfile(struct webnet_session *session, const char *path, rt_bool_t reboot, rt_bool_t reset)
{
    int fd; /* file descriptor */
    int remain, read_bytes;
    rt_bool_t startOfHeader = RT_FALSE;
    rt_bool_t endOfHeader = RT_FALSE;
    rt_uint8_t *part = RT_NULL;
    int endlen = 2 + strlen(session->request->multiPartBoundary) + 2 + 2;

    session->request->result_code = 200;

    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) {
        session->request->result_code = 500;
        return WEBNET_MODULE_FINISHED;
    }

    remain = session->request->content_length;

    while (remain > 0) {
        rt_thddog_feed("webnet_module_system_do_uploadfile");
        memset(session->buffer, 0, session->buffer_length);
        read_bytes = webnet_session_read(session, (char *)session->buffer, session->buffer_length - 1);
        if (read_bytes > 0) {
            part = session->buffer;
            if (!startOfHeader) {
                if (strstr((char const *)part, "Content-Disposition")) {
                    startOfHeader = RT_TRUE;
                }
            }

            if (startOfHeader && !endOfHeader) {
                while (strchr((char const *)part, '\n')) {
                    if ((part = (rt_uint8_t *)strchr((char const *)part, '\n')) != RT_NULL) {
                        part++;
                        if ((*part) == '\n') {
                            part += 1;
                            endOfHeader = 1;
                        } else if (((*part) == '\r') && ((*(part + 1)) == '\n')) {
                            part += 2;
                            endOfHeader = 1;
                        }
                    }
                    if (endOfHeader) {
                        break;
                    }
                }
            }

            if (endOfHeader) {
                int ofs = part - session->buffer;
                int len = read_bytes;

                if (ofs < len) {
                    if (remain <= endlen) {
                        len = 0;
                    } else if (remain - len <= endlen) {
                        len -= (endlen - (remain - len));
                    }
                    len -= ofs;
                    if (len > 0) {
                        int r_sz = len;
                        while (r_sz > 0) {
                            int w_len = write(fd, &part[len - r_sz], r_sz);
                            if (w_len <= 0) break;
                            r_sz -= w_len;
                        }
                    }
                }
            }

            remain -= read_bytes;
        } else {
            break;
        }
    }

    close(fd);

    // 上传被中断, 删除文件
    if (remain > 0) {
        unlink(path);
        session->request->result_code = 500;
    }

    if (200 == session->request->result_code) {
        webnet_session_set_header(session, "text/html", 200, "OK", -1);

        if (reboot) {
            extern void vDoSystemReboot(void);
            vDoSystemReboot();
        } else if (reset) {
            extern void vDoSystemReset(void);
            vDoSystemReset();
        }
    }

    return WEBNET_MODULE_FINISHED;
}

extern void hjt212_try_create_default_config_file(const char *path);
extern void mqtt_try_create_default_config_file(const char *path);
extern void dh_try_create_default_config_file(const char *path);

/* send a file to http client */
int webnet_module_system_dofile(struct webnet_session *session)
{
    int fd; /* file descriptor */
    const char *mimetype;
    rt_size_t file_length;
    struct webnet_request *request;

    RT_ASSERT(session != RT_NULL);
    request = session->request;
    RT_ASSERT(request != RT_NULL);

    /* get mime type */
    mimetype = mime_get_type(request->path);
    hjt212_try_create_default_config_file(request->path);
    mqtt_try_create_default_config_file(request->path);
    dh_try_create_default_config_file(request->path);
    fd = open(request->path, O_RDONLY, 0);
    if (fd < 0) {
        request->result_code = 404;
        return WEBNET_MODULE_FINISHED;
    }

    /* get file size */
    file_length = lseek(fd, 0, SEEK_END);
    /* seek to beginning of file */
    lseek(fd, 0, SEEK_SET);

    /* send file to remote */
    request->result_code = 200;
    webnet_session_set_header(session, mimetype, request->result_code, "OK", file_length);
    if (file_length <= 0) {
        close(fd);
        return WEBNET_MODULE_FINISHED;
    }

    if (request->method != WEBNET_HEADER) {
        rt_size_t size, readbytes;
        while (file_length) {
            if (file_length > WEBNET_SESSION_BUFSZ) size = (rt_size_t)WEBNET_SESSION_BUFSZ;
            else
                size = file_length;

            readbytes = read(fd, session->buffer, size);
            if (readbytes <= 0)
                /* no more data */
                break;
            rt_thddog_feed("webnet_module_system_dofile");
            if (webnet_session_write(session, session->buffer, readbytes) == 0) break;

            file_length -= (long)readbytes;
        }
    }

    /* close file */
    close(fd);

    return WEBNET_MODULE_FINISHED;
}

static int _webnet_module_system_uri_post(struct webnet_session *session, int event)
{
    int result;
    result = WEBNET_MODULE_CONTINUE;

#ifdef WEBNET_USING_LOG
    webnet_module_log(session, event);
#endif

#ifdef WEBNET_USING_ASP
    result = webnet_module_asp(session, event);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#ifdef WEBNET_USING_INDEX
    result = webnet_module_dirindex(session, event);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

    result = webnet_module_system_dofile(session);
    if (result == WEBNET_MODULE_FINISHED) return result;

    return WEBNET_MODULE_CONTINUE;
}

static int _webnet_module_system_response_header(struct webnet_session *session, int event)
{
    int result;
    result = WEBNET_MODULE_CONTINUE;

    return result;
}

static int _webnet_module_system_response_file(struct webnet_session *session, int event)
{
    int result;
    result = WEBNET_MODULE_CONTINUE;

    return result;
}

int webnet_module_handle_event(struct webnet_session *session, int event)
{
    switch (event) {
    case WEBNET_EVENT_INIT:
        return _webnet_module_system_init(session, event);
    case WEBNET_EVENT_URI_PHYSICAL:
        return _webnet_module_system_uri_physical(session, event);
    case WEBNET_EVENT_URI_POST:
        return _webnet_module_system_uri_post(session, event);
    case WEBNET_EVENT_RSP_HEADER:
        return _webnet_module_system_response_header(session, event);
    case WEBNET_EVENT_RSP_FILE:
        return _webnet_module_system_response_file(session, event);
    default:
        RT_ASSERT(0);
        break;
    }

    return WEBNET_MODULE_CONTINUE;
}

/* default index file */
static const char *default_files[] =
{
    "/index.html",
    RT_NULL
};

/**
 * handle uri
 * there are two phases on uri handling:
 * - map url to physical
 * - url handling
 */
int webnet_module_handle_uri(struct webnet_session *session)
{
    int result, fd;
    char *full_path = RT_NULL;
    rt_uint32_t index;
    struct webnet_request *request;

    RT_ASSERT(session != RT_NULL);
    /* get request */
    request = session->request;
    RT_ASSERT(request != RT_NULL);

    /* map uri to physical */
    result = webnet_module_handle_event(session, WEBNET_EVENT_URI_PHYSICAL);
    if (result == WEBNET_MODULE_FINISHED) return result;

    /* made a full physical path */
    full_path = (char *)WEBNET_CALLOC(1,WEBNET_PATH_MAX + 1);
    RT_ASSERT(full_path != RT_NULL);

#define WEBNET_DOWNLOAD "/download/"
    rt_bool_t download = RT_FALSE;
    if (str_begin_with(request->path, WEBNET_DOWNLOAD)) {
        /* use download full path */
        rt_snprintf(full_path, WEBNET_PATH_MAX, "/%s", &request->path[strlen(WEBNET_DOWNLOAD)]);
        /* normalize path */
        str_normalize_path(full_path);
        download = RT_TRUE;
    } else {

        index = 0;
        while (default_files[index] != RT_NULL) {
            /* made a full path */
            rt_snprintf(full_path, WEBNET_PATH_MAX, "%s/%s%s",
                        WEBNET_ROOT, request->path, default_files[index]);
            /* normalize path */
            str_normalize_path(full_path);

            fd = open(full_path, O_RDONLY, 0);
            if (fd >= 0) {
                /* close file descriptor */
                close(fd);
                break;
            }

            index++;
        }
        /* no this file */
        if (default_files[index] == RT_NULL) {
            /* use old full path */
            rt_snprintf(full_path, WEBNET_PATH_MAX, "%s/%s", WEBNET_ROOT, request->path);
            /* normalize path */
            str_normalize_path(full_path);
        }
    }

    /* mark path as full physical path */
    if (request->field_copied) {
        WEBNET_FREE(request->path);
    }
    request->path = full_path;

    /* check uri valid */
    if (!download && !str_begin_with(request->path, WEBNET_ROOT)) {
        /* not found */
        request->result_code = 404;
        return WEBNET_MODULE_FINISHED;
    }

    /* uri post handle */
    return webnet_module_handle_event(session, WEBNET_EVENT_URI_POST);
}
