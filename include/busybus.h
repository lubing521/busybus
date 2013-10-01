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
#include <stdint.h>

/**************************************
 * Common funcs and macros.
 **************************************/

/* Export symbol. */
#define BBUS_PUBLIC __attribute__((visibility("default")))
/* Printf-like function. */
#define BBUS_PRINTF_FUNC(FORMAT, PARAMS)				\
		__attribute__((format(printf, FORMAT, PARAMS)))
/* Unused parameter. */
#define BBUS_UNUSED __attribute__((unused))

/* Busybus malloc. Returns a valid pointer even for size == 0. */
void* bbus_malloc(size_t size) BBUS_PUBLIC;
/* Just like bbus_malloc, but zeroes allocated memory. */
void* bbus_malloc0(size_t size) BBUS_PUBLIC;
/*
 * Busybus realloc. Returns a valid pointer for size == 0,
 * for ptr == NULL behaves like bbus_malloc.
 */
void* bbus_realloc(void* ptr, size_t size) BBUS_PUBLIC;
/*
 * Busybus free. Does nothing if ptr == NULL. It's not safe to use
 * bbus_free on memory allocated by regular malloc and vice-versa.
 */
void bbus_free(void* ptr) BBUS_PUBLIC;
/*
 * Duplicates the memory area pointed to by src in a newly allocated
 * buffer created using bbus_malloc. Use bbus_free to free the memory.
 */
void* bbus_memdup(const void* src, size_t size) BBUS_PUBLIC;

struct bbus_timeval
{
	long int sec;
	long int usec;
};

char* bbus_build_string(const char* fmt, ...)
		BBUS_PRINTF_FUNC(1, 2) BBUS_PUBLIC;
char* bbus_copy_string(const char* str) BBUS_PUBLIC;
void bbus_free_string(char* str) BBUS_PUBLIC;

uint32_t bbus_crc32(const void* buf, size_t bufsize) BBUS_PUBLIC;

typedef struct __bbus_hashmap bbus_hashmap;

bbus_hashmap* bbus_hmap_create(void) BBUS_PUBLIC;
int bbus_hmap_insert(bbus_hashmap* hmap, const void* key,
		size_t ksize, void* val) BBUS_PUBLIC;
int bbus_hmap_insert_str(bbus_hashmap* hmap,
		const char* key, void* val) BBUS_PUBLIC;
void* bbus_hmap_find(bbus_hashmap* hmap, const void* key,
		size_t ksize) BBUS_PUBLIC;
void* bbus_hmap_find_str(bbus_hashmap* hmap, const char* key) BBUS_PUBLIC;
void* bbus_hmap_remove(bbus_hashmap* hmap, const void* key,
		size_t ksize) BBUS_PUBLIC;
void* bbus_hmap_remove_str(bbus_hashmap* hmap, const char* key) BBUS_PUBLIC;
void bbus_hmap_reset(bbus_hashmap* hmap) BBUS_PUBLIC;
void bbus_hmap_free(bbus_hashmap* hmap) BBUS_PUBLIC;
int bbus_hmap_dump(bbus_hashmap* hmap, char* buf, size_t bufsize) BBUS_PUBLIC;

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
#define BBUS_CONNCLOSED		10006
#define BBUS_MSGINVFMT		10007
#define BBUS_MSGMAGIC		10008
#define BBUS_MSGINVTYPERCVD	10009
#define BBUS_SORJCTD		10010
#define BBUS_SENTLESS		10011
#define BBUS_RCVDLESS		10012
#define BBUS_LOGICERR		10013
#define BBUS_NOMETHOD		10014
#define BBUS_HMAPNOELEM		10015
#define BBUS_METHODERR		10016
#define __BBUS_MAX_ERR		10017

/* Returns the value of the last error in the busybus library. */
int bbus_get_last_error(void) BBUS_PUBLIC;
/* Returns a string representation of the error code passed in 'errnum'. */
const char* bbus_error_str(int errnum) BBUS_PUBLIC;

/**************************************
 * Data marshalling.
 **************************************/

/* TODO This will be extended - more types + arrays and structs. */

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
const char* bbus_obj_getdescr(bbus_object* obj) BBUS_PUBLIC;
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
bbus_object* bbus_make_object_v(const char* descr, va_list va) BBUS_PUBLIC;
bbus_object* bbus_object_from_buf(const void* buf, size_t bufsize) BBUS_PUBLIC;
ssize_t bbus_object_to_buf(bbus_object* obj, void* buf,
		size_t bufsize) BBUS_PUBLIC;
int bbus_parse_object(bbus_object* obj, const char* descr, ...) BBUS_PUBLIC;
int bbus_parse_object_v(bbus_object* obj, const char* descr,
		va_list va) BBUS_PUBLIC;
void* bbus_obj_rawdata(bbus_object* obj) BBUS_PUBLIC;
size_t bbus_obj_rawdata_size(const bbus_object* obj) BBUS_PUBLIC;
void bbus_free_object(bbus_object* obj) BBUS_PUBLIC;
int bbus_obj_repr(bbus_object* obj, char* buf, size_t buflen) BBUS_PUBLIC;

/**************************************
 * Protocol funcs and defs.
 **************************************/

 /* TODO Signals will be added in the future. */

#define BBUS_MAGIC		"\xBB\xC5"
#define BBUS_MAGIC_SIZE		2
#define BBUS_DEF_DIRPATH	"/var/run/bbus/"
#define BBUS_DEF_SOCKNAME	"bbus.sock"
#define BBUS_MAXMSGSIZE		4096

#define BBUS_MSGTYPE_SOCLI	0x01	/* Session open client */
#define BBUS_MSGTYPE_SOSRVP	0x02	/* Session open service provider */
#define BBUS_MSGTYPE_SOOK	0x03	/* Session open confirmed */
#define BBUS_MSGTYPE_SORJCT	0x04	/* Session open rejected */
#define BBUS_MSGTYPE_SRVREG	0x05	/* Register service */
#define BBUS_MSGTYPE_SRVUNREG	0x06	/* Unregister service */
#define BBUS_MSGTYPE_SRVACK	0x07	/* Service registered (or error) */
#define BBUS_MSGTYPE_CLICALL	0x08	/* Client calls a method */
#define BBUS_MSGTYPE_CLIREPLY	0x09	/* Server replies to a client */
#define BBUS_MSGTYPE_SRVCALL	0x0A	/* Server calls a registered method */
#define BBUS_MSGTYPE_SRVREPLY	0x0B	/* Method provider replies */
#define BBUS_MSGTYPE_CLOSE	0x0C	/* Client closes session */

/* Protocol error codes */
#define BBUS_PROT_GOOD		0x00
#define BBUS_PROT_NOMETHOD	0x01
#define BBUS_PROT_METHODERR	0x02

/* Protocol flags */
#define BBUS_PROT_HASMETA	(1 << 1)
#define BBUS_PROT_HASOBJECT	(1 << 2)

struct bbus_msg_hdr
{
	uint16_t magic;		/* Busybus magic number */
	uint8_t msgtype;	/* Message type */
	uint8_t errcode;	/* Protocol error code */
	uint32_t token;		/* Used only for method calling */
	uint16_t psize;		/* Size of the payload */
	uint8_t flags;		/* Various protocol flags */
	uint8_t padding;
};

struct bbus_msg
{
	struct bbus_msg_hdr hdr;
	char payload[1];
};

#define BBUS_MSGHDR_SIZE	(sizeof(struct bbus_msg_hdr))

/**************************************
 * Method calling client.
 **************************************/

typedef struct __bbus_client_connection bbus_client_connection;

bbus_client_connection* bbus_client_connect(void) BBUS_PUBLIC;
bbus_client_connection* bbus_client_connect_wpath(
		const char* path) BBUS_PUBLIC;
bbus_object* bbus_call_method(bbus_client_connection* conn,
		char* method, bbus_object* arg) BBUS_PUBLIC;
int bbus_close_client_conn(bbus_client_connection* conn) BBUS_PUBLIC;

/**************************************
 * Service publishing client.
 **************************************/

typedef struct __bbus_service_connection bbus_service_connection;
typedef bbus_object* (*bbus_method_func)(const char*, bbus_object*);

struct bbus_method
{
	char* name;
	char* argdscr;
	char* retdscr;
	bbus_method_func func;
};

bbus_service_connection* bbus_service_connect(const char* name) BBUS_PUBLIC;
bbus_service_connection* bbus_service_connect_wpath(
		const char* name, const char* path) BBUS_PUBLIC;
int bbus_register_method(bbus_service_connection* conn,
		struct bbus_method* method) BBUS_PUBLIC;
int bbus_unregister_method(bbus_service_connection* conn,
		const char* method) BBUS_PUBLIC;
int bbus_close_service_conn(bbus_service_connection* conn) BBUS_PUBLIC;
/*
 * Returns 0 if timed out with no method call, -1 in case of an
 * error and 1 if method has been called.
 */
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

uint32_t bbus_client_gettoken(bbus_client* cli) BBUS_PUBLIC;
void bbus_client_settoken(bbus_client* cli, uint32_t tok) BBUS_PUBLIC;
int bbus_get_client_type(bbus_client* cli) BBUS_PUBLIC;

bbus_server* bbus_make_local_server(void) BBUS_PUBLIC;
bbus_server* bbus_make_local_server_wpath(const char* path) BBUS_PUBLIC;
int bbus_server_listen(bbus_server* srv) BBUS_PUBLIC;
int bbus_server_has_pending_clients(bbus_server* srv) BBUS_PUBLIC;
bbus_client* bbus_accept_client(bbus_server* srv) BBUS_PUBLIC;
int bbus_server_close(bbus_server* srv) BBUS_PUBLIC;
void bbus_free_server(bbus_server* srv) BBUS_PUBLIC;

typedef struct __bbus_pollset bbus_pollset;

bbus_pollset* bbus_make_pollset(void) BBUS_PUBLIC;
void bbus_clear_pollset(bbus_pollset* pset) BBUS_PUBLIC;
void bbus_pollset_add_srv(bbus_pollset* pset, bbus_server* src) BBUS_PUBLIC;
void bbus_pollset_add_client(bbus_pollset* pset, bbus_client* cli) BBUS_PUBLIC;
int bbus_poll(bbus_pollset* pset, struct bbus_timeval* tv) BBUS_PUBLIC;
int bbus_pollset_srv_isset(bbus_pollset* pset, bbus_server* srv) BBUS_PUBLIC;
int bbus_pollset_cli_isset(bbus_pollset* pset, bbus_client* cli) BBUS_PUBLIC;
void bbus_free_pollset(bbus_pollset* pset) BBUS_PUBLIC;

#endif /* __BUSYBUS__ */

