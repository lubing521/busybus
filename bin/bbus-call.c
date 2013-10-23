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
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

struct option_flags
{
	int print_help;
	int print_version;
};

static char* sockpath = BBUS_DEF_DIRPATH BBUS_DEF_SOCKNAME;
static char* method = NULL;
static char* argdescr = NULL;
static char** argstart = NULL;
static char** argend = NULL;
static struct option_flags options = { 0, 0 };

static void BBUS_PRINTF_FUNC(1, 2) BBUS_NORETURN die(const char* format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	exit(EXIT_FAILURE);
}

static void parse_args(int argc, char** argv)
{
	static const struct option longopts[] = {
		{ "help", no_argument, &options.print_help, 1 },
		{ "version", no_argument, &options.print_version, 1 },
		{ "sockpath", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	static const char* const shortopts = "s:";

	int opt, index;

	opterr = 0;
	while ((opt = getopt_long(argc, argv, shortopts,
				longopts, &index)) != -1) {
		switch (opt) {
		case 's':
			sockpath = optarg;
			break;
		case 0:
			/* Do nothing - we have a longopt. */
			break;
		case '?':
		default:
			die("Invalid arguments! Try %s --help\n", argv[0]);
			break;
		}
	}

	for (index = optind; index < argc; ++index) {
		if (method == NULL) {
			method = argv[index];
		} else
		if (argdescr == NULL) {
			argdescr = argv[index];
		} else {
			if ((size_t)(argc - index) != strlen(argdescr)) {
				die("Need to pass arguments matching "
						"the description");
			}

			if (argstart == NULL) {
				argstart = argend = &argv[index];
			} else {
				argend = &argv[index];
			}
		}
	}

	if (!method || !argdescr || !argstart || !argend)
		die("Invalid method call format\n");
}

int main(int argc, char** argv)
{
	bbus_client_connection* conn;
	bbus_object* arg;
	bbus_object* ret;
	int r;
	char** curarg;
	char reprbuf[BUFSIZ];

	parse_args(argc, argv);
	(void)signal(SIGPIPE, SIG_IGN);

	conn = bbus_connect();
	if (conn == NULL)
		goto err_conn;

	arg = bbus_obj_mkempty();
	if (arg == NULL)
		goto err_arg;

	r = bbus_obj_setdescr(arg, argdescr);
	if (r < 0)
		goto err_arg;

	for (curarg = argstart; *argdescr != '\0'; ++argdescr, ++curarg) {
		switch (*argdescr) {
		/* TODO other formats */
		case BBUS_TYPE_STRING:
			r = bbus_obj_insstr(arg, *curarg);
			if (r < 0)
				goto err_arg;
			break;
		default:
			bbus_closeconn(conn);
			bbus_obj_free(arg);
			die("Invalid argument description character: %c\n",
					*argdescr);
			break;
		}
	}

	ret = bbus_callmethod(conn, method, arg);
	if (ret == NULL) {
		if (bbus_lasterror() == BBUS_ENOMETHOD) {
			/*
			 * TODO check if it's a service and display
			 * available methods.
			 */
			goto err_call;
		} else {
			goto err_call;
		}
	}

	memset(reprbuf, 0, BUFSIZ);
	r = bbus_obj_repr(ret, reprbuf, BUFSIZ);
	if (r < 0)
		goto err_repr;
	fprintf(stdout, "%s\n", reprbuf);

	bbus_closeconn(conn);
	bbus_obj_free(arg);
	bbus_obj_free(ret);

	return 0;

err_conn:
	die("Error connecting to bbusd: %s\n",
			bbus_strerror(bbus_lasterror()));

err_arg:
	bbus_closeconn(conn);
	bbus_obj_free(arg);
	die("Error creating the argument object: %s\n",
			bbus_strerror(bbus_lasterror()));

err_call:
	bbus_closeconn(conn);
	bbus_obj_free(arg);
	die("Error calling method \'%s\': %s\n", method,
			bbus_strerror(bbus_lasterror()));

err_repr:
	bbus_closeconn(conn);
	bbus_obj_free(arg);
	bbus_obj_free(ret);
	die("Error creating object representation: %s\n",
			bbus_strerror(bbus_lasterror()));
}

