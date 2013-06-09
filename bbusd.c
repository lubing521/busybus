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
#include <stdarg.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>

static bbus_server* server;
static struct bbus_clientlist_elem* clients;
static bbus_pollset* pollset;
static int run;

struct option_flags
{
	int print_help;
	int print_version;
};
struct option_flags options = { 0, 0 };

static void die(const char* format, ...) BBUS_PRINTF_FUNC(1, 2);
static void die(const char* format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	exit(EXIT_FAILURE);
}

static void print_help_and_exit(void)
{
	fprintf(stdout, "Help stub\n");
	exit(EXIT_SUCCESS);
}

static void print_version_and_exit(void)
{
	fprintf(stdout, "Version stub\n");
	exit(EXIT_SUCCESS);
}

static void parse_args(int argc, char** argv)
{
	static const struct option longopts[] = {
		{ "help", no_argument, &options.print_help, 1 },
		{ "version", no_argument, &options.print_version, 1 },
		{ "sockpath", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	static const char* const shortopts = "s";

	int opt, optind;

	while ((opt = getopt_long(argc, argv, shortopts,
				longopts, &optind)) != -1) {
		switch (opt) {
		case 's':
			bbus_setsockpath(optarg);
			break;
		case '?':
			break;
		case 0:
			break;
		default:
			die("Invalid argument\n");
			break;
		}
	}
}

static void make_client_list(void)
{
	clients = bbus_make_client_list();
	if (clients == NULL) {
		die("Error initiating the client list: %s",
			bbus_error_str(bbus_get_last_error()));
	}
}

static void make_server(void)
{
	server = bbus_make_local_server(DEF_SOCKPATH);
	if (server == NULL) {
		die("Error creating the server object: %s",
			bbus_error_str(bbus_get_last_error()));
	}
}

static void start_server_listen(void)
{
	int retval;

	retval = bbus_server_listen(server);
	if (retval < 0) {
		die("Error opening server for connections",
			bbus_error_str(bbus_get_last_error()));
	}
}

static void make_pollset(void)
{
	pollset = bbus_make_pollset(server);
	if (pollset == NULL) {
		die("Error creating the poll_set: %s",
			bbus_error_str(bbus_get_last_error()));
	}
}

static int do_run(void)
{
	return __sync_fetch_and_or(&run, 0);
}

static void do_stop(void)
{
	__sync_lock_test_and_set(&run, 0);
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

static void run_main_loop(void)
{
	int retval;
	struct bbus_clientlist_elem* tmpcli;
	struct bbus_timeval tv;

	run = 1;
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);
	while (do_run()) {
		memset(&tv, 0, sizeof(struct bbus_timeval));
		bbus_clear_pollset(pollset);
		bbus_pollset_add_srv(pollset, server);
		tmpcli = clients;

		while (tmpcli != NULL) {
			bbus_pollset_add_client(pollset, tmpcli->cli);
			tmpcli = tmpcli->next;
		}

		tv.sec = 0;
		tv.usec = 500000;
		retval = bbus_poll(pollset, &tv);
		if (retval < 0) {
			die("Error polling connections: %s",
				bbus_error_str(bbus_get_last_error()));
		} else
		if (retval == 0) {
			// Timeout
			continue;
		} else {
			// Incoming data
			if (bbus_pollset_srv_isset(pollset, server)) {
				accept_clients();
			} else {
				handle_clients();
			}
		}
	}
}

static void accept_clients(void)
{

}

static void handle_clients(void)
{

}

static void cleanup(void)
{

}

int main(int argc, char** argv)
{
	parse_args(argc, argv);

	if (options.print_help)
		print_help_and_exit();
	if (options.print_version)
		print_version_and_exit();

	make_client_list();
	make_server();
	start_server_listen();
	make_pollset();
	run_main_loop();
	cleanup();

	return 0;
}


