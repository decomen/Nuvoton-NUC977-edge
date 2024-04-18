
#include "request.h"
#include "session.h"
#include "mimetype.h"
#include "util.h"
#include "module.h"
#include <stdio.h>

#include "string.h"
#include "das_os.h"
#include "das_util.h"

#define DEFAULT_AUTH_BIN        "/auth-bin/"
#define LOGIN_PATH              "/login.html"
#define REG_PATH                "/reg.html"

static char s_auth_user[32];
static char s_auth_psk[128];

struct auth_info
{
    rt_bool_t login;
	struct in_addr addr;
	rt_tick_t last_tick;
};

struct auth_info s_auth_info[WEBNET_MAX_USER];

static void _auth_all_logout( void )
{
    for( int i = 0; i < WEBNET_MAX_USER; i++ ) {
        s_auth_info[i].login = RT_FALSE;
    }
}

static int _auth_find( struct webnet_session* session )
{
    for( int i = 0; i < WEBNET_MAX_USER; i++ ) {
        if( s_auth_info[i].addr.s_addr == session->cliaddr.sin_addr.s_addr ) {
            return i;
        }
    }

    for( int i = 0; i < WEBNET_MAX_USER; i++ ) {
        if( !s_auth_info[i].login ) {
            return i;
        }
    }

    return -1;
}


rt_bool_t webnet_auth_is_login( struct webnet_session* session )
{
    int n = _auth_find( session );
    if(n >= 0 ) {
        if( s_auth_info[n].login ) {
            if( rt_tick_get() - s_auth_info[n].last_tick >= rt_tick_from_millisecond( 5 * 60 * 1000 ) ) {
                s_auth_info[n].login = RT_FALSE;
            } else {
                s_auth_info[n].last_tick = rt_tick_get();
                return RT_TRUE;
            }
        }
    }

    return RT_FALSE;
}


static int _webnet_module_auth_init(struct webnet_session* session, int event)
{
    _auth_all_logout();
    
    return WEBNET_MODULE_CONTINUE;
}

static void _auth_login( struct webnet_session* session)
{
    const char *szUser = webnet_request_get_query( session->request, "user" );
    const char *szPsk = webnet_request_get_query( session->request, "psk" );

    int n = _auth_find( session );
    if( n >= 0 ) {
        s_auth_info[n].login = RT_FALSE;
        if( !szUser || strcasecmp( szUser, s_auth_user ) != 0 ) {
            webnet_session_printf( session, "{\"ret\":1,\"msg\":\"Invalid username!\"}" );
        } else if( !szPsk || strcmp( szPsk, s_auth_psk ) != 0 ) {
            webnet_session_printf( session, "{\"ret\":1,\"msg\":\"Invalid password!\"}" );
        } else {
            webnet_session_printf( session, "{\"ret\":0,\"msg\":\"OK\"}" );
            s_auth_info[n].login = RT_TRUE;
            s_auth_info[n].addr = session->cliaddr.sin_addr;
            s_auth_info[n].last_tick = rt_tick_get();
        }
    } else {
        webnet_session_printf( session, "{\"ret\":1,\"msg\":\"Too many users online!\"}" );
    }
}

static void _auth_logout( struct webnet_session* session )
{
    int n = _auth_find( session );
    if( n >= 0 ) {
        s_auth_info[n].login = RT_FALSE;
    }
    webnet_auth_open_login( session );
}

static int _webnet_module_auth_uri_physical(struct webnet_session* session, int event)
{
    if( session->request && session->request->path ) {
        if( str_begin_with( session->request->path, DEFAULT_AUTH_BIN ) ) {
            webnet_session_set_header( session, "text/json", 200, "OK", -1 );
            char *name = session->request->path + strlen(DEFAULT_AUTH_BIN);
            if( name && strcasecmp( name, "login" ) == 0 ) {
    			_auth_login( session );
            } else if( name && strcasecmp( name, "logout" ) == 0 ) {
                _auth_logout( session );
            }
            return WEBNET_MODULE_FINISHED;
        } else if( 
            0 == strcmp( session->request->path, "/" ) ||
            str_end_with( session->request->path, ".html" ) || 
            str_end_with( session->request->path, ".htm" ) || 
            str_end_with( session->request->path, ".asp" ) ) 
        {
            if( !webnet_auth_is_login( session ) && !str_begin_with( session->request->path, LOGIN_PATH ) ) {
                webnet_auth_open_login( session );
                return WEBNET_MODULE_FINISHED;
            } else if( webnet_auth_is_login( session ) && !g_isreg && reg_testover() && !str_begin_with( session->request->path, REG_PATH ) ) {
                webnet_open_reg( session );
                return WEBNET_MODULE_FINISHED;
            }
        }
    }

    return WEBNET_MODULE_CONTINUE;
}

int webnet_module_auth(struct webnet_session* session, int event)
{
	switch (event)
	{
	case WEBNET_EVENT_INIT:
		return _webnet_module_auth_init(session, event);
	case WEBNET_EVENT_URI_PHYSICAL:
		return _webnet_module_auth_uri_physical(session, event);
	case WEBNET_EVENT_URI_POST:
	case WEBNET_EVENT_RSP_HEADER:
	case WEBNET_EVENT_RSP_FILE:
	    return WEBNET_MODULE_CONTINUE;
	default:
		RT_ASSERT(0);
		break;
	}

	return WEBNET_MODULE_CONTINUE;
}

void webnet_auth_open_login( struct webnet_session* session )
{
    session->request->result_code = 200;
    webnet_session_printf( session, "<html> <head><title>login</title><meta http-equiv=\"Refresh\" content=\"1;url=." LOGIN_PATH "\"> </head> <body> login ... </body> </html>" );
}

void webnet_open_reg( struct webnet_session* session )
{
    session->request->result_code = 200;
    webnet_session_printf( session, "<html> <head><title>reg</title><meta http-equiv=\"Refresh\" content=\"1;url=." REG_PATH "\"> </head> <body> jump reg page ... </body> </html>" );
}

void webnet_auth_set(const char* user, const char* psk)
{
    if( user && 0 == strcmp( user, s_auth_user ) &&
        psk  && 0 == strcmp( psk, s_auth_psk ) ) {
        ; // do nothing
    } else {
        if( user && strlen(user) < sizeof(s_auth_user) ) {
            strcpy( s_auth_user, user );
        }
        if( psk && strlen(psk) < sizeof(s_auth_psk) ) {
            strcpy( s_auth_psk, psk );
        }

        _auth_all_logout();
    }
}

