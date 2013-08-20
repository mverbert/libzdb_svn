/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#include "Config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "URL.h"


/**
 * Implementation of the URL interface. The scanner handle 
 * ISO Latin 1 or UTF-8 encoded url's transparently. 
 *
 * @file
 */


/* ----------------------------------------------------------- Definitions */

typedef struct param_t {
        char *name;
        char *value;
        struct param_t *next;
} *param_t;

#define T URL_T
struct URL_S {
	int port;
       	char *ref;
	char *path;
	char *host;
	char *user;
        char *qptr;
	char *query;
	char *portStr;
	char *protocol;
	char *password;
	char *toString;
        param_t params;
        char **paramNames;
	uchar_t *data;
	uchar_t *buffer;
	uchar_t *marker, *ctx, *limit, *token;
        /* Keep the above align with zild URL_T */
};

/* Unsafe URL characters: [00-1F, 7F-FF] <>\"#%}{|\\^[] ` */
static const uchar_t urlunsafe[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

#define UNKNOWN_PORT -1
#define YYCTYPE       uchar_t
#define YYCURSOR      U->buffer  
#define YYLIMIT       U->limit  
#define YYMARKER      U->marker
#define YYCTXMARKER   U->ctx
#define YYFILL(n)     ((void)0)
#define YYTOKEN       U->token
#define SET_PROTOCOL(PORT) *(YYCURSOR-3)=0; U->protocol=U->token; U->port=PORT; goto authority


/* ------------------------------------------------------- Private methods */


static int parseURL(T U) {
        param_t param = NULL;
	/*!re2c
	ws		= [ \t\r\n];
	any		= [\000-\377];
	protocol        = [a-zA-Z0-9]+"://";
	auth            = ([\040-\377]\[@])+[@];
	host            = ([a-zA-Z0-9\-]+)([.]([a-zA-Z0-9\-]+))*;
	port            = [:][0-9]+;
	path            = [/]([\041-\377]\[?#;])*;
	query           = ([\040-\377]\[#])*;
	parameterkey    = ([\041-\377]\[=])+;
	parametervalue  = ([\040-\377]\[&])*;
	*/
proto:
	if (YYCURSOR >= YYLIMIT)
		return false;
	YYTOKEN = YYCURSOR;
	/*!re2c

        ws         {
                        goto proto;
		   }

        "mysql://" {
                      	SET_PROTOCOL(MYSQL_DEFAULT_PORT);
                   }
                   
        "postgresql://" {
                      	SET_PROTOCOL(POSTGRESQL_DEFAULT_PORT);
                   }

        "oracle://" {
                      	SET_PROTOCOL(ORACLE_DEFAULT_PORT);
                   }

        protocol   {
                      	SET_PROTOCOL(UNKNOWN_PORT);
                   }
    
        any        {
                      	goto proto;
                   }
	*/
authority:
	if (YYCURSOR >= YYLIMIT)
		return true;
	YYTOKEN = YYCURSOR;
	/*!re2c
    
        ws         { 
                        goto authority; 
                   }

        auth       {
                        *(YYCURSOR - 1) = 0;
                        U->user = YYTOKEN;
                        char *p = strchr(U->user, ':');
                        if (p) {
                                *(p++) = 0;
                                U->password = URL_unescape(p);
                        }
                        URL_unescape(U->user);
                        goto authority; 
                   }

        host       {
                        U->host = Str_ndup(YYTOKEN, (int)(YYCURSOR - YYTOKEN));
                        goto authority; 
                   }

        port       {
                        U->port = Str_parseInt(YYTOKEN + 1); // read past ':'
                        goto authority; 
                   }

        path       {
                        *YYCURSOR = 0;
                        U->path = URL_unescape(YYTOKEN);
                        return true;
                   }
                   
        path[?]    {
                        *(YYCURSOR-1) = 0;
                        U->path = URL_unescape(YYTOKEN);
                        goto query; 
                   }
                   
       any         {
                      	return true;
                   }
                   
	*/
query:
        if (YYCURSOR >= YYLIMIT)
		return true;
	YYTOKEN =  YYCURSOR;
	/*!re2c

        query      {
                        *YYCURSOR = 0;
                        U->query = Str_ndup(YYTOKEN, (int)(YYCURSOR - YYTOKEN));
                        YYCURSOR = YYTOKEN; // backtrack to start of query string after terminating it and
                        goto params;
                   }

        any        { 
                      return true;     
                   }
		   
	*/
params:
	if (YYCURSOR >= YYLIMIT)
		return true;
	YYTOKEN =  YYCURSOR;
	/*!re2c
         
         parameterkey {
                /* No parameters in querystring */
                return true;
        }

        parameterkey/[=] {
                NEW(param);
                param->name = YYTOKEN;
                param->next = U->params;
                U->params = param;
                goto params;
        }

        [=]parametervalue[&]? {
                *YYTOKEN++ = 0;
                if (*(YYCURSOR - 1) == '&')
                        *(YYCURSOR - 1) = 0;
                if (! param) /* format error */
                        return true; 
                param->value = URL_unescape(YYTOKEN);
                goto params;
        }

        any { 
                return true;
        }
        */
        return false;
}


static inline int x2b(uchar_t *x) {
	register int b;
	b = ((x[0] >= 'A') ? ((x[0] & 0xdf) - 'A')+10 : (x[0] - '0'));
	b *= 16;
	b += (x[1] >= 'A' ? ((x[1] & 0xdf) - 'A')+10 : (x[1] - '0'));
	return b;
}


static inline uchar_t *b2x(uchar_t b, uchar_t *x) {
        static const char b2x_table[] = "0123456789ABCDEF";
        *x++ = '%';
        *x++ = b2x_table[b >> 4];
        *x = b2x_table[b & 0xf];
        return x;
}


static void freeParams(param_t p) {
        for (param_t q = NULL; p; p = q) {
                q = p->next;
                FREE(p);
        }
}


static T ctor(uchar_t *data) {
        T U;
	NEW(U);
	U->data = data;
	YYCURSOR = U->data;
	U->port = UNKNOWN_PORT;
	YYLIMIT = U->data + strlen(U->data);
	if (! parseURL(U))
                URL_free(&U);
	return U;
}


/* -------------------------------------------------------- Public methods */


T URL_new(const char *url) {
        if (STR_UNDEF(url))
                return NULL;
        Exception_init();
        return ctor((uchar_t*)Str_dup(url));
}


T URL_create(const char *url, ...) {
        if (STR_UNDEF(url))
                return NULL;
        Exception_init();
	va_list ap;
        va_start(ap, url);
	T U = ctor((uchar_t*)Str_vcat(url, ap));
  	va_end(ap);
        return U;
}

void URL_free(T *U) {
	assert(U && *U);
        freeParams((*U)->params);
        FREE((*U)->paramNames);
	FREE((*U)->toString);
	FREE((*U)->query);
	FREE((*U)->data);
	FREE((*U)->host);
	FREE(*U);
}


/* ------------------------------------------------------------ Properties */


const char *URL_getProtocol(T U) {
	assert(U);
	return U->protocol;
}


const char *URL_getUser(T U) {
	assert(U);
	return U->user;
}


const char *URL_getPassword(T U) {
	assert(U);
	return U->password;
}


const char *URL_getHost(T U) {
	assert(U);
	return U->host;
}


int URL_getPort(T U) {
	assert(U);
	return U->port;
}


const char *URL_getPath(T U) {
	assert(U);
	return U->path;
}


const char *URL_getQueryString(T U) {
	assert(U);
	return U->query;
}


const char **URL_getParameterNames(T U) {
        assert(U);
        if (U->params && (U->paramNames == NULL)) {
                param_t p;
                int i = 0, len = 0;
                for (p = U->params; p; p = p->next) len++;
                U->paramNames = ALLOC((len + 1) * sizeof *(U->paramNames));
                for (p = U->params; p; p = p->next)
                        U->paramNames[i++] = p->name;
                U->paramNames[i] = NULL;
        }
	return (const char **)U->paramNames;
}


const char *URL_getParameter(T U, const char *name) {
	assert(U);
        assert(name);
        for (param_t p = U->params; p; p = p->next) {
                if (Str_isByteEqual(p->name, name))
                        return p->value;
        }
        return NULL;
}


/* ---------------------------------------------------------------- Public */


const char *URL_toString(T U) {
	assert(U);
	if (! U->toString) {
                uchar_t port[7] = {0};
                if (U->port >= 0)
                        snprintf(port, 6, ":%d", U->port);
		U->toString = Str_cat("%s://%s%s%s%s%s%s%s%s%s", 
                                      U->protocol,
                                      U->user ? U->user : "",
                                      U->password ? ":" : "",
                                      U->password ? U->password : "",
                                      U->user ? "@" : "",
                                      U->host ? U->host : "",
                                      port,
                                      U->path ? U->path : "",
                                      U->query ? "?" : "",
                                      U->query ? U->query : ""); 
	}
	return U->toString;
}


/* --------------------------------------------------------- Class methods */


char *URL_unescape(char *url) {
	if (STR_DEF(url)) {
                register int x, y;
                for (x = 0, y = 0; url[y]; x++, y++) {
                        if ((url[x] = url[y]) == '+')
                                url[x] = ' ';
                        else if (url[x] == '%') {
                                if (! (url[x + 1] && url[x + 2]))
                                        break;
                                url[x] = x2b(url + y + 1);
                                y += 2;
                        }
                }
                url[x] = 0;
        }
	return url;
}


char *URL_escape(const char *url) {
        char *escaped = 0;
        if (url) {
                char *p;
                int i, n;
                for (n = i = 0; url[i]; i++) 
                        if (urlunsafe[(unsigned char)(url[i])]) 
                                n += 2;
                p = escaped = ALLOC(i + n + 1);
                for (; *url; url++, p++) {
                        if (urlunsafe[(unsigned char)(*p = *url)])
                                p = b2x(*url, p);
                }
                *p = 0;
        }
        return escaped;
}


char *URL_normalize(char *path) {
	if (path) {
                char c;
                int i,j;
                for (i = j = 0; (c = path[i]); ++i) {
                        if (c == '/') {
                                while (path[i+1] == '/') ++i;	
                        } else if (c == '.' && j && path[j-1] == '/') {
                                if (path[i+1] == '.' && (path[i+2] == '/' || path[i+2] == 0)) {
                                        if (j>1)
                                        for (j -= 2; path[j] != '/' && j > 0; --j);
                                        i += 2;
                                } else if (path[i+1] == '/' || path[i+1] == 0) {
                                        ++i;
                                        continue;
                                }
                        }
                        if (! (path[j] = path[i])) break; ++j;
                }
                if (! j) { path[0] = '/'; j = 1; }
                path[j] = 0;
                if (path[0] == '/' && path[1] == '/') {
                        for (i = j = 0; (c = path[i]); ++i) {
                                if (c == '/') {
                                        while (path[i+1] == '/') ++i;	
                                }
                                if (! (path[j] = path[i])) break; 
                                ++j;
                        }
                        path[j] = 0;
                }
        }
	return path;
}
