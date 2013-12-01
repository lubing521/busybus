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
#include <stdlib.h>
#include <string.h>

/*
 * The longest error message in glibc is about 50 characters long so 64 should
 * be enough to store every error message in the future too.
 */
#define ERRSTR_MAX 64

static BBUS_THREAD_LOCAL int last_error = BBUS_ESUCCESS;
static BBUS_THREAD_LOCAL char errmsg[ERRSTR_MAX];

static const char* const error_descr[] = {
	"success",
	"out of memory",
	"invalid argument",
	"invalid busybus object format",
	"not enough space in buffer",
	"connection closed by remote peer",
	"invalid message format",
	"wrong magic number in received message",
	"received message of incorrect type",
	"session open rejected",
	"didn't manage to send all data",
	"received less data, than expected",
	"internal logic error",
	"no such method",
	"internal method error",
	"poll interrupted by a signal",
	"error registering the method",
	"invalid key type used on a hashmap",
	"invalid regular expression pattern"
};

int bbus_lasterror(void)
{
	return last_error;
}

const char* bbus_strerror(int errnum)
{
	if (errnum < BBUS_ESUCCESS)
		return strerror_r(errnum, errmsg, ERRSTR_MAX);
	else if (errnum >= __BBUS_MAX_ERR)
		return "invalid error code";
	else
		return error_descr[errnum-BBUS_ESUCCESS];
}

void __bbus_seterr(int errnum)
{
	last_error = errnum;
}

