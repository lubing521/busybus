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
#include <string.h>

BBUSUNIT_DEFINE_TEST(crc32)
{
	BBUSUNIT_BEGINTEST;

		static const uint32_t proper = 0x28D8E5AAU;
		static const char* const data = "abcdefgh123456789";

		BBUSUNIT_ASSERT_EQ(proper, bbus_crc32(data, strlen(data)));

	BBUSUNIT_FINALLY;

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(memdup)
{
	BBUSUNIT_BEGINTEST;

		static const char* const buf = "\xDE\xAD\xBE\xEF";
		static const size_t bufsize = 4;
		void* newp;

		newp = bbus_memdup(buf, bufsize);
		BBUSUNIT_ASSERT_NOT_NULL(newp);
		BBUSUNIT_ASSERT_EQ(0, memcmp(buf, newp, bufsize));

	BBUSUNIT_FINALLY;

		bbus_free(newp);

	BBUSUNIT_ENDTEST;
}

