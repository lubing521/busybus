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
#include "common.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <syslog.h>

#define SYSLOG_IDENT "bbusd"

struct option_flags
{
	int print_help;
	int print_version;
	int log_to_console;
	int log_to_syslog;
};

enum loglevel
{
	BBUS_LOG_EMERG = LOG_EMERG,
	BBUS_LOG_ALERT = LOG_ALERT,
	BBUS_LOG_CRIT = LOG_CRIT,
	BBUS_LOG_ERR = LOG_ERR,
	BBUS_LOG_WARN = LOG_WARNING,
	BBUS_LOG_NOTICE = LOG_NOTICE,
	BBUS_LOG_INFO = LOG_INFO,
	BBUS_LOG_DEBUG = LOG_DEBUG
};

static bbus_server* server;
static struct bbus_clientlist_elem* clients_head;
static struct bbus_clientlist_elem* clients_tail;
static bbus_pollset* pollset;
static int run;
struct option_flags options = { 0, 0, 1, 0 };

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

static int loglvl_to_sysloglvl(enum loglevel lvl)
{
	return (int)lvl;
}

static void BBUS_PRINTF_FUNC(2, 3) log(enum loglevel lvl, const char* fmt, ...)
{
	va_list va;

	if (options.log_to_console) {
		va_start(va, fmt);
		switch (lvl) {
		case BBUS_LOG_EMERG:
		case BBUS_LOG_ALERT:
		case BBUS_LOG_CRIT:
		case BBUS_LOG_ERR:
		case BBUS_LOG_WARN:
			vfprintf(stderr, fmt, va);
			break;
		case BBUS_LOG_NOTICE:
		case BBUS_LOG_INFO:
		case BBUS_LOG_DEBUG:
			vfprintf(stdout, fmt, va);
			break;
		default:
			die("Invalid log level\n");
			break;
		}
		va_end(va);
	}

	if (options.log_to_syslog) {
		va_start(va, fmt);
		openlog(SYSLOG_IDENT, LOG_PID, LOG_DAEMON);
		vsyslog(loglvl_to_sysloglvl(lvl), fmt, va);
		closelog();
		va_end(va);
	}
}

static void parse_args(int argc, char** argv)
{
	static const struct option longopts[] = {
		{ "help", no_argument, &options.print_help, 1 },
		{ "version", no_argument, &options.print_version, 1 },
		{ "sockpath", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	static const char* const shortopts = "hvs:";

	int opt, index;

	while ((opt = getopt_long(argc, argv, shortopts,
				longopts, &index)) != -1) {
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

static void accept_clients(void)
{
	bbus_client* cli;

	while (bbus_server_has_pending_clients(server)) {
		cli = bbus_accept_client(server);
		if (cli == NULL) {
			log(BBUS_LOG_ERR,
				"Error accepting incoming client connection");
			continue;
		}

		insque(cli, clients_tail);
		switch (bbus_get_client_type(cli)) {
		case BBUS_CLIENT_CALLER:
			break;
		case BBUS_CLIENT_SERVICE:
			break;
		default:
			break;
		}
	}
}

static void handle_clients(void)
{

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
		for (tmpcli = clients_head; tmpcli != NULL;
				tmpcli = tmpcli->next) {
			bbus_pollset_add_client(pollset, tmpcli->cli);
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
			}
			handle_clients();
		}
	}
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


