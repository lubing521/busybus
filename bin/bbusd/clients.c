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

#include "clients.h"

static struct bbusd_clientlist clients = { NULL, NULL };

int bbusd_clientlist_add(bbus_client* cli)
{
	return __bbusd_clientlist_add(cli, &clients);
}

void bbusd_clientlist_rm(struct bbusd_clientlist_elem** elem)
{
	__bbusd_clientlist_rm(elem, &clients);
}

struct bbusd_clientlist_elem* bbusd_clientlist_getfirst(void)
{
	return clients.head;
}

struct bbusd_clientlist_elem* bbusd_clientlist_getlast(void)
{
	return clients.tail;
}

