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
#include <stdio.h>
#include <errno.h>
#include <time.h>

BBUSUNIT_DEFINE_TEST(crc32)
{
	BBUSUNIT_BEGINTEST;

		static const uint32_t proper = 0x28D8E5AAU;
		static const char* const data = "abcdefgh123456789";

		BBUSUNIT_ASSERT_EQ(proper, bbus_crc32(data, strlen(data)));

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(crc32_emptybuf)
{
	BBUSUNIT_BEGINTEST;

		BBUSUNIT_ASSERT_EQ(0, bbus_crc32("", 0));

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
		BBUSUNIT_ASSERT_NOTNULL(newp);
		BBUSUNIT_ASSERT_EQ(0, memcmp(buf, newp, bufsize));
		BBUSUNIT_ASSERT_NOTEQ(buf, newp);

	BBUSUNIT_FINALLY;

		bbus_free(newp);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(error_strings)
{
	BBUSUNIT_BEGINTEST;

		/*
		 * Check some error strings to avoid mistakes when modifying
		 * error codes and descriptions.
		 */

		BBUSUNIT_ASSERT_STREQ("success",
					bbus_strerror(BBUS_ESUCCESS));
		BBUSUNIT_ASSERT_STREQ("received message of incorrect type",
					bbus_strerror(BBUS_EMSGINVTYPRCVD));
		BBUSUNIT_ASSERT_STREQ("error registering the method",
					bbus_strerror(BBUS_EMREGERR));

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(error_libc_error_strings)
{
	BBUSUNIT_BEGINTEST;

		/*
		 * Check, that bbus_strerror called with regular errnos
		 * returns the same error string strerror does.
		 */

		BBUSUNIT_ASSERT_STREQ(strerror(ENOMEM), bbus_strerror(ENOMEM));
		BBUSUNIT_ASSERT_STREQ(strerror(EACCES), bbus_strerror(EACCES));

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(malloc0)
{
	BBUSUNIT_BEGINTEST;

		/* Ensure that malloc0 really zeroes the memory. */

		char proper[BUFSIZ];
		void* p;

		memset(proper, 0, BUFSIZ);
		p = bbus_malloc0(BUFSIZ);
		BBUSUNIT_ASSERT_NOTNULL(p);
		BBUSUNIT_ASSERT_EQ(0, memcmp(p, proper, BUFSIZ));

	BBUSUNIT_FINALLY;

		bbus_free(p);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(build_basic_string)
{
	BBUSUNIT_BEGINTEST;

		static const char* const proper =
			"This is a test string and it "
			"contains some numbers: 123 3.14";

		char* str;

		str = bbus_str_build("This is %s and it contains "
					"some numbers: %d %1.2f",
					"a test string", 123, 3.14f);
		BBUSUNIT_ASSERT_NOTNULL(str);
		BBUSUNIT_ASSERT_STREQ(str, proper);

	BBUSUNIT_FINALLY;

		bbus_str_free(str);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(build_long_string)
{
	BBUSUNIT_BEGINTEST;

		static const char* const proper =
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG"
			"LONGLONGLONGLONGLONGLONGLONGLONG";

		char* str;

		str = bbus_str_build("%s", proper);
		BBUSUNIT_ASSERT_NOTNULL(str);
		BBUSUNIT_ASSERT_STREQ(str, proper);

	BBUSUNIT_FINALLY;

		bbus_str_free(str);

	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(str_cpy)
{
	BBUSUNIT_BEGINTEST;

		static const char* const tocopy = "somethin somethin";
		char* str;

		str = bbus_str_cpy(tocopy);
		BBUSUNIT_ASSERT_NOTNULL(str);
		BBUSUNIT_ASSERT_STREQ(tocopy, str);

	BBUSUNIT_FINALLY;

		bbus_str_free(str);

	BBUSUNIT_ENDTEST;
}

/* For the min_multiple_eval test. */
static int* twice(int* i)
{
	*i *= 2;
	return i;
}

BBUSUNIT_DEFINE_TEST(min_multiple_eval)
{
	BBUSUNIT_BEGINTEST;

		/* Test the BBUS_MIN macro for double evaluation. */

		int x = 2;
		int y = 5;

		BBUSUNIT_ASSERT_EQ(4, BBUS_MIN(*(twice(&x)), y));

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(array_size)
{
	BBUSUNIT_BEGINTEST;

		static const struct timeval tv[] = {
			{
				.tv_sec = 1,
				.tv_usec = 1,
			},
			{
				.tv_sec = 2,
				.tv_usec = 2,
			},
			{
				.tv_sec = 3,
				.tv_usec = 3,
			},
			{
				.tv_sec = 4,
				.tv_usec = 5,
			},
		};

		BBUSUNIT_ASSERT_EQ(4, BBUS_ARRAY_SIZE(tv));

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

