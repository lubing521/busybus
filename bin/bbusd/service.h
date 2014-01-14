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

#ifndef __BBUSD_SERVICE__
#define __BBUSD_SERVICE__

#include "common.h"
#include "clientlist.h"

#define BBUSD_METHOD_LOCAL	0x01
#define BBUSD_METHOD_REMOTE	0x02
#define BBUSD_METHOD_SIGNAL	0x03

struct bbusd_method
{
	int type;
	char data[0];
};

struct bbusd_local_method
{
	int type;
	bbus_method_func func;
};

struct bbusd_remote_method
{
	int type;
	struct bbusd_clientlist_elem* srvc;
};

struct bbusd_signal
{
	int type;
	struct bbusd_clientlist handlers;
};

int bbusd_insert_method(const char* path, struct bbusd_method* mthd);
struct bbusd_method* bbusd_locate_method(const char* path);
void bbusd_init_service_map(void);
void bbusd_free_service_map(void);

#endif /* __BBUSD_SERVICE__ */
