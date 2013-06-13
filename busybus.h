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

/* Busybus malloc. Returns a valid pointer even for size == 0. */
void* bbus_malloc(size_t size);
/*
 * Busybus realloc. Returns a valid pointer for size == 0,
 * for ptr == NULL behaves like bbus_malloc.
 */
void* bbus_realloc(void* ptr, size_t size);
/*
 * Busybus free. Does nothing if ptr == NULL. It's not safe to use
 * bbus_free on memory allocated by regular malloc and vice-versa.
 */
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
#define BBUS_INVALARG		10002
#define BBUS_OBJINVOP		10003
#define BBUS_OBJINVFMT		10004
#define BBUS_NOSPACE		10005
#define __BBUS_MAX_ERR		10006

/* Returns the value of the last error in the busybus library. */
int bbus_get_last_error(void) BBUS_PUBLIC;
/* Returns a string representation of the error code passed in 'errnum'. */
const char* bbus_error_str(int errnum) BBUS_PUBLIC;

/**************************************
 * Data marshalling.
 **************************************/

#define BBUS_TYPE_INT		'i'
#define BBUS_TYPE_UNSIGNED	'u'
#define BBUS_TYPE_BYTE		'b'
#define BBUS_TYPE_STRING	's'

typedef int32_t bbus_int;
typedef uint32_t bbus_unsigned;
typedef uint8_t bbus_byte;

#define BBUS_TYPES		'iubs'

typedef struct __bbus_object bbus_object;

#define BBUS_OBJ_EMPTY		1
#define BBUS_OBJ_INSERTING	2
#define BBUS_OBJ_READY		3
#define BBUS_OBJ_EXTRACTING	4

bbus_object* bbus_empty_object(void) BBUS_PUBLIC;
int bbus_obj_setdescr(bbus_object* obj, const char* descr) BBUS_PUBLIC;
int bbus_obj_insert_int(bbus_object* obj, bbus_int val) BBUS_PUBLIC;
int bbus_obj_insert_unsigned(bbus_object* obj, bbus_unsigned val) BBUS_PUBLIC;
int bbus_obj_insert_string(bbus_object* obj, uint8_t* val) BBUS_PUBLIC;
int bbus_obj_extract_int(bbus_object* obj, bbus_int* val) BBUS_PUBLIC;
int bbus_obj_extract_unsigned(bbus_object* obj,
		bbus_unsigned* val) BBUS_PUBLIC;
int bbus_obj_extract_string(bbus_object* obj, uint8_t** val) BBUS_PUBLIC;
void bbus_obj_reset(bbus_object* obj) BBUS_PUBLIC;
int bbus_obj_getstate(bbus_object* obj) BBUS_PUBLIC;
bbus_object* bbus_make_object(const char* descr, ...) BBUS_PUBLIC;
bbus_object* bbus_object_from_buf(const void* buf, size_t bufsize) BBUS_PUBLIC;
ssize_t bbus_object_to_buf(bbus_object* obj, void* buf,
		size_t bufsize) BBUS_PUBLIC;
int bbus_parse_object(bbus_object* obj, const char* descr, ...) BBUS_PUBLIC;
void bbus_free_object(bbus_object* obj) BBUS_PUBLIC;

/**************************************
 * Protocol funcs and defs.
 **************************************/

#define BBUS_MAGIC		0xBBC5
#define BBUS_DEF_DIRPATH	"/var/run/bbus/"
#define BBUS_DEF_SOCKPATH	"/var/run/bbus/bbus.sock"

#define BBUS_MSGTYPE_SO		0x01	/* Session open */
#define BBUS_MSGTYPE_SOOK	0x02	/* Session open confirmed */
#define BBUS_MSGTYPE_SORJCT	0x03	/* Session open rejected */
#define BBUS_MSGTYPE_SRVREG	0x04	/* Register service */
#define BBUS_MSGTYPE_SRVACK	0x05	/* Service acknowledged */
#define BBUS_MSGTYPE_CLICALL	0x06	/* Client calls a method */
#define BBUS_MSGTYPE_CLIREPLY	0x07	/* Server replies to a client */
#define BBUS_MSGTYPE_SRVCALL	0x08	/* Server calls a registered method */
#define BBUS_MSGTYPE_SRVREPLY	0x09	/* Method provider replies */
#define BBUS_MSGTYPE_ERROR	0x0A	/* Erroneous call */
#define BBUS_MSGTYPE_CLOSE	0x0B	/* Client closes session */

struct bbus_msg_hdr
{
	uint16_t magic;
	uint8_t msgtype;
	uint8_t errcode;
	uint32_t size;
	uint8_t payload[1];
};

#define BBUS_MSGHDR_SIZE	(sizeof(struct bbus_msg_hdr)-1)

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

