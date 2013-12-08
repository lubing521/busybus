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
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

static char* method = NULL;
static char* argdescr = NULL;
static char** argstart = NULL;
static char** argend = NULL;

static void BBUS_PRINTF_FUNC(1, 2) BBUS_NORETURN die(const char* format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	exit(EXIT_FAILURE);
}

static void opt_setsockpath(const char* path)
{
	bbus_prot_setsockpath(path);
}

static struct bbus_option cmdopts[] = {
	{
		.shortopt = 0,
		.longopt = "sockpath",
		.hasarg = BBUS_OPT_ARGREQ,
		.action = BBUS_OPTACT_CALLFUNC,
		.actdata = &opt_setsockpath,
		.descr = "path to the busybus socket",
	}
};

static struct bbus_posarg posargs[] = {
	{
		.action = BBUS_OPTACT_GETOPTARG,
		.actdata = &method,
		.descr = "name of the method to call",
	},
	{
		.action = BBUS_OPTACT_GETOPTARG,
		.actdata = &argdescr,
		.descr = "description string of the argument object",
	}
};

static struct bbus_opt_list optlist = {
	.opts = cmdopts,
	.numopts = BBUS_ARRAY_SIZE(cmdopts),
	.pargs = posargs,
	.numpargs = BBUS_ARRAY_SIZE(posargs),
	.progname = "Busybus",
	.version = "ALPHA",
	.progdescr = "bbus-call: call remote methods offered by service "
				"providers using the busybus protocol",
};

int main(int argc, char** argv)
{
	bbus_client_connection* conn;
	bbus_object* arg;
	bbus_object* ret;
	int r;
	char** curarg;
	char reprbuf[BUFSIZ];
	char* descr;
	struct bbus_nonopts* nonopts;

	r = bbus_parse_args(argc, argv, &optlist, &nonopts);
	if (r == BBUS_ARGS_HELP)
		return EXIT_SUCCESS;
	else if (r == BBUS_ARGS_ERR)
		return EXIT_FAILURE;

	if (nonopts->numargs != strlen(argdescr))
		die("Need to pass arguments matching the description\n");
	argstart = &nonopts->args[0];
	argend = &nonopts->args[nonopts->numargs - 1];

	(void)signal(SIGPIPE, SIG_IGN);

	conn = bbus_connect();
	if (conn == NULL)
		goto err_conn;

	arg = bbus_obj_alloc();
	if (arg == NULL)
		goto err_arg;

	descr = argdescr;
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
	r = bbus_obj_repr(ret, descr, reprbuf, BUFSIZ);
	if (r < 0)
		goto err_repr;
	fprintf(stdout, "%s\n", reprbuf);

	bbus_closeconn(conn);
	bbus_obj_free(arg);
	bbus_obj_free(ret);

	return EXIT_SUCCESS;

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

