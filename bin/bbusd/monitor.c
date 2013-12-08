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

#include "monitor.h"
#include "log.h"

static struct bbusd_clientlist monitors = { NULL, NULL };

int bbusd_monlist_add(bbus_client* cli)
{
	return __bbusd_clientlist_add(cli, &monitors);
}

void bbusd_monlist_rm(bbus_client* cli)
{
	struct bbusd_clientlist_elem* mon;

	for (mon = monitors.head; mon != NULL; mon = mon->next) {
		if (mon->cli == cli) {
			__bbusd_clientlist_rm(&mon, &monitors);
			return;
		}
	}

	bbusd_logmsg(BBUS_LOG_WARN,
		"Monitor not found in the list, "
		"this should not happen.\n");
}

void bbusd_send_to_monitors(struct bbus_msg* msg BBUS_UNUSED)
{
	return;
}

