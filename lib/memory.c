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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <busybus.h>
#include "memory.h"
#include "error.h"

void* bbus_malloc(size_t size)
{
	void* p;

	if (size == 0)
		size = 1;
	p = malloc(size);
	if (p == NULL)
		__bbus_seterr(BBUS_ENOMEM);
	return p;
}

void* bbus_malloc0(size_t size)
{
	void* p;

	p = bbus_malloc(size);
	if (p)
		memset(p, 0, size);

	return p;
}

void* bbus_realloc(void* ptr, size_t size)
{
	void* p;

	if (size == 0)
		size = 1;
	p = realloc(ptr, size);
	if (p == NULL)
		__bbus_seterr(BBUS_ENOMEM);
	return p;
}

void bbus_free(void* ptr)
{
	if (ptr != NULL)
		free(ptr);
}

void* bbus_memdup(const void* src, size_t size)
{
	void* newp;

	newp = bbus_malloc(size);
	if (newp)
		memcpy(newp, src, size);

	return newp;
}

