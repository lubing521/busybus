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

#include <busybus.h>
#include "bbus-unit.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

struct testlist
{
	struct bbusunit_listelem* head;
	struct bbusunit_listelem* tail;
};

static unsigned tests_run = 0;
static unsigned tests_failed = 0;
static struct testlist tests;

static BBUS_ATSTART_FIRST void testlist_init(void)
{
	tests.head = NULL;
	tests.tail = NULL;
}

void bbusunit_registertest(struct bbusunit_listelem* test)
{
	bbus_list_push(&tests, test);
}

#define PRINT_FROM_VA(STREAM, HDR, FMT)					\
	do {								\
		va_list va;						\
		va_start(va, FMT);					\
		fprintf(STREAM, HDR"\t");				\
		vfprintf(STREAM, FMT, va);				\
		fprintf(STREAM, "\n");					\
		va_end(va);						\
	} while (0)

static void BBUS_PRINTF_FUNC(1, 2) die(const char* fmt, ...)
{
	PRINT_FROM_VA(stderr, "[FATAL]\t", fmt);
	exit(EXIT_FAILURE);
}

void bbusunit_print(const char* fmt, ...)
{
	PRINT_FROM_VA(stdout, "[INFO]", fmt);
}

void bbusunit_printerr(const char* fmt, ...)
{
	PRINT_FROM_VA(stderr, "[ERROR]\t", fmt);
}

static int testList(void)
{
	/* TODO Add tests for doubly-linked lists. */
	return 0;
}

int main(int argc BBUS_UNUSED, char** argv BBUS_UNUSED)
{
	struct bbusunit_listelem* el;
	int r;

	bbusunit_print("##############################");
	bbusunit_print("####> Busybus unit-tests <####");
	bbusunit_print("##############################");
	bbusunit_print("Performing self-tests...");
	r = testList();
	if (r < 0)
		die("Doubly-linked lists do not work as expected.");
	bbusunit_print("Done!");
	bbusunit_print("Running tests...");
	el = tests.head;
	while (el != NULL) {
		bbusunit_print("Case:\t[%s]", el->name);
		r = el->testfunc();
		if (r < 0) {
			bbusunit_printerr("[%s]: TEST FAILED", el->name);
			tests_failed++;
		}
		tests_run++;
		el = el->next;
	}
	bbusunit_print("All done!");
	bbusunit_print("SUMMARY:");
	bbusunit_print("  Tests run:\t%d", tests_run);
	if (tests_failed > 0) {
		bbusunit_print("  Tests failed:\t%d", tests_failed);
		r = EXIT_FAILURE;
	} else {
		bbusunit_print("  All tests PASSED!");
		r = EXIT_SUCCESS;
	}

	return r;
}

