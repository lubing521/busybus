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

#include <busybus.h>
#include "service.h"

#define DEF_LOCAL_METHOD(FUNC)						\
	static struct bbusd_local_method __m_##FUNC##__ = {		\
		.type = BBUSD_METHOD_LOCAL,				\
		.func = FUNC,						\
	}

#define REG_LOCAL_METHOD(PATH, FUNC)					\
	do {								\
		if (bbusd_insert_method(PATH,				\
				(struct bbusd_method*)			\
					&__m_##FUNC##__) < 0) {		\
			bbusd_die(					\
				"Error inserting method: '%s'\n",	\
				PATH);					\
		}							\
	} while (0)

static bbus_object* lm_echo(bbus_object* arg)
{
	char* msg;
	int ret;

	ret = bbus_obj_parse(arg, "s", &msg);
	if (ret < 0)
		return NULL;
	else
		return bbus_obj_build("s", msg);
}
DEF_LOCAL_METHOD(lm_echo);

void bbusd_register_local_methods(void)
{
	REG_LOCAL_METHOD("bbus.bbusd.echo", lm_echo);
}

