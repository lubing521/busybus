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

BBUSUNIT_DEFINE_TEST(hashmap_basic)
{
	BBUSUNIT_BEGINTEST;

		bbus_hashmap* hmap;
		int r;
		long i;
		char keybuf[128];
		void* val;

		hmap = bbus_hmap_create();
		BBUSUNIT_ASSERT_NOTNULL(hmap);

		for (i = 0; i < 140; ++i) {
			memset(keybuf, 0, sizeof(keybuf));
			snprintf(keybuf, sizeof(keybuf), "%ld", i);
			r = bbus_hmap_sets(hmap, keybuf, (void*)i);
			BBUSUNIT_ASSERT_FALSE(r < 0);
		}

		val = bbus_hmap_finds(hmap, "40");
		BBUSUNIT_ASSERT_EQ(40, (long)val);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(hashmap_reassign)
{
	BBUSUNIT_BEGINTEST;

		static const int key = 42;
		static int val1 = 1;
		static int val2 = 2;

		bbus_hashmap* hmap;
		int r;
		void* v;

		hmap = bbus_hmap_create();
		BBUSUNIT_ASSERT_NOTNULL(hmap);
		r = bbus_hmap_set(hmap, &key, sizeof(key), &val1);
		BBUSUNIT_ASSERT_FALSE(r < 0);
		v = bbus_hmap_find(hmap, &key, sizeof(key));
		BBUSUNIT_ASSERT_TRUE(*((int*)v) == 1);
		r = bbus_hmap_set(hmap, &key, sizeof(key), &val2);
		BBUSUNIT_ASSERT_FALSE(r < 0);
		v = bbus_hmap_find(hmap, &key, sizeof(key));
		BBUSUNIT_ASSERT_TRUE(*((int*)v) == 2);

	BBUSUNIT_FINALLY;

		bbus_hmap_free(hmap);

	BBUSUNIT_ENDTEST;
}

