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

#include "callers.h"
#include "common.h"

/*
 * Caller map:
 * 	keys -> tokens,
 * 	values -> pointers to caller objects.
 */
static bbus_hashmap* caller_map;

void bbusd_init_caller_map(void)
{
	caller_map = bbus_hmap_create(BBUS_HMAP_KEYUINT);
	if (caller_map == NULL) {
		bbusd_die("Error creating the caller hashmap: %s\n",
					bbus_strerror(bbus_lasterror()));
	}
}

void bbusd_clean_caller_map(void)
{
	bbus_hmap_free(caller_map);
}

struct bbusd_clientlist_elem* bbusd_get_caller(unsigned token)
{
	return (struct bbusd_clientlist_elem*)bbus_hmap_finduint(caller_map,
									token);
}

int bbusd_add_caller(unsigned token, struct bbusd_clientlist_elem* caller)
{
	return bbus_hmap_setuint(caller_map, (unsigned)token, caller);
}

