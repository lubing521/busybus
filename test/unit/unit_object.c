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

BBUSUNIT_DEFINE_TEST(basic_object)
{
	BBUSUNIT_BEGINTEST;

		static const char* const propbuf = "\x11\x22\x33\x44string\0";
		static const size_t propsize = 11;
		static const unsigned u = 0x11223344;
		static const char* const s = "string";

		bbus_object* obj;
		int ret;

		obj = bbus_obj_alloc();
		BBUSUNIT_ASSERT_NOTNULL(obj);
		ret = bbus_obj_insuint(obj, u);
		BBUSUNIT_ASSERT_EQ(0, ret);
		ret = bbus_obj_insstr(obj, s);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(0, memcmp(bbus_obj_rawdata(obj),
					propbuf, propsize));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(build_object)
{
	BBUSUNIT_BEGINTEST;

		static const char* const propbuf =
				"\x00\x00\x00\x02\x11\x22\x33\x44oneone\0\x55"
				"\x66\x77\x88twotwo\0\xAA\xBB\xCC\xDD\xFF\x66";
		static const size_t propsize = 32;

		bbus_object* obj;

		obj = bbus_obj_build("A(us)(u(bb))", 2,
				0x11223344u, "oneone",
				0x55667788u, "twotwo",
				0xaabbccddu, 0xff, 0x66);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		BBUSUNIT_ASSERT_EQ(0, memcmp(bbus_obj_rawdata(obj),
					propbuf, propsize));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(parse_object)
{
	BBUSUNIT_BEGINTEST;

		static const char* const propbuf =
				"\x00\x00\x00\x02\x11\x22\x33\x44oneone\0\x55"
				"\x66\x77\x88twotwo\0\xAA\xBB\xCC\xDD\xFF\x66";
		static const size_t propsize = 32;

		bbus_object* obj;
		int ret;

		bbus_size arrsize;
		bbus_uint32 au1;
		bbus_uint32 au2;
		char* as1;
		char* as2;
		bbus_uint32 su;
		bbus_byte sb1;
		bbus_byte sb2;

		obj = bbus_obj_frombuf(propbuf, propsize);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		ret = bbus_obj_parse(obj, "A(us)(u(bb))",
					&arrsize, &au1, &as1, &au2,
					&as2, &su, &sb1, &sb2);
		BBUSUNIT_ASSERT_EQ(0, ret);

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(repr_object)
{
	BBUSUNIT_BEGINTEST;

		static const char* const proprepr =
			"bbus_object(A[(287454020, oneone)(1432778632, "
			"twotwo)](2864434397, (0xf0, 0x58)))";

		char buf[256];
		int ret;
		bbus_object* obj;

		obj = bbus_obj_build("A(us)(u(bb))", 2,
				0x11223344u, "oneone",
				0x55667788u, "twotwo",
				0xaabbccddu, 0xf0, 0x58);
		BBUSUNIT_ASSERT_NOTNULL(obj);

		memset(buf, 0, sizeof(buf));
		ret = bbus_obj_repr(obj, "A(us)(u(bb))", buf, sizeof(buf));
		BBUSUNIT_ASSERT_EQ(strlen(proprepr), (size_t)ret);
		BBUSUNIT_ASSERT_STREQ(proprepr, buf);

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(repr_string)
{
	BBUSUNIT_BEGINTEST;

		static const char* const str = "something";
		static const char* const proprepr = "bbus_object(something)";

		char buf[256];
		int ret;
		bbus_object* obj;

		obj = bbus_obj_build("s", str);
		BBUSUNIT_ASSERT_NOTNULL(obj);

		memset(buf, 0, sizeof(buf));
		ret = bbus_obj_repr(obj, "s", buf, sizeof(buf));
		BBUSUNIT_ASSERT_EQ(strlen(proprepr), (size_t)ret);
		BBUSUNIT_ASSERT_STREQ(proprepr, buf);

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

