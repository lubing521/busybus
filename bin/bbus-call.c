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

static char* method = NULL;
static char* argdescr = NULL;
static char** argstart = NULL;
static char** argend = NULL;

static parse_args(int argc, char** argv)
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

	for (index = optind; index < argc; ++index) {
		if (method == NULL) {
			method = argv[index];
		} else
		if (argdescr == NULL) {
			argdescr = argv[index];
		} else {
			if ((argc - index) != strlen(argdescr)) {
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

	conn = bbus_client_connect();
	if (conn == NULL)
		goto err_conn;

	arg = bbus_empty_object();
	if (arg == NULL)
		goto err_arg;

	r = bbus_obj_setdescr(arg, argdescr);
	if (r < 0)
		goto err_arg;

	for (curarg = argstart; argdescr != NULL; ++argdescr, ++curarg) {
		switch (*argdescr) {
		/* TODO other formats */
		case BBUS_TYPE_STRING:
			r = bbus_obj_insert_string(arg, (bbus_byte*)curarg);
			if (r < 0)
				goto err_arg;
			break;
		default:
			bbus_close_client_conn(conn);
			bbus_free_object(arg);
			die("Invalid argument description character: %c\n",
					*argdescr);
			break;
		}
	}

	ret = bbus_call_method(conn, method, arg);
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
	ret = bbus_obj_repr(ret, reprbuf, BUFSIZ);
	if (ret < 0)
		goto err_repr;
	fprintf(stdout, "%s\n", reprbuf);

	return 0;

err_conn:
	die("Error connecting to bbusd: %s\n",
			bbus_strerror(bbus_lasterror()));

err_arg:
	bbus_close_client_conn(conn);
	bbus_free_object(arg);
	die("Error creating the argument object: %s\n",
			bbus_strerror(bbus_lasterror()));

err_call:
	bbus_close_client_conn(conn);
	bbus_free_object(arg);
	die("Error calling method \'%s\': %s\n", method,
			bbus_strerror(bbus_lasterror()));

err_repr:
	bbus_close_client_conn(conn);
	bbus_free_object(arg);
	bbus_free_object(ret);
	die("Error creating object representation: %s\n",
			bbus_strerror(bbus_lasterror()));
}

