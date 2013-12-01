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

#include "bbus-unit.h"
#include <busybus.h>

BBUSUNIT_DEFINE_TEST(regex_match)
{
	BBUSUNIT_BEGINTEST;

		static const char pattern[] = "[a-z][0-9][^&][A-Z]+";
		static const char good[] = "c4%E";
		static const char bad[] = "a3&Z";

		BBUSUNIT_ASSERT_EQ(BBUS_TRUE, bbus_regex_match(pattern, good));
		BBUSUNIT_ASSERT_EQ(BBUS_FALSE, bbus_regex_match(pattern, bad));

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(regex_invalid_pattern)
{
	BBUSUNIT_BEGINTEST;

		static const char badpattern[] = "[(-";
		static const char dummy[] = "dummy";

		BBUSUNIT_ASSERT_EQ(-1, bbus_regex_match(badpattern, dummy));
		BBUSUNIT_ASSERT_EQ(BBUS_EREGEXPTRN, bbus_lasterror());

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}
