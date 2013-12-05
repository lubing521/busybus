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

struct bbusd_clientlist clients = { NULL, NULL };
struct bbusd_clientlist monitors = { NULL, NULL };

static int list_add(bbus_client* cli, struct bbusd_clientlist* list)
{
	struct bbusd_clientlist_elem* el;

	el = bbus_malloc(sizeof(struct bbusd_clientlist_elem));
	if (el == NULL)
		return -1;

	el->cli = cli;
	bbus_list_push(list, el);

	return 0;
}

void list_rm(struct bbusd_clientlist_elem** elem,
				struct bbusd_clientlist* list)
{
	bbus_list_rm(list, *elem);
	bbus_free(*elem);
	*elem = NULL;
}

int client_list_add(bbus_client* cli)
{
	return list_add(cli, &clients);
}

int monitor_list_add(bbus_client* cli)
{
	return list_add(cli, &monitors);
}

