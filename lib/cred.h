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

#ifndef __BBUS_CRED__
#define __BBUS_CRED__

#include <busybus.h>

int __bbus_cred_get(int sock, struct bbus_client_cred* cred);
void __bbus_cred_copy(struct bbus_client_cred* dst,
			const struct bbus_client_cred* src);

#endif /* __BBUS_CRED__ */

