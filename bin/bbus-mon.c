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

#include <busybus.h>
#include <stdio.h>
#include <signal.h>

static unsigned char __msgbuf[BBUS_MAXMSGSIZE];
static struct bbus_msg* msgbuf = (struct bbus_msg*)__msgbuf;
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

int main(int argc BBUS_UNUSED, char** argv BBUS_UNUSED)
{
	bbus_client_connection* conn;
	int ret;
	struct bbus_timeval tv;
	bbus_object* obj;
	const char* meta;
	char buf[1024];

	conn = bbus_mon_connect();
	if (conn == NULL) {
		die("Error connecting to bbusd: %s\n",
			bbus_strerror(bbus_lasterror()));
	}

	(void)signal(SIGTERM, sighandler);
	(void)signal(SIGINT, sighandler);

	run = 1;
	while (do_run()) {
		tv.sec = 0;
		tv.usec = 500000;

		ret = bbus_mon_recvmsg(conn, msgbuf, BBUS_MAXMSGSIZE,
							&tv, &obj, &meta);
		if (ret < 0) {
			if (bbus_lasterror() == BBUS_EPOLLINTR) {
				continue;
			} else {
				die("Error receiving message: %s\n",
					bbus_strerror(bbus_lasterror()));
			}
		} else
		if (ret == 0) {
			/* Timeout. */
			continue;
		} else {
			ret = bbus_obj_repr(obj, "bbbuubs", buf, sizeof(buf));
			if (ret < 0) {
				die("Error building the object "
					"representation: %s\n",
					bbus_strerror(bbus_lasterror()));
			}
			bbus_obj_free(obj);
			fprintf(stdout, "%s %s\n", meta, buf);
		}
	}

	bbus_closeconn(conn);
	fprintf(stdout, "\n");

	return 0;
}

