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

#include "busybus.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

struct __bbus_object
{
	int state;
	size_t bufsize;
	size_t bufused;
	char* buf;
	/* These fields are used for data extraction. */
	char* at;
	char* dc;
};

#define BUFFER_BASE	64
#define BUFFER_AT(OBJ)	(OBJ->buf + OBJ->bufused)

static int has_needed_space(bbus_object* obj, size_t needed)
{
	return (obj->bufsize - obj->bufused) >= needed ? 1 : 0;
}

static int enlarge_buffer(bbus_object* obj)
{
	if (obj->buf == NULL) {
		obj->buf = bbus_malloc(BUFFER_BASE);
		if (obj->buf == NULL)
			return -1;
		obj->bufsize = BUFFER_BASE;
	} else {
		char* newbuf;

		newbuf = bbus_realloc(obj->buf, obj->bufsize*2);
		if (newbuf == NULL)
			return -1;
		obj->buf = newbuf;
		obj->bufsize *= 2;
	}

	return 0;
}

static int make_enough_space(bbus_object* obj, size_t needed)
{
	int r;

	while (!has_needed_space(obj, needed)) {
		r = enlarge_buffer(obj);
		if (r < 0)
			return -1;
	}

	return 0;
}

static int validate_object_fmt(const void* objbuf, size_t objsize)
{
	const char* descr;
	char* buf_at;
	char* buf_src;
	size_t at;
	size_t sl;
	unsigned i;
	size_t ds;

	buf_src = memmem(objbuf, objsize, "\0", 1);
	if (buf_src == NULL)
		return -1;

	descr = objbuf;
	buf_at = buf_src+1;
	ds = strlen(descr);
	at = ds+1;
	for (i = 0; i < ds; ++i) {
		switch (descr[i]) {
		case BBUS_TYPE_INT:
		case BBUS_TYPE_UNSIGNED:
			if ((objsize-at) < sizeof(bbus_int))
				return -1;
			at += sizeof(bbus_int);
			buf_at += sizeof(bbus_int);
			break;
		case BBUS_TYPE_BYTE:
			if ((objsize-at) < sizeof(bbus_byte))
				return -1;
			at += sizeof(bbus_byte);
			buf_at += sizeof(bbus_byte);
			break;
		case BBUS_TYPE_STRING:
			buf_src = memmem(buf_at, objsize-at, "\0", 1);
			if (buf_src == NULL)
				return -1;
			sl = strlen(buf_at)+1;
			at += sl;
			buf_at += sl;
			break;
		default:
			return -1;
			break;
		}
	}

	return 0;
}

static void make_ready_for_extraction(bbus_object* obj)
{
	size_t ds = strlen(obj->buf)+1;

	obj->state = BBUS_OBJ_EXTRACTING;
	obj->dc = obj->buf;
	obj->at = obj->buf + ds;
}

bbus_object* bbus_empty_object(void)
{
	struct __bbus_object* obj;

	obj = bbus_malloc(sizeof(struct __bbus_object));
	if (obj == NULL)
		return NULL;
	memset(obj, 0, sizeof(struct __bbus_object));
	obj->state = BBUS_OBJ_EMPTY;
	return obj;
}

int bbus_obj_setdescr(bbus_object* obj, const char* descr)
{
	int r;
	size_t ds;

	if (obj->state != BBUS_OBJ_EMPTY) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	ds = strlen(descr);
	r = make_enough_space(obj, ds);
	if (r < 0) {
		__bbus_set_err(BBUS_NOMEM);
		return -1;
	}

	strncpy(obj->buf, descr, obj->bufsize);
	obj->bufused = ds+1;
	obj->state = BBUS_OBJ_INSERTING;
	return 0;
}

const char* bbus_obj_getdescr(bbus_object* obj)
{
	if (obj->state != BBUS_OBJ_READY) {
		__bbus_set_err(BBUS_OBJINVOP);
		return NULL;
	}

	return (const char*)obj->buf;
}

int bbus_obj_insert_int(bbus_object* obj, bbus_int val)
{
	return bbus_obj_insert_unsigned(obj, (bbus_unsigned)val);
}

int bbus_obj_insert_unsigned(bbus_object* obj, bbus_unsigned val)
{
	int r;

	if (obj->state != BBUS_OBJ_INSERTING) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	val = htonl(val);
	r = make_enough_space(obj, sizeof(bbus_unsigned));
	if (r < 0) {
		__bbus_set_err(BBUS_NOMEM);
		return -1;
	}
	memcpy(BUFFER_AT(obj), &val, sizeof(bbus_unsigned));
	obj->bufused += sizeof(bbus_unsigned);
	return 0;
}

int bbus_obj_insert_string(bbus_object* obj, uint8_t* val)
{
	int r;
	size_t len;

	if (obj->state != BBUS_OBJ_INSERTING) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	len = strlen((char*)val)+1;
	r = make_enough_space(obj, len);
	if (r < 0) {
		__bbus_set_err(BBUS_NOMEM);
		return -1;
	}
	memcpy(BUFFER_AT(obj), val, len);
	obj->bufused += len;
	return 0;
}

int bbus_obj_extract_int(bbus_object* obj, bbus_int* val)
{
	if (obj->state != BBUS_OBJ_EXTRACTING) {
		if (obj->state != BBUS_OBJ_READY) {
			__bbus_set_err(BBUS_OBJINVOP);
			return -1;
		} else {
			make_ready_for_extraction(obj);
		}
	}

	if (*(obj->dc) != BBUS_TYPE_INT) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	memcpy(val, obj->at, sizeof(bbus_int));
	*val = ntohl(*val);
	obj->at += sizeof(bbus_int);
	obj->dc += 1;

	return 0;
}

int bbus_obj_extract_unsigned(bbus_object* obj, bbus_unsigned* val)
{
	if (obj->state != BBUS_OBJ_EXTRACTING) {
		if (obj->state != BBUS_OBJ_READY) {
			__bbus_set_err(BBUS_OBJINVOP);
			return -1;
		} else {
			make_ready_for_extraction(obj);
		}
	}

	if (*(obj->dc) != BBUS_TYPE_UNSIGNED) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	memcpy(val, obj->at, sizeof(bbus_unsigned));
	*val = ntohl(*val);
	obj->at += sizeof(bbus_unsigned);
	obj->dc += 1;

	return 0;
}

int bbus_obj_extract_string(bbus_object* obj, uint8_t** val)
{
	size_t slen;

	if (obj->state != BBUS_OBJ_EXTRACTING) {
		if (obj->state != BBUS_OBJ_READY) {
			__bbus_set_err(BBUS_OBJINVOP);
			return -1;
		} else {
			make_ready_for_extraction(obj);
		}
	}

	if (*(obj->dc) != BBUS_TYPE_STRING) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	slen = strlen(obj->at);
	*val = (uint8_t*)obj->at;
	obj->at += slen;
	obj->dc += 1;

	return 0;
}

void bbus_obj_reset(bbus_object* obj)
{
	obj->bufused = 0;
	obj->state = BBUS_OBJ_EMPTY;
}

int bbus_obj_getstate(bbus_object* obj)
{
	return obj->state;
}

bbus_object* bbus_make_object(const char* descr, ...)
{
	va_list va;
	bbus_object* obj;

	va_start(va, descr);
	obj = bbus_make_object_v(descr, va);
	va_end(va);

	return obj;
}

bbus_object* bbus_make_object_v(const char* descr, va_list va)
{
	unsigned i;
	size_t dlen;
	int r;
	bbus_object* obj;

	obj = bbus_empty_object();
	if (obj == NULL)
		goto out;
	r = bbus_obj_setdescr(obj, descr);
	if (r < 0)
		goto out;
	dlen = strlen(descr);
	for (i = 0; i < dlen; ++i) {
		switch (descr[i]) {
		case BBUS_TYPE_INT:
			r = bbus_obj_insert_int(obj, va_arg(va, bbus_int));
			break;
		case BBUS_TYPE_UNSIGNED:
			r = bbus_obj_insert_unsigned(
					obj, va_arg(va, bbus_unsigned));
			break;
		case BBUS_TYPE_STRING:
			r = bbus_obj_insert_string(obj,
					va_arg(va, bbus_byte*));
			break;
		default:
			__bbus_set_err(BBUS_OBJINVFMT);
			goto out;
			break;
		}
		if (r < 0)
			goto out;
	}

	obj->state = BBUS_OBJ_READY;
	return obj;

out:
	bbus_free_object(obj);
	obj = NULL;
	return obj;
}

bbus_object* bbus_object_from_buf(const void* buf, size_t bufsize)
{
	int r;
	bbus_object* obj;

	r = validate_object_fmt(buf, bufsize);
	if (r < 0) {
		__bbus_set_err(BBUS_OBJINVFMT);
		return NULL;
	}

	obj = bbus_malloc(sizeof(struct __bbus_object));
	memset(obj, 0, sizeof(struct __bbus_object));
	obj->buf = bbus_malloc(bufsize);
	memcpy(obj->buf, buf, bufsize);
	obj->state = BBUS_OBJ_READY;
	obj->bufsize = bufsize;
	obj->bufused = bufsize;

	return obj;
}

ssize_t bbus_object_to_buf(bbus_object* obj, void* buf, size_t bufsize)
{
	if (obj->state != BBUS_OBJ_READY) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	if (bufsize < obj->bufsize) {
		__bbus_set_err(BBUS_NOSPACE);
		return -1;
	}

	memcpy(buf, obj->buf, obj->bufused);
	return obj->bufused;
}

int bbus_parse_object(bbus_object* obj, const char* descr, ...)
{
	va_list va;
	int r;

	va_start(va, descr);
	r = bbus_parse_object_v(obj, descr, va);
	va_end(va);

	return r;
}

int bbus_parse_object_v(bbus_object* obj, const char* descr, va_list va)
{
	unsigned i;
	size_t dlen;
	int r = 0;

	dlen = strlen(descr);
	for (i = 0; i < dlen; ++i) {
		switch (descr[i]) {
		case BBUS_TYPE_INT:
			r = bbus_obj_extract_int(obj, va_arg(va, bbus_int*));
			break;
		case BBUS_TYPE_UNSIGNED:
			r = bbus_obj_extract_unsigned(
					obj, va_arg(va, bbus_unsigned*));
			break;
		case BBUS_TYPE_STRING:
			r = bbus_obj_extract_string(obj,
					va_arg(va, bbus_byte**));
			break;
		default:
			r = -1;
			__bbus_set_err(BBUS_OBJINVFMT);
			goto out;
			break;
		}
		if (r < 0)
			goto out;
	}

out:
	return r;
}

void* bbus_obj_rawdata(bbus_object* obj)
{
	return obj->buf;
}

size_t bbus_obj_rawdata_size(bbus_object* obj)
{
	return obj->bufused;
}

void bbus_free_object(bbus_object* obj)
{
	bbus_free(obj->buf);
	bbus_free(obj);
}


