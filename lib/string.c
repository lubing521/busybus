/*
 * Copyright (C) 2013 Bartosz Golaszewski <bartekgola@gmail.com>
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
 */

#include <busybus.h>
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_BUFSIZE		128
#define MAX_ITERATIONS		8

char* bbus_str_build(const char* fmt, ...)
{
	char* buf;
	va_list va;
	int r;
	ssize_t newbufsize;
	int i;
	char* newbuf;

	buf = bbus_malloc(INITIAL_BUFSIZE);
	if (buf == NULL)
		return NULL;
	memset(buf, 0, INITIAL_BUFSIZE);
	va_start(va, fmt);
	r = vsnprintf(buf, INITIAL_BUFSIZE, fmt, va);
	va_end(va);

	if (r >= INITIAL_BUFSIZE) {
		newbufsize = INITIAL_BUFSIZE;
		for (i = 0; i < MAX_ITERATIONS; ++i) {
			newbufsize *= 2;
			newbuf = bbus_realloc(buf, newbufsize);
			if (newbuf == NULL)
				goto errout_free;
			buf = newbuf;
			va_start(va, fmt);
			r = vsnprintf(buf, newbufsize, fmt, va);
			va_end(va);
			if (r < newbufsize)
				break;
		}
		if (r >= newbufsize) {
			__bbus_seterr(BBUS_ENOSPACE);
			goto errout_free;
		}
	}

	return buf;

errout_free:
	bbus_free(buf);
	return NULL;
}

char* bbus_str_cpy(const char* str)
{
	char* s;
	size_t size;

	size = strlen(str)+1;
	s = bbus_malloc(size);
	if (s == NULL)
		return NULL;
	memset(s, 0, size);
	strncpy(s, str, size);

	return s;
}

char* bbus_str_join(char* dst, const char* src)
{
	size_t dsize, ssize;

	dsize = strlen(dst);
	ssize = strlen(src);

	dst = bbus_realloc(dst, dsize + ssize + 1);
	if (dst == NULL)
		return NULL;

	strncpy(dst+dsize, src, ssize);
	dst[dsize + ssize] = '\0';

	return dst;
}

void bbus_str_free(char* str)
{
	bbus_free(str);
}


