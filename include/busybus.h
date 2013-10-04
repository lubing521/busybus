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

/**
 * @mainpage Busybus public API
 *
 * This is the busybus public API documentation.
 *
 * <p>These functions and macros are all that is needed in order to register
 * <p>and call busybus methods, as well as to create bindings in other languages
 * <p>and even to build fully functional clients and servers. In fact bbusd has
 * <p>been built utilising this public API exclusively.
 */

#ifndef __BUSYBUS__
#define __BUSYBUS__

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

/**
 * @defgroup __common__ Common functions and macros
 * @{
 *
 * Commonly used functions handling memory allocation, string
 * manipulation etc, and some utility macros.
 */

/**
 * @brief Makes symbol visible.
 */
#define BBUS_PUBLIC __attribute__((visibility("default")))

/**
 * @brief Marks function as being "printf-like".
 * @param FORMAT Position of the format parameter in the argument list.
 * @param PARAMS Position of the first of the variadic arguments.
 *
 * Makes preprocessor verify that the variadic arguments' types match
 * arguments specified in the format string.
 */
#define BBUS_PRINTF_FUNC(FORMAT, PARAMS)				\
		__attribute__((format(printf, FORMAT, PARAMS)))

/**
 * @brief Marks a function's argument as unused.
 */
#define BBUS_UNUSED __attribute__((unused))

/**
 * @brief Busybus malloc.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory or NULL in case of an error.
 *
 * Returns a valid pointer even for equal to zero.
 */
void* bbus_malloc(size_t size) BBUS_PUBLIC;

/**
 * @brief Works just like bbus_malloc, but zeroes allocated memory.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory or NULL in case of an error.
 */
void* bbus_malloc0(size_t size) BBUS_PUBLIC;

/**
 * @brief Busybus realloc.
 * @param ptr Memory, that needs reallocation
 * @param size Number of bytes to allocate.
 *
 * Returns a valid pointer for size equal to zero, for NULL ptr behaves
 * like bbus_malloc.
 */
void* bbus_realloc(void* ptr, size_t size) BBUS_PUBLIC;

/**
 * @brief Busybus free.
 * @param ptr Memory to be freed.
 *
 * Does nothing if ptr is NULL. It's not safe to use bbus_free on memory
 * allocated by regular malloc and vice-versa.
 */
void bbus_free(void* ptr) BBUS_PUBLIC;

/**
 * @brief Duplicates memory area.
 * @param src Memory to duplicate.
 * @param size Number of bytes to duplicate.
 * @return Pointer to the newly allocated buffer.
 *
 * Allocates new memory using bbus_malloc() - must be freed using bbus_free().
 */
void* bbus_memdup(const void* src, size_t size) BBUS_PUBLIC;

/**
 * @brief Represents an elapsed time.
 */
struct bbus_timeval
{
	long int sec;	/**< Number of seconds. */
	long int usec;	/**< Number of miliseconds. */
};

/**
 * @brief Build a string from given format and arguments.
 * @param fmt Format of the string to be built.
 * @return Pointer to the newly allocated string or NULL if an error occurred.
 *
 * Returned string must be freed using bbus_str_free.
 */
char* bbus_str_build(const char* fmt, ...) BBUS_PUBLIC BBUS_PRINTF_FUNC(1, 2);

/**
 * @brief Copy a string.
 * @param str Source string to be copied.
 * @return Pointer to the newly allocated string or NULL if an error occurred.
 *
 * Works more like strdup, than strcpy as it allocates it's own buffer using
 * bbus_malloc(). Returned string must be freed using bbus_str_free.
 */
char* bbus_str_cpy(const char* str) BBUS_PUBLIC;

/**
 * @brief Frees a string allocated by one of the bbus string functions.
 * @param str Pointer to the string that will be freed.
 */
void bbus_str_free(char* str) BBUS_PUBLIC;

/**
 * @brief Computes crc32 checksum of given data.
 * @param buf Buffer containing the data.
 * @param bufsize Size of the data to be computed.
 * @return Crc32 checksum.
 */
uint32_t bbus_crc32(const void* buf, size_t bufsize) BBUS_PUBLIC;

/**
 * @}
 *
 * @defgroup __hashmap__ Hashmap functions
 * @{
 *
 * Busybus hashmap interface. Both keys and values can be pointers to any data
 * types, hashes are computed directly from memory buffers.
 */

/**
 * @brief Opaque hashmap object. Only accessible through interface functions.
 */
typedef struct __bbus_hashmap bbus_hashmap;

/**
 * @brief Creates an empty hashmap object.
 * @return Pointer to the new hashmap or NULL if no memory.
 */
bbus_hashmap* bbus_hmap_create(void) BBUS_PUBLIC;

/**
 * @brief Inserts an entry or sets a new value for an existing one.
 * @param hmap The hashmap.
 * @param key The key.
 * @param ksize Size of 'key'.
 * @param val New value.
 * @return 0 on success, -1 if no memory.
 */
int bbus_hmap_insert(bbus_hashmap* hmap, const void* key,
		size_t ksize, void* val) BBUS_PUBLIC;

/**
 * @brief Inserts an entry or sets a new value for an existing one.
 * @param hmap The hashmap.
 * @param key The key.
 * @param val New value.
 * @return 0 on success, -1 if no memory.
 *
 * A string is used as key.
 */
int bbus_hmap_inserts(bbus_hashmap* hmap,
		const char* key, void* val) BBUS_PUBLIC;

/**
 * @brief Looks up the value for a given key.
 * @param hmap The hashmap.
 * @param key The key.
 * @param ksize Size of 'key'.
 * @return Pointer to the looked up entry or NULL if not present.
 */
void* bbus_hmap_find(bbus_hashmap* hmap, const void* key,
		size_t ksize) BBUS_PUBLIC;

/**
 * @brief Looks up the value for a given string.
 * @param hmap The hashmap.
 * @param key The key.
 * @return Pointer to the looked up entry or NULL if not present.
 */
void* bbus_hmap_finds(bbus_hashmap* hmap, const char* key) BBUS_PUBLIC;

/**
 * @brief Removes an entry for a given key.
 * @param hmap The hashmap.
 * @param key The key.
 * @param ksize Size of 'key'.
 * @return Pointer to the looked up value or NULL if not present.
 *
 * If value points to dynamically allocated data, it must be freed
 * separately.
 */
void* bbus_hmap_rm(bbus_hashmap* hmap, const void* key,
		size_t ksize) BBUS_PUBLIC;

/**
 * @brief Removes an entry for a given string.
 * @param hmap The hashmap.
 * @param key The key.
 * @return Pointer to the looked up value or NULL if not present.
 *
 * If value points to dynamically allocated data, it must be freed
 * separately.
 */
void* bbus_hmap_rms(bbus_hashmap* hmap, const char* key) BBUS_PUBLIC;

/**
 * @brief Deletes all key-value pairs from the hashmap.
 * @param hmap Hashmap to reset.
 *
 * Hmap remains a valid hashmap object and must be freed.
 */
void bbus_hmap_reset(bbus_hashmap* hmap) BBUS_PUBLIC;

/**
 * @brief Frees a hashmap object.
 * @param hmap Hashmap to free.
 *
 * Does nothing to the value pointers, so it's possible to use
 * statically allocated data structures as hashmap values.
 */
void bbus_hmap_free(bbus_hashmap* hmap) BBUS_PUBLIC;

/**
 * @brief Converts the hashmap's contents into human-readable form.
 * @param hmap Hashmap object to dump.
 * @param buf Buffer to store the data.
 * @param bufsize Size of the buffer.
 * @return 0 if the hashmap has been properly dumped, -1 in case of an error.
 *
 * Buffer must be big enough to store the output, or this function will exit
 * with an error, although even then it's possible to read the content
 * that has been already written to buf.
 */
int bbus_hmap_dump(bbus_hashmap* hmap, char* buf, size_t bufsize) BBUS_PUBLIC;

/**
 * @}
 *
 * @defgroup __error__ Error handling
 * @{
 *
 * Error handling functions and macros.
 *
 * @defgroup __errcodes__ Error codes
 * @{
 *
 * All error codes, that can be set by busybus API functions.
 */

#define BBUS_SUCCESS		10000 /**< No error */
#define BBUS_NOMEM		10001 /**< Out of memory */
#define BBUS_INVALARG		10002 /**< Invalid argument */
#define BBUS_OBJINVOP		10003 /**< Invalid operation on an object */
#define BBUS_OBJINVFMT		10004 /**< Invalid format of an object */
#define BBUS_NOSPACE		10005 /**< No space in buffer */
#define BBUS_CONNCLOSED		10006 /**< Connection closed */
#define BBUS_MSGINVFMT		10007 /**< Invalid message format */
#define BBUS_MSGMAGIC		10008 /**< Invalid magic number in a message */
#define BBUS_MSGINVTYPERCVD	10009 /**< Invalid message type */
#define BBUS_SORJCTD		10010 /**< Session open rejected */
#define BBUS_SENTLESS		10011 /**< Sent less data, than expected */
#define BBUS_RCVDLESS		10012 /**< Received less data, than expected */
#define BBUS_LOGICERR		10013 /**< Logic error */
#define BBUS_NOMETHOD		10014 /**< No method with given name */
#define BBUS_HMAPNOELEM		10015 /**< No such key in the hashmap */
#define BBUS_METHODERR		10016 /**< Error calling method */
#define __BBUS_MAX_ERR		10017 /**< Highest error code */

/**
 * @}
 */

/**
 * @brief Returns the value of the last error in the busybus library.
 * @return Error number.
 *
 * All functions in the busybus public API set the externally invisible error
 * value to indicate the error's cause. This function is thread-safe - it
 * returns the last error in current thread.
 */
int bbus_lasterror(void) BBUS_PUBLIC;

/**
 * @brief Returns a string representation of an error code.
 * @param errnum Error code to interpret.
 * @return Pointer to a human-readable error message.
 *
 * Returns pointer to a static string, which must not be modified. Subsequent
 * calls will not modify the string under this address. If errnum is equal
 * to one of the glibc errnos it will use strerror to obtain the
 * error message.
 */
const char* bbus_strerror(int errnum) BBUS_PUBLIC;

/**
 * @}
 */

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

/**
 * @defgroup __protocol__ Protocol definitions
 * @{
 *
 * Contains data structure definitions and constants used in the busybus
 * message exchange protocol.
 */

/* TODO Signals and simple messages will be added in the future. */

/** @brief Busybus magic number. */
#define BBUS_MAGIC			"\xBB\xC5"
/** @brief Size of the magic number. */
#define BBUS_MAGIC_SIZE			2
/** @brief Unix socket directory. */
#define BBUS_DEF_DIRPATH		"/var/run/bbus/"
/** @brief Unix socket filename. */
#define BBUS_DEF_SOCKNAME 		"bbus.sock"
/** @brief Biggest allowed message size. */
#define BBUS_MAXMSGSIZE			4096

/**
 * @defgroup __protmsgtypes__ Protocol message types
 * @{
 *
 * Message types carried in the 'msgtype' field of the header.
 */
#define BBUS_MSGTYPE_SOCLI	0x01 /**< Session open client. */
#define BBUS_MSGTYPE_SOSRVP	0x02 /**< Session open service provider. */
#define BBUS_MSGTYPE_SOOK	0x03 /**< Session open confirmed. */
#define BBUS_MSGTYPE_SORJCT	0x04 /**< Session open rejected. */
#define BBUS_MSGTYPE_SRVREG	0x05 /**< Register service. */
#define BBUS_MSGTYPE_SRVUNREG	0x06 /**< Unregister service. */
#define BBUS_MSGTYPE_SRVACK	0x07 /**< Service registered (or error). */
#define BBUS_MSGTYPE_CLICALL	0x08 /**< Client calls a method. */
#define BBUS_MSGTYPE_CLIREPLY	0x09 /**< Server replies to a client. */
#define BBUS_MSGTYPE_SRVCALL	0x0A /**< Server calls a registered method. */
#define BBUS_MSGTYPE_SRVREPLY	0x0B /**< Method provider replies. */
#define BBUS_MSGTYPE_CLOSE	0x0C /**< Client closes session. */
/**
 * @}
 *
 * @defgroup __proterrcodes__ Protocol error codes
 * @{
 *
 * Error codes are used to verify, that a method has been properly called.
 * These codes are carried in the 'errcode' field of the header and matter
 * only for replies.
 */
#define BBUS_PROT_GOOD		0x00 /**< Success. */
#define BBUS_PROT_NOMETHOD	0x01 /**< No such method. */
#define BBUS_PROT_METHODERR	0x02 /**< Error calling the method. */
/**
 * @}
 *
 * @defgroup __protflags__ Protocol flags
 * @{
 *
 * Flags, that can be carried by the message header in the 'flags' field.
 */
#define BBUS_PROT_HASMETA	(1 << 1) /**< Message contains metadata. */
#define BBUS_PROT_HASOBJECT	(1 << 2) /**< Message contains an object. */
/**
 * @}
 */

/**
 * @brief Represents the header of every busybus message.
 */
struct bbus_msg_hdr
{
	uint16_t magic;		/**< Busybus magic number. */
	uint8_t msgtype;	/**< Message type. */
	uint8_t errcode;	/**< Protocol error code. */
	uint32_t token;		/**< Used only for method calling. */
	uint16_t psize;		/**< Size of the payload. */
	uint8_t flags;		/**< Various protocol flags. */
	uint8_t padding;	/**< Padding space. */
};

/**
 * @brief Represents a busybus message.
 */
struct bbus_msg
{
	struct bbus_msg_hdr hdr;	/**< Message header. */
	char payload[1];		/**< Start of the payload data. */
};

/**
 * @brief Size of the busybus message header.
 */
#define BBUS_MSGHDR_SIZE	(sizeof(struct bbus_msg_hdr))

/**
 * @}
 *
 * @defgroup __caller__ Client calls
 * @{
 *
 * Functions used by method calling clients.
 */

/* TODO Asynchronous calls. */

/**
 * @brief Opaque type representing a client connection.
 */
typedef struct __bbus_client_connection bbus_client_connection;

/**
 * @brief Establishes a client connection with the busybus server.
 * @return New connection object or NULL in case of an error.
 */
bbus_client_connection* bbus_cli_connect(void) BBUS_PUBLIC;

/**
 * @brief Establishes a client connection with custom socket path.
 * @param path Filename of the socket to connect to.
 * @return New connection object or NULL in case of an error.
 */
bbus_client_connection* bbus_cli_connectp(const char* path) BBUS_PUBLIC;

/**
 * @brief Calls a method synchronously.
 * @param conn The client connection.
 * @param method Full service and method name.
 * @param arg Marshalled arguments.
 * @return Returned marshalled data or NULL if error.
 */
bbus_object* bbus_cli_callmethod(bbus_client_connection* conn,
		char* method, bbus_object* arg) BBUS_PUBLIC;

/**
 * @brief Closes the client connection.
 * @param conn The client connection to close.
 * @return 0 if the connection has been properly closed, -1 on error.
 */
int bbus_cli_closeconn(bbus_client_connection* conn) BBUS_PUBLIC;

/**
 * @}
 *
 * @defgroup __service__ Service publishing
 * @{
 *
 * Functions and data structures used by service publishing clients.
 */

 /**
  * @brief Opaque type representing a service publisher connection.
  */
typedef struct __bbus_service_connection bbus_service_connection;

/**
 * @brief Represents a function that is actually being called on method call.
 */
typedef bbus_object* (*bbus_method_func)(const char*, bbus_object*);

/**
 * @brief Represents a single busybus method.
 *
 * Contains all the data, that is needed to properly register a method
 * within bbusd.
 */
struct bbus_method
{
	char* name;		/**< Name of the method. */
	char* argdscr;		/**< Description of the required arguments. */
	char* retdscr;		/**< Description of the return value. */
	bbus_method_func func;	/**< Pointer to the method function. */
};

/**
 * @brief Establishes a service publisher connection with the busybus server.
 * @param name Whole path of the service location ie. 'foo.bar.baz'.
 * @return New connection object or NULL in case of an error.
 */
bbus_service_connection* bbus_srvc_connect(const char* name) BBUS_PUBLIC;

/**
 * @brief Establishes a service publisher connection with custom socket path.
 * @param name Whole path of the service location ie. 'foo.bar.baz'.
 * @param path Filename of the socket to connect to.
 * @return New connection object or NULL in case of an error.
 */
bbus_service_connection* bbus_srvc_connectp(
		const char* name, const char* path) BBUS_PUBLIC;

/**
 * @brief Registers a method within the busybus server.
 * @param conn The publisher connection.
 * @param method Method data to register.
 * @return 0 on successful registration, -1 on error.
 */
int bbus_srvc_regmethod(bbus_service_connection* conn,
		struct bbus_method* method) BBUS_PUBLIC;

/**
 * @brief Unregisters a method from the busybus server.
 * @param conn The publisher connection.
 * @param method Method data to unregister.
 * @return 0 on successful unregistration, -1 on error.
 */
int bbus_srvc_unregmethod(bbus_service_connection* conn,
		const char* method) BBUS_PUBLIC;

/**
 * @brief Closes the service publisher connection.
 * @param conn The publisher connection to close.
 * @return 0 if the connection has been properly closed, -1 on error.
 */
int bbus_srvc_closeconn(bbus_service_connection* conn) BBUS_PUBLIC;

/**
 * @brief Listens for method calls on an open connection.
 * @param conn The service publisher connection.
 * @param tv Time after which the function will exit with a timeout status.
 * @return An integer indicating the result.
 *
 * Returns 0 if timed out with no method call, -1 in case of an
 * error and 1 if method has been called.
 */
int bbus_srvc_listencalls(bbus_service_connection* conn,
		struct bbus_timeval* tv) BBUS_PUBLIC;

/* TODO Listening on multiple connections. */

/**
 * @}
 */

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
int bbus_srv_rcvmsg(bbus_client* cli, void* buf, size_t bufsize) BBUS_PUBLIC;
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

