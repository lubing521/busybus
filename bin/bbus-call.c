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
#include <getopt.h>
#include <stdio.h>
#include <string.h>

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
			sockpath = optarg;
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
				argstart = &argv[index];
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

	conn = bbus_connect();
	if (conn == NULL)
		goto err_conn;

	arg = bbus_obj_mkempty();
	if (arg == NULL)
		goto err_arg;

	r = bbus_obj_setdescr(arg, argdescr);
	if (r < 0)
		goto err_arg;

	for (curarg = argstart; argdescr != NULL; ++argdescr, ++curarg) {
		switch (*argdescr) {
		/* TODO other formats */
		case BBUS_TYPE_STRING:
			r = bbus_obj_insstr(arg, (bbus_byte*)curarg);
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
		if (bbus_lasterror() == BBUS_NOMETHOD) {
			/*
			 * TODO check if it's a service and display
			 * available methods.
			 */
		} else {
			goto err_call;
		}
	}

	memset(reprbuf, 0, BUFSIZ);
	r = bbus_obj_repr(ret, reprbuf, BUFSIZ);
	if (r < 0)
		goto err_repr;
	fprintf(stdout, "%s\n", reprbuf);

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

