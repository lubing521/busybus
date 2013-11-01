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

#include <busybus.h>
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

struct __bbus_object
{
	size_t bufsize;
	size_t bufused;
	char* buf;
	/* These fields are used for data extraction. */
	int extracting;	/* 0 if not currently extracting, 1 otherwise. */
	char* at;	/* Current position during extraction. */

};

#define BUFFER_BASE	64
#define BUFFER_AT(OBJ)	((OBJ)->buf + (OBJ)->bufused)

bbus_object* bbus_obj_alloc(void)
{
	return bbus_malloc0(sizeof(struct __bbus_object));
}

void bbus_obj_free(bbus_object* obj)
{
	if (obj) {
		bbus_free(obj->buf);
		bbus_free(obj);
	}
}

void bbus_obj_reset(bbus_object* obj)
{
	obj->bufused = 0;
}

void* bbus_obj_rawdata(bbus_object* obj)
{
	return obj->buf;
}

size_t bbus_obj_rawsize(const bbus_object* obj)
{
	return obj->bufused;
}

int bbus_obj_descrvalid(const char* descr)
{
	unsigned lbr = 0;
	unsigned rbr = 0;
	int empty_struct = 0;

	if (strlen(descr) == 0)
		return 0;

	while (*descr) {
		switch (*descr) {
		case BBUS_TYPE_INT32:
		case BBUS_TYPE_UINT32:
		case BBUS_TYPE_BYTE:
		case BBUS_TYPE_STRING:
		case BBUS_TYPE_ARRAY:
			empty_struct = 0;
			break;
		case BBUS_TYPE_STRUCT_START:
			empty_struct = 1;
			++lbr;
			break;
		case BBUS_TYPE_STRUCT_END:
			if (empty_struct)
				return 0;
			++rbr;
			if (rbr > lbr)
				return 0;
			break;
		default:
			return 0;
		}
		++descr;
	}

	if (lbr != rbr)
		return 0;

	return 1;
}

static int has_needed_space(bbus_object* obj, size_t needed)
{
	return (obj->bufsize - obj->bufused) >= needed ? 1 : 0;
}

static int enlarge_buffer(bbus_object* obj)
{
	if (obj->buf == NULL) {
		obj->buf = bbus_malloc0(BUFFER_BASE);
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

static int insert_data(bbus_object* obj, const void* data, size_t size)
{
	int r;

	r = make_enough_space(obj, size);
	if (r < 0) {
		__bbus_seterr(BBUS_ENOMEM);
		return -1;
	}

	memcpy(BUFFER_AT(obj), data, size);
	obj->bufused += size;

	return 0;
}

static void make_ready_for_extraction(bbus_object* obj)
{
	obj->at = obj->buf;
	obj->extracting = 1;
}

static int can_extract_size(bbus_object* obj, size_t size)
{
	return ((obj->at + size) > (obj->buf + obj->bufused)) ? 0 : 1;
}

static int extract_data(bbus_object* obj, void* buf, size_t size)
{
	if (obj->extracting == 0) {
		make_ready_for_extraction(obj);
	}

	if (!can_extract_size(obj, size)) {
		__bbus_seterr(BBUS_EOBJINVFMT);
		return -1;
	}

	memcpy(buf, obj->at, size);
	obj->at += size;

	return 0;
}

int bbus_obj_insarray(bbus_object* obj, bbus_size arrsize)
{
	arrsize = htonl(arrsize);

	return insert_data(obj, &arrsize, sizeof(bbus_size));
}

int bbus_obj_extrarray(bbus_object* obj, bbus_size* arrsize)
{
	int r;

	r = extract_data(obj, arrsize, sizeof(bbus_size));
	if (r < 0)
		return -1;
	*arrsize = ntohl(*arrsize);

	return 0;
}

int bbus_obj_insint(bbus_object* obj, bbus_int32 val)
{
	val = htonl(val);

	return insert_data(obj, &val, sizeof(bbus_int32));
}

int bbus_obj_extrint(bbus_object* obj, bbus_int32* val)
{
	int r;

	r = extract_data(obj, val, sizeof(bbus_int32));
	if (r < 0)
		return -1;
	*val = ntohl(*val);

	return 0;
}

int bbus_obj_insuint(bbus_object* obj, bbus_uint32 val)
{
	val = htonl(val);

	return insert_data(obj, &val, sizeof(bbus_uint32));
}

int bbus_obj_extruint(bbus_object* obj, bbus_uint32* val)
{
	int r;

	r = extract_data(obj, val, sizeof(bbus_uint32));
	if (r < 0)
		return -1;
	*val = ntohl(*val);

	return 0;
}

int bbus_obj_insstr(bbus_object* obj, const char* val)
{
	return insert_data(obj, val, strlen(val) + 1);
}

int bbus_obj_extrstr(bbus_object* obj, char** val)
{
	void* end;

	if (obj->extracting == 0) {
		make_ready_for_extraction(obj);
	}

	/*
	 * This one's a bit different than any other data-extracting. We need
	 * to determine whether a string actually exists in the buffer, and
	 * make *val point to its beginning.
	 */

	end = memmem(obj->at, (obj->buf + obj->bufused) - obj->at, "\0", 1);
	if (end == NULL) {
		__bbus_seterr(BBUS_EOBJINVFMT);
		return -1;
	}

	*val = obj->at;
	obj->at += strlen(obj->at) + 1;

	return 0;
}

int bbus_obj_insbyte(bbus_object* obj, bbus_byte val)
{
	return insert_data(obj, &val, 1);
}

int bbus_obj_extrbyte(bbus_object* obj, bbus_byte* val)
{
	bbus_uint32 u = 0;
	int ret;

	ret = extract_data(obj, &u, 1);
	if (ret < 0)
		return -1;

	*val = (bbus_byte)u;
	return 0;
}

int bbus_obj_insbytes(bbus_object* obj, const void* buf, size_t size)
{
	int r;

	r = bbus_obj_insarray(obj, (bbus_size)size);
	if (r < 0)
		return -1;

	return insert_data(obj, buf, size);
}

int bbus_obj_extrbytes(bbus_object* obj, void* buf, size_t size)
{
	int r;
	bbus_size arrsize;

	r = bbus_obj_extrarray(obj, &arrsize);
	if (r < 0)
		return -1;

	if (size != (size_t)arrsize) {
		__bbus_seterr(BBUS_EOBJINVFMT);
		return -1;
	}

	return extract_data(obj, buf, size);
}

void bbus_obj_rewind(bbus_object* obj)
{
	obj->extracting = 0;
}

bbus_object* bbus_obj_frombuf(const void* buf, size_t bufsize)
{
	bbus_object* obj;

	obj = bbus_malloc0(sizeof(struct __bbus_object));
	if (obj == NULL)
		return NULL;
	obj->buf = bbus_malloc0(bufsize);
	if (obj->buf == NULL) {
		bbus_free(obj);
		return NULL;
	}
	memcpy(obj->buf, buf, bufsize);
	obj->bufsize = bufsize;
	obj->bufused = bufsize;

	return obj;
}

struct va_list_box
{
	va_list va;
};

static int build_simple_type(char descr, bbus_object* obj,
				struct va_list_box* va_box)
{
	int ret;

	switch (descr) {
	case BBUS_TYPE_INT32:
		ret = bbus_obj_insint(obj, va_arg(va_box->va, bbus_int32));
		break;
	case BBUS_TYPE_UINT32:
		ret = bbus_obj_insuint(obj, va_arg(va_box->va, bbus_uint32));
		break;
	case BBUS_TYPE_BYTE:
		ret = bbus_obj_insbyte(obj,
				(bbus_byte)va_arg(va_box->va, int));
		break;
	case BBUS_TYPE_STRING:
		ret = bbus_obj_insstr(obj, va_arg(va_box->va, char*));
		break;
	default:
		__bbus_seterr(BBUS_ELOGICERR);
		return -1;
	}

	return ret;
}

/* Prototype for build_struct(). */
static int build_array(const char** descr, bbus_object* obj,
					struct va_list_box* va_box);

static int build_struct(const char** descr, bbus_object* obj,
					struct va_list_box* va_box)
{
	int ret;

	while (**descr != BBUS_TYPE_STRUCT_END) {
		switch (**descr) {
		case BBUS_TYPE_ARRAY:
			ret = build_array(descr, obj, va_box);
			break;
		case BBUS_TYPE_STRUCT_START:
			++(*descr);
			ret = build_struct(descr, obj, va_box);
			break;
		default:
			ret = build_simple_type(**descr, obj, va_box);
			++(*descr);
			break;
		}
		if (ret < 0)
			return -1;
	}

	++(*descr);
	return 0;
}

static int build_array(const char** descr, bbus_object* obj,
					struct va_list_box* va_box)
{
	bbus_size arrsize;
	int ret;
	const char* pos;

	arrsize = va_arg(va_box->va, bbus_size);
	ret = bbus_obj_insarray(obj, arrsize);
	++(*descr);
	while (--arrsize) {
		switch (**descr) {
		case BBUS_TYPE_ARRAY:
			ret = build_array(descr, obj, va_box);
			break;
		case BBUS_TYPE_STRUCT_START:
			pos = *descr;
			++(*descr);
			ret = build_struct(descr, obj, va_box);
			*descr = pos;
			break;
		default:
			ret = build_simple_type(**descr, obj, va_box);
			break;
		}
		if (ret < 0)
			return -1;
	}

	return 0;
}

bbus_object* bbus_obj_build(const char* descr, ...)
{
	va_list va;
	bbus_object* obj;

	va_start(va, descr);
	obj = bbus_obj_vbuild(descr, va);
	va_end(va);

	return obj;
}

bbus_object* bbus_obj_vbuild(const char* descr, va_list va)
{
	bbus_object* obj;
	int ret;
	struct va_list_box va_box;

	if (!bbus_obj_descrvalid(descr)) {
		__bbus_seterr(BBUS_EINVALARG);
		return NULL;
	}

	obj = bbus_obj_alloc();
	if (obj == NULL)
		return NULL;

	va_copy(va_box.va, va);

	while (*descr) {
		switch (*descr) {
		case BBUS_TYPE_ARRAY:
			ret = build_array(&descr, obj, &va_box);
			break;
		case BBUS_TYPE_STRUCT_START:
			++descr;
			ret = build_struct(&descr, obj, &va_box);
			break;
		case BBUS_TYPE_STRUCT_END:
			/* This should have been handled by build_struct(). */
			__bbus_seterr(BBUS_ELOGICERR);
			ret = -1;
			break;
		default:
			ret = build_simple_type(*descr, obj, &va_box);
			++descr;
			break;
		}
		if (ret < 0)
			goto out;
	}

	return obj;

out:
	va_end(va_box.va);
	bbus_obj_free(obj);
	return NULL;
}

static int parse_simple_type(char descr, bbus_object* obj,
					struct va_list_box* va_box)
{
	int ret;

	switch (descr) {
	case BBUS_TYPE_INT32:
		ret = bbus_obj_extrint(obj, va_arg(va_box->va, bbus_int32*));
		break;
	case BBUS_TYPE_UINT32:
		ret = bbus_obj_extruint(obj, va_arg(va_box->va, bbus_uint32*));
		break;
	case BBUS_TYPE_BYTE:
		ret = bbus_obj_extrbyte(obj,
				(bbus_byte*)va_arg(va_box->va, int*));
		break;
	case BBUS_TYPE_STRING:
		ret = bbus_obj_extrstr(obj, va_arg(va_box->va, char**));
		break;
	default:
		__bbus_seterr(BBUS_ELOGICERR);
		return -1;
	}

	return ret;
}

/* Prototype for parse_struct(). */
static int parse_array(const char** descr, bbus_object* obj,
					struct va_list_box* va_box);

static int parse_struct(const char** descr, bbus_object* obj,
					struct va_list_box* va_box)
{
	int ret;

	while (**descr != BBUS_TYPE_STRUCT_END) {
		switch (**descr) {
		case BBUS_TYPE_ARRAY:
			ret = parse_array(descr, obj, va_box);
			break;
		case BBUS_TYPE_STRUCT_START:
			++(*descr);
			ret = parse_struct(descr, obj, va_box);
			break;
		default:
			ret = parse_simple_type(**descr, obj, va_box);
			++(*descr);
			break;
		}
		if (ret < 0)
			return -1;
	}

	++(*descr);
	return 0;
}

static int parse_array(const char** descr, bbus_object* obj,
					struct va_list_box* va_box)
{
	bbus_size arrsize;
	int ret;
	const char* pos;

	ret = bbus_obj_extrarray(obj, &arrsize);
	++(*descr);
	while (--arrsize) {
		switch (**descr) {
		case BBUS_TYPE_ARRAY:
			ret = parse_array(descr, obj, va_box);
			break;
		case BBUS_TYPE_STRUCT_START:
			pos = *descr;
			++(*descr);
			ret = parse_struct(descr, obj, va_box);
			*descr = pos;
			break;
		default:
			ret = parse_simple_type(**descr, obj, va_box);
			break;
		}
		if (ret < 0)
			return -1;
	}

	return 0;
}

int bbus_obj_parse(bbus_object* obj, const char* descr, ...)
{
	va_list va;
	int r;

	va_start(va, descr);
	r = bbus_obj_vparse(obj, descr, va);
	va_end(va);

	return r;
}

int bbus_obj_vparse(bbus_object* obj, const char* descr, va_list va)
{
	int ret = 0;
	struct va_list_box va_box;

	if (!bbus_obj_descrvalid(descr)) {
		__bbus_seterr(BBUS_EINVALARG);
		return -1;
	}

	va_copy(va_box.va, va);

	while (*descr) {
		switch (*descr) {
		case BBUS_TYPE_ARRAY:
			ret = parse_array(&descr, obj, &va_box);
			break;
		case BBUS_TYPE_STRUCT_START:
			++descr;
			ret = parse_struct(&descr, obj, &va_box);
			break;
		case BBUS_TYPE_STRUCT_END:
			/* This should have been handled by parse_struct(). */
			__bbus_seterr(BBUS_ELOGICERR);
			ret = -1;
			break;
		default:
			ret = parse_simple_type(*descr, obj, &va_box);
			++descr;
			break;
		}
		if (ret < 0)
			goto out;
	}

out:
	va_end(va_box.va);
	obj->extracting = 0;
	return ret;
}

static inline void shrinkbuf(char** buf, size_t* bufsize, size_t numbytes)
{
	*buf += numbytes;
	*bufsize -= numbytes;
}

static int repr_simple_type(char descr, bbus_object* obj,
				char** buf, size_t* bufsize)
{
	int ret;

	switch (descr) {
	case BBUS_TYPE_INT32:
		{
			bbus_int32 v;
			ret = bbus_obj_extrint(obj, &v);
			if (ret < 0)
				goto out;
			ret = snprintf(*buf, *bufsize, "%d, ", v);
		}
		break;
	case BBUS_TYPE_UINT32:
		{
			bbus_uint32 v;
			ret = bbus_obj_extruint(obj, &v);
			if (ret < 0)
				goto out;
			ret = snprintf(*buf, *bufsize, "%u, ", v);
		}
		break;
	case BBUS_TYPE_BYTE:
		{
			bbus_byte v;
			ret = bbus_obj_extrbyte(obj, &v);
			if (ret < 0)
				goto out;
			ret = snprintf(*buf, *bufsize, "0x%x, ", v);
		}
		break;
	case BBUS_TYPE_STRING:
		{
			char* v;
			ret = bbus_obj_extrstr(obj, &v);
			if (ret < 0)
				goto out;
			ret = snprintf(*buf, *bufsize, "%s, ", v);
		}
		break;
	default:
		__bbus_seterr(BBUS_ELOGICERR);
		return -1;
	}

	if (ret < 0) {
		__bbus_seterr(BBUS_ENOSPACE);
		return -1;
	}

	shrinkbuf(buf, bufsize, ret);

out:
	return ret;
}

/* Prototype for repr_struct(). */
static int repr_array(const char** descr, bbus_object* obj,
				char** buf, size_t* bufsize);

static int repr_struct(const char** descr, bbus_object* obj,
				char** buf, size_t* bufsize)
{
	int ret;

	ret = snprintf(*buf, *bufsize, "(");
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	shrinkbuf(buf, bufsize, ret);

	while (**descr != BBUS_TYPE_STRUCT_END) {
		switch (**descr) {
		case BBUS_TYPE_ARRAY:
			ret = repr_array(descr, obj, buf, bufsize);
			break;
		case BBUS_TYPE_STRUCT_START:
			++(*descr);
			ret = repr_struct(descr, obj, buf, bufsize);
			break;
		default:
			ret = repr_simple_type(**descr, obj, buf, bufsize);
			++(*descr);
			break;
		}
		if (ret < 0)
			return -1;
	}

	if (strncmp((*buf)-2, ", ", 1) == 0) {
		*buf -= 2;
		*bufsize += 2;
	}

	ret = snprintf(*buf, *bufsize, ")");
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	shrinkbuf(buf, bufsize, ret);

	++(*descr);
	return 0;
}

static int repr_array(const char** descr, bbus_object* obj,
				char** buf, size_t* bufsize)
{
	bbus_size arrsize;
	int ret;
	const char* strstart;
	const char* strend = NULL;;

	ret = snprintf(*buf, *bufsize, "A[");
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	shrinkbuf(buf, bufsize, ret);

	ret = bbus_obj_extrarray(obj, &arrsize);
	++(*descr);
	while (arrsize--) {
		switch (**descr) {
		case BBUS_TYPE_ARRAY:
			ret = repr_array(descr, obj, buf, bufsize);
			strend = NULL;
			break;
		case BBUS_TYPE_STRUCT_START:
			strstart = *descr;
			++(*descr);
			ret = repr_struct(descr, obj, buf, bufsize);
			strend = *descr;
			*descr = strstart;
			break;
		default:
			ret = repr_simple_type(**descr, obj, buf, bufsize);
			strend = NULL;
			break;
		}
		if (ret < 0)
			return -1;
	}

	ret = snprintf(*buf, *bufsize, "]");
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	shrinkbuf(buf, bufsize, ret);
	if (strend)
		*descr = strend;

	return 0;
}

int bbus_obj_repr(bbus_object* obj, const char* descr,
		char* buf, size_t bufsize)
{
	int ret;
	char* bufstart = buf;

	if (!bbus_obj_descrvalid(descr)) {
		__bbus_seterr(BBUS_EINVALARG);
		return -1;
	}

	ret = snprintf(buf, bufsize, "bbus_object(");
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	shrinkbuf(&buf, &bufsize, ret);

	while (*descr) {
		switch (*descr) {
		case BBUS_TYPE_ARRAY:
			ret = repr_array(&descr, obj, &buf, &bufsize);
			break;
		case BBUS_TYPE_STRUCT_START:
			++descr;
			ret = repr_struct(&descr, obj, &buf, &bufsize);
			break;
		case BBUS_TYPE_STRUCT_END:
			/* This should have been handled by repr_struct(). */
			__bbus_seterr(BBUS_ELOGICERR);
			ret = -1;
			break;
		default:
			ret = repr_simple_type(*descr, obj, &buf, &bufsize);
			++descr;
			break;
		}
		if (ret < 0)
			goto out;
	}

	if (strncmp((buf)-2, ", ", 1) == 0) {
		buf -= 2;
		bufsize += 2;
	}

	snprintf(buf, bufsize, ")");
	ret = strlen(bufstart);

out:
	return ret;
}

