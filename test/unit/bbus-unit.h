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

#ifndef __BBUS_UNIT__
#define __BBUS_UNIT__

#include <busybus.h>

void bbusunit_print(const char* fmt, ...) BBUS_PRINTF_FUNC(1, 2);
void bbusunit_printerr(const char* fmt, ...) BBUS_PRINTF_FUNC(1, 2);

typedef int (*bbusunit_testfunc)(void);

struct bbusunit_listelem
{
	struct bbusunit_listelem* next;
	struct bbusunit_listelem* prev;
	const char* name;
	bbusunit_testfunc testfunc;
};

void bbusunit_registertest(struct bbusunit_listelem* test);

#define BBUSUNIT_DEFINE_TEST(NAME)					\
	static int __##NAME##_test(void);				\
	static struct bbusunit_listelem __##NAME##_elem = {		\
		.name = #NAME,						\
		.testfunc = __##NAME##_test,				\
	};								\
	static void BBUS_ATSTART_LAST __##NAME##_register(void)		\
	{								\
		bbusunit_registertest(&__##NAME##_elem);		\
	}								\
	static int __##NAME##_test(void)

#define BBUSUNIT_BEGINTEST int __test_retval = 0

#define BBUSUNIT_ENDTEST						\
	do {								\
		return __test_retval;					\
	} while (0)

/*
 * Both 'goto __finally' and 'break' are here only to get rid
 * of compiler warnings.
 */
#define BBUSUNIT_FINALLY						\
	do {								\
		goto __finally;						\
		__finally:						\
			break;						\
	} while (0)

/*
 * Assertions:
 */

#define BBUSUNIT_PRINTASSERTFAIL					\
	bbusunit_printerr(						\
		"Assertion failed in file %s, line %d!",		\
		__FILE__, __LINE__)

#define BBUSUNIT_TESTERRQUIT						\
	do {								\
		__test_retval = -1;					\
		goto __finally;						\
	} while (0)

#define BBUSUNIT_ASSERT_EQ(EXPECTED, ACTUAL)				\
	do {								\
		if ((EXPECTED) != (ACTUAL)) {				\
			BBUSUNIT_PRINTASSERTFAIL;			\
			bbusunit_printerr("'%s' isn't equal to '%s'",	\
						#EXPECTED, #ACTUAL);	\
			BBUSUNIT_TESTERRQUIT;				\
		}							\
	} while (0)

#define BBUSUNIT_ASSERT_NOTEQ(EXPECTED, ACTUAL)				\
	do {								\
		if ((EXPECTED) == (ACTUAL)) {				\
			BBUSUNIT_PRINTASSERTFAIL;			\
			bbusunit_printerr("'%s' is equal to '%s'",	\
						#EXPECTED, #ACTUAL);	\
			BBUSUNIT_TESTERRQUIT;				\
		}							\
	} while (0)

#define BBUSUNIT_ASSERT_NOT_NULL(PTR)					\
	do {								\
		if ((PTR) == NULL) {					\
			BBUSUNIT_PRINTASSERTFAIL;			\
			bbusunit_printerr("'%s' is NULL!", #PTR);	\
			BBUSUNIT_TESTERRQUIT;				\
		}							\
	} while (0)

#define BBUSUNIT_ASSERT_FALSE(STATEMENT)				\
	do {								\
		if ((STATEMENT)) {					\
			BBUSUNIT_PRINTASSERTFAIL;			\
			bbusunit_printerr(				\
				"Statement '%s' evaluated to true!",	\
				#STATEMENT);				\
			BBUSUNIT_TESTERRQUIT;				\
		}							\
	} while (0)

#define BBUSUNIT_ASSERT_TRUE(STATEMENT)					\
	do {								\
		if (!(STATEMENT)) {					\
			BBUSUNIT_PRINTASSERTFAIL;			\
			bbusunit_printerr(				\
				"Statement '%s' evaluated to false!",	\
				#STATEMENT);				\
			BBUSUNIT_TESTERRQUIT;				\
		}							\
	} while (0)

#define BBUSUNIT_ASSERT_STREQ(STR1, STR2)				\
	do {								\
		if (strcmp(STR1, STR2) != 0) {				\
			BBUSUNIT_PRINTASSERTFAIL;			\
			bbusunit_printerr(				\
				"Strings '%s' and '%s' "		\
				"are not the same", STR1, STR2);	\
			BBUSUNIT_TESTERRQUIT;				\
		}							\
	} while (0)

#endif /* __BBUS_UNIT__ */

