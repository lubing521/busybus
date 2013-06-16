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
	} else {
		char* newbuf;

		newbuf = bbus_realloc(obj->buf, obj->bufsize*2);
		if (newbuf == NULL)
			return -1;
		obj->buf = newbuf;
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

static int validate_object_fmt(void* objbuf, size_t objsize)
{
	char* descr;
	char* s;
	size_t at;
	int i;
	size_t ds;

	s = memmem(objbuf, objsize, '\0', 1);
	if (descr == NULL) {
		__bbus_set_err(BBUS_OBJINVFMT);
		return -1;
	}

	descr = objbuf;
	s += 1;
	ds = strlen(descr);
	at = ds+1;
	for (i = 0; i < ds; ++i) {
		switch (descr[i]) {
		case BBUS_TYPE_INT:
		case BBUS_TYPE_UNSIGNED:
			break;
		case BBUS_TYPE_BYTE:
			break;
		case BBUS_TYPE_STRING:
			break;
		default:
			break;
		}
	}

	return 0;
}

static void make_ready_for_extraction(bbus_object* obj)
{
	obj->state = BBUS_OBJ_EXTRACTING;
	obj->dc = obj->buf;
	obj->at = strlen(obj->buf)+1;
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
	if (r < 0)
		return -1;

	strncpy(obj->buf, descr, obj->bufsize);
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
	int r;

	if (obj->state != BBUS_OBJ_INSERTING) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	val = htonl(val);
	r = make_enough_space(obj, sizeof(bbus_int));
	if (r < 0)
		return -1;
	memcpy(obj->buf, &val, sizeof(bbus_int));
	BUFFER_AT(obj) += sizeof(bbus_int);
	return 0;
}




//int bbus_obj_insert_unsigned(bbus_object* obj, bbus_unsigned val) BBUS_PUBLIC;
//int bbus_obj_insert_string(bbus_object* obj, uint8_t* val) BBUS_PUBLIC;


int bbus_obj_extract_int(bbus_object* obj, bbus_int* val)
{
	if (obj->state != BBUS_OBJ_READY) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	} else {
		make_ready_for_extraction(obj);
	}

	if (*(obj->dc) != BBUS_TYPE_INT) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	memcpy(val, obj->at, sizeof(bbus_int));
	val = ntohl(val);
	obj->at += sizeof(bbus_int);
	obj->dc += 1;

	return 0;
}



//int bbus_obj_extract_unsigned(bbus_object* obj,
//		bbus_unsigned* val) BBUS_PUBLIC;
//int bbus_obj_extract_string(bbus_object* obj, uint8_t** val) BBUS_PUBLIC;




void bbus_obj_reset(bbus_object* obj)
{
	obj->bufused = 0;
	obj->state = BBUS_OBJ_EMPTY;
}

int bbus_obj_getstate(bbus_object* obj)
{
	return obj->state;
}

//bbus_object* bbus_make_object(const char* descr, ...) BBUS_PUBLIC;
//bbus_object* bbus_object_from_buf(const void* buf, size_t bufsize) BBUS_PUBLIC;

ssize_t bbus_object_to_buf(bbus_object* obj, void* buf, size_t bufsize)
{
	if (obj->state != BBUS_OBJ_READY) {
		__bbus_set_err(BBUS_OBJINVOP);
		return -1;
	}

	if (bufsize < obj->bufsize) {
		_bbus_set_err(BBUS_NOSPACE);
		return -1;
	}

	memcpy(buf, obj->buf, obj->bufused);
	return obj->bufused;
}


//int bbus_parse_object(bbus_object* obj, const char* descr, ...) BBUS_PUBLIC;


void bbus_free_object(bbus_object* obj)
{
	bbus_free(obj->buf);
	bbus_free(obj);
}


