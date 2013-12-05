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

#ifndef __BBUSD_CLIENTS__
#define __BBUSD_CLIENTS__

#include <busybus.h>

struct bbusd_clientlist_elem
{
	struct bbusd_clientlist_elem* next;
	struct bbusd_clientlist_elem* prev;
	bbus_client* cli;
};

struct bbusd_clientlist
{
	struct bbusd_clientlist_elem* head;
	struct bbusd_clientlist_elem* tail;
};

int client_list_add(bbus_client* cli);
int monitor_list_add(bbus_client* cli);

void list_rm(struct bbusd_clientlist_elem** elem,
				struct bbusd_clientlist* list);

#endif /* __BBUSD_CLIENTS__ */

