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

#include "busybus.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <search.h>

typedef int (*test_callback)(void);

struct testlist_elem
{
	struct testlist_elem* next;
	struct testlist_elem* prev;
	const char* name;
	test_callback testfunc;
};

static unsigned tests_run = 0;
static unsigned tests_failed = 0;
static struct testlist_elem* tests_head = NULL;
static struct testlist_elem* tests_tail = NULL;

static void die(const char* fmt, ...) BBUS_PRINTF_FUNC(1, 2);
static void die(const char* fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	exit(EXIT_FAILURE);
}

static void* xmalloc0(size_t size)
{
	void* p;

	p = malloc(size);
	if (p == NULL)
		die("Out of memory");
	memset(p, 0, size);
	return p;
}

static void print(const char* fmt, ...) BBUS_PRINTF_FUNC(1, 2);
static void print(const char* fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stdout, fmt, va);
	va_end(va);
}

#define PRINT_TESTERR(...)						\
	do {								\
		print("%s, line %d: ", __FILE__, __LINE__);		\
		print(__VA_ARGS__);					\
		print("\n");						\
	} while(0)

#define DEFINE_TEST(NAME)	static int __test_##NAME##__(void)
#define BEGIN			{
#define END			return 0; }

#define REGISTER_TEST(NAME)						\
	do {								\
		if (tests_head == NULL) {				\
			tests_head = xmalloc0(				\
					sizeof(struct testlist_elem));	\
			tests_tail = tests_head;			\
			tests_head->next = NULL;			\
			tests_head->prev = NULL;			\
		} else {						\
			struct testlist_elem* e;			\
			e = xmalloc0(sizeof(struct testlist_elem));	\
			insque(e, tests_tail);				\
			tests_tail = e;					\
		}							\
		tests_tail->name = #NAME;				\
		tests_tail->testfunc = __test_##NAME##__;		\
	} while (0)

#define ASSERT_NOT_NULL(PTR)						\
	do {								\
		if (PTR == NULL) {					\
			PRINT_TESTERR("Encountered a null pointer, "	\
				"expected a valid pointer.");		\
			return -1;					\
		}							\
	} while (0)

#define ASSERT_NULL(PTR)						\
	do {								\
		if (PTR != NULL) {					\
			PRINT_TESTERR("Expected a null pointer, "	\
				"a valid pointer ecountered.");		\
			return -1;					\
		}							\
	} while (0)

#define ASSERT_TRUE(EXP)						\
	do {								\
		if (!(EXP)) {						\
			PRINT_TESTERR("Expression evaluated to false, "	\
				"expected value is true.");		\
			return -1;					\
		}							\
	} while (0)

#define ASSERT_EQ(EXPECTED, ACTUAL)					\
	do {								\
		if (EXPECTED != ACTUAL) {				\
			PRINT_TESTERR("Expected equal values, "		\
				"actual value differs.");		\
			return -1;					\
		}							\
	} while (0)

#define ASSERT_STREQ(EXPECTED, ACTUAL)					\
	do {								\
		if (strcmp(EXPECTED, ACTUAL) != 0) {			\
			PRINT_TESTERR("Expected equal strings");	\
			return -1;					\
		}							\
	} while (0)

/**************************************
 * TESTS
 **************************************/

DEFINE_TEST(make_object)
BEGIN
	bbus_object* obj;
	char buf[128];
	static const char const proper[] = "ius\0\x11\x22\x33\x44\x44"
						"\x33\x22\x11somethin\0";

	obj = bbus_make_object("ius", 0x11223344, 0x44332211,
						"somethin");
	ASSERT_NOT_NULL(obj);
	memset(buf, 0, sizeof(buf));
	ASSERT_TRUE(bbus_object_to_buf(obj, buf, sizeof(buf)));
	ASSERT_TRUE(memcmp(proper, buf, 21) == 0);
	bbus_free_object(obj);
END

DEFINE_TEST(validate_object_format)
BEGIN
	static const char const good[] = "ius\0\x44\x33\x22\x11\x44"
						"\x33\x22\x11somethin\0";
	static const char const bad[] = "ius\0\x44\x33\x22\x11\x44"
						"\x33\x11somethin";
	bbus_object* obj;

	obj = bbus_object_from_buf(bad, 19);
	ASSERT_NULL(obj);
	obj = bbus_object_from_buf(good, 21);
	ASSERT_NOT_NULL(obj);
END

DEFINE_TEST(parse_object)
BEGIN
	static const char const objbuf[] = "ius\0\x11\x22\x33\x44\x44"
						"\x33\x22\x11somethin\0";

	bbus_object* obj;
	bbus_int i;
	bbus_unsigned u;
	bbus_byte* s;
	int r;

	obj = bbus_object_from_buf(objbuf, 21);
	ASSERT_NOT_NULL(obj);
	r = bbus_parse_object(obj, "ius", &i, &u, &s);
	ASSERT_EQ(0, r);
	ASSERT_EQ(0x11223344, i);
	ASSERT_EQ(0x44332211, u);
	ASSERT_STREQ((char*)s, "somethin");
END

/**************************************
 * \TESTS
 **************************************/

static int run_all_tests(void)
{
	struct testlist_elem* e;
	int r;

	print("######################################\n");
	print("####> Starting busybus testsuite <####\n");
	print("######################################\n");
	e = tests_head;
	while (e != NULL) {
		print("Running test:\t[%s]\n", e->name);
		r = e->testfunc();
		if (r < 0) {
			print("\t[%s]: TEST FAILED\n", e->name);
			tests_failed++;
		}
		tests_run++;
		e = e->next;
	}
	print("SUMMARY:\n");
	print("Tests run:\t%d\n", tests_run);
	if (tests_failed > 0) {
		print("Tests failed:\t%d\n", tests_failed);
		return -1;
	} else {
		print("All tests PASSED!\n");
		return 0;
	}
}

int main(int argc, char** argv)
{
	REGISTER_TEST(make_object);
	REGISTER_TEST(validate_object_format);
	REGISTER_TEST(parse_object);
	return run_all_tests();
}



