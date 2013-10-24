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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

static volatile int run;

static void BBUS_PRINTF_FUNC(1, 2) BBUS_NORETURN die(const char* format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	exit(EXIT_FAILURE);
}

static int do_run(void)
{
	return BBUS_ATOMIC_GET(run);
}

static void do_stop(void)
{
	BBUS_ATOMIC_SET(run, 0);
}

static void sighandler(int signum)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		do_stop();
		break;
	}
}

static bbus_object* rm_echo(const char* name BBUS_UNUSED, bbus_object* arg)
{
	char* msg;
	int ret;

	ret = bbus_obj_parse(arg, "s", &msg);
	if (ret < 0)
		return NULL;
	else
		return bbus_obj_build("s", msg);
}

struct bbus_method echo_method = {
	.name = "echo",
	.argdscr = "s",
	.retdscr = "s",
	.func = rm_echo,
};

int main(int argc BBUS_UNUSED, char** argv BBUS_UNUSED)
{
	bbus_service_connection* conn;
	struct bbus_timeval tv;
	int ret;

	(void)signal(SIGTERM, sighandler);
	(void)signal(SIGINT, sighandler);
	(void)signal(SIGPIPE, SIG_IGN);

	conn = bbus_srvc_connect("echod");
	if (conn == NULL) {
		die("Error connecting to bbusd: %s\n",
				bbus_strerror(bbus_lasterror()));
	}

	ret = bbus_srvc_regmethod(conn, &echo_method);
	if (ret < 0) {
		die("Error registering method: %s\n",
				bbus_strerror(bbus_lasterror()));
	}

	run = 1;
	while (do_run())
	{
		tv.sec = 0;
		tv.usec = 500000;

		ret = bbus_srvc_listencalls(conn, &tv);
		if (ret < 0) {
			die("Error receiving a method call: %s\n",
					bbus_strerror(bbus_lasterror()));
		} else
		if (ret == 0) {
			/* Timeout. */
			continue;
		} else {
			fprintf(stderr, "Method properly called.\n");
		}
	}

	bbus_srvc_closeconn(conn);

	return 0;
}
