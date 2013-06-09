/*
 * Copyright (C) 2013 Bartosz Golaszewski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**************************************
 * Busybus public API.
 **************************************/

#ifndef __BUSYBUS__
#define __BUSYBUS__

#include <stdlib.h>
#include <stdarg.h>

/**************************************
 * Common funcs and macros.
 **************************************/

/* Export symbol. */
#define BBUS_PUBLIC __attribute__((visibility("default")))
/* Printf-like function. */
#define BBUS_PRINTF_FUNC(FORMAT, PARAMS)				\
		__attribute__((format(printf, FORMAT, PARAMS)))

void* bbus_malloc(size_t size);
void* bbus_realloc(void* ptr, size_t size);
void bbus_free(void* ptr);

struct bbus_timeval
{
	long int sec;
	long int usec;
};

/**************************************
 * Error handling.
 **************************************/

/* Busybus error codes. */
#define BBUS_SUCCESS		10000
#define BBUS_NOMEM		10001
#define __BBUS_MAX_ERR		10002

/* Returns the value of the last error in the busybus library. */
int bbus_get_last_error(void) BBUS_PUBLIC;
/* Returns a string representation of the error code passed in 'errnum'. */
const char* bbus_error_str(int errnum) BBUS_PUBLIC;

/**************************************
 * Data marshalling.
 **************************************/

typedef struct __bbus_object bbus_object;

#define BBUS_TYPE_INT		'i'
#define BBUS_TYPE_UNSIGNED	'u'
#define BBUS_TYPE_BYTE		'b'
#define BBUS_TYPE_STRING	's'

#define BBUS_TYPES		'iubs'

bbus_object* bbus_make_object(const char* fmt, ...) BBUS_PUBLIC;
int bbus_parse_object(bbus_object* obj, const char* fmt, ...) BBUS_PUBLIC;
void bbus_free_object(bbus_object* obj) BBUS_PUBLIC;

/**************************************
 * Protocol funcs and defs.
 **************************************/

#define BBUS_DEF_SOCKPATH	"/var/run/bbus.sock"

struct bbus_msg_hdr
{

};

/**************************************
 * Method calling client.
 **************************************/

typedef struct __bbus_client_connection bbus_client_connection;

bbus_client_connection* bbus_client_connect(void) BBUS_PUBLIC;
bbus_client_connection* bbus_client_connect_wpath(
		const char* path) BBUS_PUBLIC;
bbus_object* bbus_call_method(bbus_client_connection* conn,
		const char* method, bbus_object* arg) BBUS_PUBLIC;
int bbus_close_client_conn(bbus_client_connection* conn) BBUS_PUBLIC;

/**************************************
 * Service publishing client.
 **************************************/

typedef struct __bbus_service_connection bbus_service_connection;
typedef bbus_object* (*bbus_method_func)(bbus_object*);

struct bbus_method
{
    char* name;
    char* argdscr;
    char* retdscr;
    bbus_method_func func;
};

bbus_service_connection* bbus_service_connect(void) BBUS_PUBLIC;
bbus_service_connection* bbus_service_connect_wpath(
		const char* path) BBUS_PUBLIC;
bbus_object* bbus_register_method(bbus_service_connection* conn,
		struct bbus_method* method) BBUS_PUBLIC;
int bbus_close_service_conn(bbus_service_connection* conn) BBUS_PUBLIC;
int bbus_listen_method_calls(bbus_service_connection* conn,
		struct bbus_timeval* tv) BBUS_PUBLIC;

/**************************************
 * Server stuff.
 **************************************/

/* Opaque server object. */
typedef struct __bbus_server bbus_server;
/*
 * Opaque object corresponding with a single connected client (can
 * be both a method-calling client and a service offering client).
 */
typedef struct __bbus_client bbus_client;

#define BBUS_CLIENT_CALLER	1
#define BBUS_CLIENT_SERVICE	2
#define BBUS_CLIENT_MON		3
#define BBUS_CLIENT_CTL		4

int bbus_get_client_type(bbus_client* cli) BBUS_PUBLIC;

/*
 * Client list element. Manipulated internally by insque and remque
 * functions from glibc.
 */
struct bbus_clientlist_elem
{
	struct bbus_clientlist_elem* next;
	struct bbus_clientlist_elem* prev;
	bbus_client* cli;
};

void bbus_clientlist_insert(struct bbus_clientlist_elem* cli,
		struct bbus_clientlist_elem* prev) BBUS_PUBLIC;
void bbus_clientlist_remove(struct bbus_clientlist_elem* cli) BBUS_PUBLIC;

bbus_server* bbus_make_local_server(void) BBUS_PUBLIC;
bbus_server* bbus_make_local_server_wpath(const char* path) BBUS_PUBLIC;
int bbus_server_listen(bbus_server* srv) BBUS_PUBLIC;
bbus_client* bbus_accept_client(bbus_server* srv) BBUS_PUBLIC;
int bbus_server_close(bbus_server* srv) BBUS_PUBLIC;
void bbus_free_server(bbus_server* srv) BBUS_PUBLIC;

typedef struct __bbus_pollset bbus_pollset;

bbus_pollset* bbus_make_pollset(void);
bbus_free_pollset(bbus_pollset* pollset);

#endif /* __BUSYBUS__ */

