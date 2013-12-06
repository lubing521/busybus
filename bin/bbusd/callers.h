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

#ifndef __BBUSD_CALLERS__
#define __BBUSD_CALLERS__

#include <busybus.h>
#include "clients.h"

void bbusd_init_caller_map(void);
void bbusd_clean_caller_map(void);

struct bbusd_clientlist_elem* bbusd_get_caller(unsigned token);
int bbusd_add_caller(unsigned token, struct bbusd_clientlist_elem* caller);


#endif /* __BBUSD_CALLERS__ */


