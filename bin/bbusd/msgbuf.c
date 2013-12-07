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

#include "msgbuf.h"
#include <string.h>

static unsigned char __msgbuf[BBUS_MAXMSGSIZE];
static struct bbus_msg* msgbuf = (struct bbus_msg*)__msgbuf;

struct bbus_msg* bbusd_getmsgbuf(void)
{
	return msgbuf;
}

void bbusd_zeromsgbuf(void)
{
	memset(msgbuf, 0, BBUS_MAXMSGSIZE);
}

size_t bbusd_msgbufsize(void)
{
	return BBUS_MAXMSGSIZE;
}

