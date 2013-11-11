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
#include <stdio.h>

BBUSUNIT_DEFINE_TEST(hashmap_keystr)
{
	BBUSUNIT_BEGINTEST;

		bbus_hashmap* hmap;
		int r;
		long i;
		char keybuf[32];
		void* val;

		hmap = bbus_hmap_create(BBUS_HMAP_KEYSTR);
		BBUSUNIT_ASSERT_NOTNULL(hmap);

		for (i = 0; i < 140; ++i) {
			memset(keybuf, 0, sizeof(keybuf));
			snprintf(keybuf, sizeof(keybuf), "%ld", i);
			r = bbus_hmap_setstr(hmap, keybuf, (void*)i);
			BBUSUNIT_ASSERT_EQ(0, r);
		}

		val = bbus_hmap_findstr(hmap, "40");
		BBUSUNIT_ASSERT_EQ(40, (long)val);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(hashmap_keyuint)
{
	BBUSUNIT_BEGINTEST;

		bbus_hashmap* hmap;
		int r;
		long i;
		unsigned key;
		void* val;

		hmap = bbus_hmap_create(BBUS_HMAP_KEYUINT);
		BBUSUNIT_ASSERT_NOTNULL(hmap);

		for (i = 0; i < 140; ++i) {
			key = (unsigned)i;
			r = bbus_hmap_setuint(hmap, key, (void*)i);
			BBUSUNIT_ASSERT_EQ(0, r);
		}

		val = bbus_hmap_finduint(hmap, 40);
		BBUSUNIT_ASSERT_EQ(40, (long)val);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(hashmap_reassign)
{
	BBUSUNIT_BEGINTEST;

		static const unsigned key = 42;
		static int val1 = 1;
		static int val2 = 2;

		bbus_hashmap* hmap;
		int r;
		void* v;

		hmap = bbus_hmap_create(BBUS_HMAP_KEYUINT);
		BBUSUNIT_ASSERT_NOTNULL(hmap);
		r = bbus_hmap_setuint(hmap, key, &val1);
		BBUSUNIT_ASSERT_EQ(0, r);
		v = bbus_hmap_finduint(hmap, key);
		BBUSUNIT_ASSERT_TRUE(*((int*)v) == 1);
		r = bbus_hmap_setuint(hmap, key, &val2);
		BBUSUNIT_ASSERT_EQ(0, r);
		v = bbus_hmap_finduint(hmap, key);
		BBUSUNIT_ASSERT_TRUE(*((int*)v) == 2);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(hashmap_invalid_type)
{
	BBUSUNIT_BEGINTEST;

		bbus_hashmap* hmap;
		int r;

		hmap = bbus_hmap_create(BBUS_HMAP_KEYSTR);
		BBUSUNIT_ASSERT_NOTNULL(hmap);
		r = bbus_hmap_setuint(hmap, 123, NULL);
		BBUSUNIT_ASSERT_EQ(-1, r);
		BBUSUNIT_ASSERT_EQ(BBUS_EHMAPINVTYPE, bbus_lasterror());

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(hashmap_remove)
{
	BBUSUNIT_BEGINTEST;

		bbus_hashmap* hmap;
		int r;
		long val1;
		long val2;
		void* found;

		hmap = bbus_hmap_create(BBUS_HMAP_KEYUINT);
		BBUSUNIT_ASSERT_NOTNULL(hmap);
		val1 = 123;
		val2 = 456;
		r = bbus_hmap_setuint(hmap, 123, &val1);
		BBUSUNIT_ASSERT_EQ(0, r);
		r = bbus_hmap_setuint(hmap, 456, &val2);
		BBUSUNIT_ASSERT_EQ(0, r);
		found = bbus_hmap_finduint(hmap, 123);
		BBUSUNIT_ASSERT_NOTNULL(found);
		found = bbus_hmap_finduint(hmap, 456);
		BBUSUNIT_ASSERT_NOTNULL(found);
		found = bbus_hmap_rmuint(hmap, 123);
		BBUSUNIT_ASSERT_NOTNULL(found);
		found = bbus_hmap_finduint(hmap, 123);
		BBUSUNIT_ASSERT_NULL(found);
		found = bbus_hmap_finduint(hmap, 456);
		BBUSUNIT_ASSERT_NOTNULL(found);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(hashmap_reset)
{
	BBUSUNIT_BEGINTEST;

		bbus_hashmap* hmap;
		int r;
		long val1;
		long val2;
		void* found;

		hmap = bbus_hmap_create(BBUS_HMAP_KEYUINT);
		BBUSUNIT_ASSERT_NOTNULL(hmap);
		val1 = 123;
		val2 = 456;
		r = bbus_hmap_setuint(hmap, 123, &val1);
		BBUSUNIT_ASSERT_EQ(0, r);
		r = bbus_hmap_setuint(hmap, 456, &val2);
		BBUSUNIT_ASSERT_EQ(0, r);
		found = bbus_hmap_finduint(hmap, 123);
		BBUSUNIT_ASSERT_NOTNULL(found);
		found = bbus_hmap_finduint(hmap, 456);
		BBUSUNIT_ASSERT_NOTNULL(found);
		bbus_hmap_reset(hmap);
		found = bbus_hmap_finduint(hmap, 123);
		BBUSUNIT_ASSERT_NULL(found);
		found = bbus_hmap_finduint(hmap, 456);
		BBUSUNIT_ASSERT_NULL(found);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

