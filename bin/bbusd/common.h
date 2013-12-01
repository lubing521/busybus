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

#ifndef __BBUSD_COMMON__
#define __BBUSD_COMMON__

#include <busybus.h>

void bbusd_die(const char* format, ...) BBUS_PRINTF_FUNC(1, 2) BBUS_NORETURN;

#endif /* __BBUSD_COMMON__ */

