/*
 * Copyright (C) 2013 Bartosz Golaszewski
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "bbus-unit.h"
#include <busybus.h>
#include <string.h>

BBUSUNIT_DEFINE_TEST(prot_extract_obj)
{
	BBUSUNIT_BEGINTEST;

		static const char msgbuf[] =
				"\xBB\xC5"
				"\x01"
				"\x00"
				"\x00\x00\x00\x00"
				"\x00\x09"
				"\x02"
				"\x00"
				"a string\0";

		static const struct bbus_msg* msg = (struct bbus_msg*)msgbuf;
		static const size_t msgsize = sizeof(msgbuf)-1;
		static const size_t objsize = sizeof(msgbuf)-1
							-BBUS_MSGHDR_SIZE;

		bbus_object* obj;
		char* s;
		int ret;

		obj = bbus_prot_extractobj(msg, msgsize);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		BBUSUNIT_ASSERT_EQ(objsize, bbus_obj_rawsize(obj));
		ret = bbus_obj_extrstr(obj, &s);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_STREQ("a string", s);

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}
