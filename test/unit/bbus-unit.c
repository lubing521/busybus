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

#include <busybus.h>
#include "bbus-unit.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

struct testlist
{
	struct bbusunit_listelem* head;
	struct bbusunit_listelem* tail;
};

static struct testlist tests;
static unsigned tests_registered = 0;

static BBUS_ATSTART_FIRST void testlist_init(void)
{
	tests.head = NULL;
	tests.tail = NULL;
}

void bbusunit_registertest(struct bbusunit_listelem* test)
{
	if (tests.tail == NULL) {
		tests.head = tests.tail = test;
		test->next = NULL;
	} else {
		tests.tail->next = test;
		test->next = NULL;
		tests.tail = test;
	}
	++tests_registered;
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

void bbusunit_print(const char* fmt, ...)
{
	PRINT_FROM_VA(stdout, "[INFO]", fmt);
}

void bbusunit_printerr(const char* fmt, ...)
{
	PRINT_FROM_VA(stderr, "[ERROR]\t", fmt);
}

static void timeval_diff(struct timeval* res,
				const struct timeval* x,
				const struct timeval* y)
{
	struct timeval tmp;

	tmp.tv_sec = y->tv_sec;
	tmp.tv_usec = y->tv_usec;

	if (x->tv_usec < tmp.tv_usec) {
		int nsec = (tmp.tv_usec - x->tv_usec) / 1000000 + 1;
		tmp.tv_usec -= 1000000 * nsec;
		tmp.tv_sec += nsec;
	}

	if (x->tv_usec - tmp.tv_usec > 1000000) {
		int nsec = (x->tv_usec - tmp.tv_usec) / 1000000;
		tmp.tv_usec += 1000000 * nsec;
		tmp.tv_sec -= nsec;
	}

	res->tv_sec = x->tv_sec - tmp.tv_sec;
	res->tv_usec = x->tv_usec - tmp.tv_usec;
}

int main(int argc BBUS_UNUSED, char** argv BBUS_UNUSED)
{
	struct bbusunit_listelem* el;
	int r;
	unsigned tests_run = 0;
	unsigned tests_failed = 0;
	struct timeval begin;
	struct timeval end;
	struct timeval time_spent;

	memset(&begin, 0, sizeof(struct timeval));
	memset(&end, 0, sizeof(struct timeval));
	memset(&time_spent, 0, sizeof(struct timeval));

	bbusunit_print("##############################");
	bbusunit_print("####> Busybus unit-tests <####");
	bbusunit_print("##############################");
	bbusunit_print("%u tests registered.", tests_registered);
	bbusunit_print("Running tests...");
	el = tests.head;
	gettimeofday(&begin, NULL);
	while (el != NULL) {
		bbusunit_print("Case:\t[%s]", el->name);
		r = el->testfunc();
		if (r < 0) {
			bbusunit_printerr("[%s]: TEST FAILED", el->name);
			tests_failed++;
		}
		++tests_run;
		el = el->next;
	}
	gettimeofday(&end, NULL);
	timeval_diff(&time_spent, &end, &begin);
	bbusunit_print("All done!");
	bbusunit_print("SUMMARY:");
	bbusunit_print("  %u %s run in %d.%06d seconds.", tests_run,
			tests_run == 1 ? "test" : "tests",
			(int)time_spent.tv_sec, (int)time_spent.tv_usec);
	if (tests_failed > 0) {
		bbusunit_printerr("  %u %s FAILED", tests_failed,
				tests_failed == 1 ? "test" : "tests");
		bbusunit_printerr("  Failure rate: %.2f%%",
					((double)tests_failed/tests_run)*100);
		r = EXIT_FAILURE;
	} else {
		bbusunit_print("  All tests PASSED!");
		r = EXIT_SUCCESS;
	}

	return r;
}

