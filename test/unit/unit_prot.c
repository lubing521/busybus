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

BBUSUNIT_DEFINE_TEST(prot_extract_meta)
{
	BBUSUNIT_BEGINTEST;

		static const char msgbuf[] =
				"\xBB\xC5"
				"\x01"
				"\x00"
				"\x00\x00\x00\x00"
				"\x00\x0C"
				"\x01"
				"\x00"
				"meta string\0";

		static const struct bbus_msg* msg = (struct bbus_msg*)msgbuf;
		static const size_t msgsize = sizeof(msgbuf)-1;

		const char* meta;

		meta = bbus_prot_extractmeta(msg, msgsize);
		BBUSUNIT_ASSERT_NOTNULL(meta);
		BBUSUNIT_ASSERT_STREQ("meta string", meta);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(prot_extract_meta_and_obj)
{
	BBUSUNIT_BEGINTEST;

		static const char msgbuf[] =
				"\xBB\xC5"
				"\x01"
				"\x00"
				"\x00\x00\x00\x00"
				"\x00\x14"
				"\x03"
				"\x00"
				"meta string\0"
				"\x11\x22\x33\x44"
				"\x55\x66\x77\x88";

		static const struct bbus_msg* msg = (struct bbus_msg*)msgbuf;
		static const size_t msgsize = sizeof(msgbuf)-1;

		bbus_object* obj = NULL;
		/* FIXME GCC complains later if obj is uninitialized, why? */
		const char* meta;

		meta = bbus_prot_extractmeta(msg, msgsize);
		BBUSUNIT_ASSERT_NOTNULL(meta);
		BBUSUNIT_ASSERT_STREQ("meta string", meta);
		obj = bbus_prot_extractobj(msg, msgsize);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		BBUSUNIT_ASSERT_EQ(2*sizeof(bbus_uint32),
					bbus_obj_rawsize(obj));
		BBUSUNIT_ASSERT_EQ(0, memcmp(
					"\x11\x22\x33\x44\x55\x66\x77\x88",
					bbus_obj_rawdata(obj),
					bbus_obj_rawsize(obj)));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(prot_extract_invalid_meta)
{
	BBUSUNIT_BEGINTEST;

		static const char msgbuf[] =
				"\xBB\xC5"
				"\x01"
				"\x00"
				"\x00\x00\x00\x00"
				"\x00\x0C"
				"\x01"
				"\x00"
				"meta string without null";

		static const struct bbus_msg* msg = (struct bbus_msg*)msgbuf;
		static const size_t msgsize = sizeof(msgbuf)-1;

		const char* meta;

		meta = bbus_prot_extractmeta(msg, msgsize);
		BBUSUNIT_ASSERT_NULL(meta);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(prot_extract_flags_not_set)
{
	BBUSUNIT_BEGINTEST;

		static const char msgbuf[] =
				"\xBB\xC5"
				"\x01"
				"\x00"
				"\x00\x00\x00\x00"
				"\x00\x14"
				"\x00"
				"\x00"
				"meta string\0"
				"\x11\x22\x33\x44"
				"\x55\x66\x77\x88";

		static const struct bbus_msg* msg = (struct bbus_msg*)msgbuf;
		static const size_t msgsize = sizeof(msgbuf)-1;

		const char* meta;
		bbus_object* obj;

		meta = bbus_prot_extractmeta(msg, msgsize);
		BBUSUNIT_ASSERT_NULL(meta);
		obj = bbus_prot_extractobj(msg, msgsize);
		BBUSUNIT_ASSERT_NULL(obj);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

