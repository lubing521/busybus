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

/*
 * We assert, that bbus_obj_rawdata() and bbus_obj_rawsize() work as
 * expected, as we're gonna use them to test other functions.
 */

BBUSUNIT_DEFINE_TEST(object_basic_insert)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\xFE\xDC\xBA\x99"
					"\x11\x22\x33\x44"
					"string\0"
					"\x66"
					"\x00\x00\x00\x05"
					"\x21\x32\x43\x54\x65";

		static const size_t propsize = sizeof(propbuf)-1;
		static const int i = -0x01234567;
		static const unsigned u = 0x11223344;
		static const char* const s = "string";
		static const char b = '\x66';
		static const char t[] = "\x21\x32\x43\x54\x65";
		static const size_t ts = sizeof(t)-1;

		bbus_object* obj;
		int ret;

		obj = bbus_obj_alloc();
		BBUSUNIT_ASSERT_NOTNULL(obj);
		ret = bbus_obj_insint(obj, i);
		BBUSUNIT_ASSERT_EQ(0, ret);
		ret = bbus_obj_insuint(obj, u);
		BBUSUNIT_ASSERT_EQ(0, ret);
		ret = bbus_obj_insstr(obj, s);
		BBUSUNIT_ASSERT_EQ(0, ret);
		ret = bbus_obj_insbyte(obj, b);
		BBUSUNIT_ASSERT_EQ(0, ret);
		ret = bbus_obj_insbytes(obj, t, ts);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(propsize, bbus_obj_rawsize(obj));
		BBUSUNIT_ASSERT_EQ(0, memcmp(bbus_obj_rawdata(obj),
						propbuf, propsize));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_frombuf)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\xFE\xDC\xBA\x99"
					"\x11\x22\x33\x44"
					"string\0"
					"\x66"
					"\x00\x00\x00\x05"
					"\x21\x32\x43\x54\x65";

		static const size_t propsize = sizeof(propbuf)-1;

		bbus_object* obj;

		obj = bbus_obj_frombuf(propbuf, propsize);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		BBUSUNIT_ASSERT_EQ(propsize, bbus_obj_rawsize(obj));
		BBUSUNIT_ASSERT_EQ(0, memcmp(bbus_obj_rawdata(obj),
						propbuf, propsize));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_basic_extract)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\xFE\xDC\xBA\x99"
					"\x11\x22\x33\x44"
					"string\0"
					"\x66"
					"\x00\x00\x00\x05"
					"\x21\x32\x43\x54\x65";

		static const size_t propsize = sizeof(propbuf)-1;

		int i;
		unsigned u;
		char* s;
		unsigned char b;
		char t[5];
		size_t ts = sizeof(t);
		bbus_object* obj;
		int ret;

		obj = bbus_obj_frombuf(propbuf, propsize);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		ret = bbus_obj_extrint(obj, &i);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(i, -0x01234567);
		ret = bbus_obj_extruint(obj, &u);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(u, 0x11223344);
		ret = bbus_obj_extrstr(obj, &s);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_STREQ("string", s);
		ret = bbus_obj_extrbyte(obj, &b);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(b, 0x66);
		memset(t, 0, ts);
		ret = bbus_obj_extrbytes(obj, t, ts);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(0, memcmp(t, "\x21\x32\x43\x54\x65", ts));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_extract_rewind)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\x11\x22\x33\x44"
					"oneone\0"
					"\x66";

		static const size_t propsize = sizeof(propbuf)-1;

		unsigned u;
		char* s;
		unsigned char b;
		int ret;
		bbus_object* obj;

		obj = bbus_obj_frombuf(propbuf, propsize);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		ret = bbus_obj_extruint(obj, &u);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(u, 0x11223344);
		ret = bbus_obj_extrstr(obj, &s);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_STREQ("oneone", s);
		bbus_obj_rewind(obj);
		u = 0;
		s = NULL;
		ret = bbus_obj_extruint(obj, &u);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(u, 0x11223344);
		ret = bbus_obj_extrstr(obj, &s);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_STREQ("oneone", s);
		ret = bbus_obj_extrbyte(obj, &b);
		BBUSUNIT_ASSERT_EQ(0, ret);
		BBUSUNIT_ASSERT_EQ(b, 0x66);

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_basic_build)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\x00\x00\x00\x02"
					"\x11\x22\x33\x44"
					"oneone\0"
					"\x55\x66\x77\x88"
					"twotwo\0"
					"\xAA\xBB\xCC\xDD"
					"\xFF"
					"\x66";

		static const size_t propsize = sizeof(propbuf)-1;

		bbus_object* obj;

		obj = bbus_obj_build("A(us)(u(bb))", 2,
				0x11223344u, "oneone",
				0x55667788u, "twotwo",
				0xaabbccddu, 0xff, 0x66);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		BBUSUNIT_ASSERT_EQ(propsize, bbus_obj_rawsize(obj));
		BBUSUNIT_ASSERT_EQ(0, memcmp(bbus_obj_rawdata(obj),
						propbuf, propsize));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_basic_parse)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\x00\x00\x00\x02"
					"\x11\x22\x33\x44"
					"oneone\0"
					"\x55\x66\x77\x88"
					"twotwo\0"
					"\xAA\xBB\xCC\xDD"
					"\xFF"
					"\x66";

		static const size_t propsize = sizeof(propbuf)-1;

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
		BBUSUNIT_ASSERT_EQ(2, arrsize);
		BBUSUNIT_ASSERT_EQ(0x11223344, au1);
		BBUSUNIT_ASSERT_STREQ("oneone", as1);
		BBUSUNIT_ASSERT_EQ(0x55667788, au2);
		BBUSUNIT_ASSERT_STREQ("twotwo", as2);
		BBUSUNIT_ASSERT_EQ(0xAABBCCDD, su);
		BBUSUNIT_ASSERT_EQ(0xFF, sb1);
		BBUSUNIT_ASSERT_EQ(0x66, sb2);

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_hardcore_nesting)
{
	BBUSUNIT_BEGINTEST;

		static const char propbuf[] =
					"\x00\x00\x00\x02"
					"\x00\x00\x00\x03"
					"\x11\x22\x33\x44"
					"first string\0"
					"\x55\x66\x77\x88"
					"second string\0"
					"\x12\x23\x34\x45"
					"third string\0"
					"\x00\x00\x00\x08"
					"\x11\x22\x33\x44\x55\x66\x77\x88"
					"\x00\x00\x00\x02"
					"\x11\x22\x33\x44"
					"first second string\0"
					"\x55\x66\x77\x88"
					"second second string\0"
					"\x00\x00\x00\x05"
					"\xAA\xBB\xCC\xDD\xEE"
					"\x21\x32\x43\x54"
					"\x19"
					"\x91"
					"\x00\x00\x00\x03"
					"first first\0"
					"second second\0"
					"third third\0"
					"\x13\x24\x35\x46"
					"\x51\x52\x53\x54";

		static const size_t propsize = sizeof(propbuf)-1;

		bbus_object* obj;

		obj = bbus_obj_build("A(A(us)Ab)(u(bbAs(uu)))",
						2,
						3,
						0x11223344,
						"first string",
						0x55667788,
						"second string",
						0x12233445,
						"third string",
						8,
						0x11, 0x22, 0x33, 0x44,
						0x55, 0x66, 0x77, 0x88,
						2,
						0x11223344,
						"first second string",
						0x55667788,
						"second second string",
						5,
						0xAA, 0xBB, 0xCC, 0xDD, 0xEE,
						0x21324354,
						0x19,
						0x91,
						3,
						"first first",
						"second second",
						"third third",
						0x13243546,
						0x51525354);
		BBUSUNIT_ASSERT_NOTNULL(obj);
		BBUSUNIT_ASSERT_EQ(propsize, bbus_obj_rawsize(obj));
		BBUSUNIT_ASSERT_EQ(0, memcmp(bbus_obj_rawdata(obj),
						propbuf, propsize));

	BBUSUNIT_FINALLY;

		bbus_obj_free(obj);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(object_repr)
{
	BBUSUNIT_BEGINTEST;

		static const char* const proprepr =
			"bbus_object(A[(287454020, 'oneone')(1432778632, "
			"'twotwo')](2864434397, (0xf0, 0x58)))";

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

BBUSUNIT_DEFINE_TEST(object_repr_one_string)
{
	BBUSUNIT_BEGINTEST;

		static const char* const str = "something";
		static const char* const proprepr = "bbus_object('something')";

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

