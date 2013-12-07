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
#include "clientlist.h"

int bbusd_clientlist_add(bbus_client* cli);
void bbusd_clientlist_rm(struct bbusd_clientlist_elem** elem);
struct bbusd_clientlist_elem* bbusd_clientlist_getfirst(void);
struct bbusd_clientlist_elem* bbusd_clientlist_getlast(void);

#endif /* __BBUSD_CLIENTS__ */

