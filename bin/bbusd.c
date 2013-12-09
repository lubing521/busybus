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
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include "bbusd/log.h"
#include "bbusd/common.h"
#include "bbusd/service.h"
#include "bbusd/methods.h"
#include "bbusd/msgbuf.h"
#include "bbusd/clientlist.h"
#include "bbusd/clients.h"
#include "bbusd/callers.h"
#include "bbusd/monitor.h"

static volatile int run;

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

static struct bbus_opt_list optlist = {
	.opts = cmdopts,
	.numopts = BBUS_ARRAY_SIZE(cmdopts),
	.progname = "Busybus",
	.version = "ALPHA",
	.progdescr = "Tiny message bus daemon."
};

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

static char* mname_from_srvcname(const char* srvc)
{
	char* found;

	found = rindex(srvc, '.');
	if (found != NULL)
		++found;
	return found;
}

static int handle_clientcall(bbus_client* cli, struct bbus_msg* msg)
{
	struct bbusd_method* mthd;
	const char* mname;
	int ret;
	bbus_object* argobj = NULL;
	bbus_object* retobj = NULL;
	struct bbus_msg_hdr hdr;
	char* meta;

	mname = bbus_prot_extractmeta(msg);
	if (mname == NULL)
		return -1;

	memset(&hdr, 0, sizeof(struct bbus_msg_hdr));
	mthd = bbusd_locate_method(mname);
	if (mthd == NULL) {
		bbusd_logmsg(BBUS_LOG_ERR, "No such method: %s\n", mname);
		bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY,
					BBUS_PROT_ENOMETHOD);
		ret = -1;
		goto respond;
	}

	argobj = bbus_prot_extractobj(msg);
	if (argobj == NULL)
		return -1;

	if (mthd->type == BBUSD_METHOD_LOCAL) {
		retobj = ((struct bbusd_local_method*)mthd)->func(argobj);
		if (retobj == NULL) {
			bbusd_logmsg(BBUS_LOG_ERR, "Error calling method.\n");
			bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY,
					BBUS_PROT_EMETHODERR);
		} else {
			bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY,
					BBUS_PROT_EGOOD);
			bbus_hdr_setpsize(&hdr, bbus_obj_rawsize(retobj));
		}

		goto respond;
	} else
	if (mthd->type == BBUSD_METHOD_REMOTE) {
		meta = mname_from_srvcname(mname);
		if (meta == NULL) {
			bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY,
					BBUS_PROT_EMETHODERR);
			goto respond;
		}
		bbus_hdr_build(&hdr, BBUS_MSGTYPE_SRVCALL, BBUS_PROT_EGOOD);
		BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASMETA);
		BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASOBJECT);
		bbus_hdr_setpsize(&hdr, (uint16_t)(strlen(meta) + 1
						+ bbus_obj_rawsize(argobj)));
		bbus_hdr_settoken(&hdr, bbus_client_gettoken(cli));

		ret = bbus_client_sendmsg(
				((struct bbusd_remote_method*)mthd)->srvc->cli,
				&hdr, meta, argobj);
		if (ret < 0) {
			bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY,
					BBUS_PROT_EMETHODERR);
			goto respond;
		}
	} else {
		bbusd_die("Internal logic error, invalid method type\n");
	}

	goto dontrespond;

respond:
	ret = bbus_client_sendmsg(cli, &hdr, NULL, retobj);
	if (ret < 0) {
		bbusd_logmsg(BBUS_LOG_ERR,
				"Error sending reply to client: %s\n",
				bbus_strerror(bbus_lasterror()));
		ret = -1;
	}

	bbus_obj_free(retobj);

dontrespond:
	bbus_obj_free(argobj);
	return ret;
}

static int register_service(struct bbusd_clientlist_elem* cli,
						struct bbus_msg* msg)
{
	const char* extrmeta;
	char* meta;
	int ret;
	char* comma;
	char* path;
	struct bbusd_remote_method* mthd;
	struct bbus_msg_hdr hdr;

	extrmeta = bbus_prot_extractmeta(msg);
	if (extrmeta == NULL) {
		ret = -1;
		goto respond;
	}

	meta = bbus_str_cpy(extrmeta);
	if (meta == NULL) {
		ret = -1;
		goto metafree;
	}

	comma = index(meta, ',');
	if (comma == NULL) {
		ret = -1;
		goto metafree;
	}
	*comma = '\0';

	path = bbus_str_build("bbus.%s", meta);
	if (path == NULL) {
		ret = -1;
		goto metafree;
	}

	mthd = bbus_malloc0(sizeof(struct bbusd_remote_method));
	if (mthd == NULL) {
		ret = -1;
		goto pathfree;
	}

	mthd->type = BBUSD_METHOD_REMOTE;
	mthd->srvc = cli;

	ret = bbusd_insert_method(path, (struct bbusd_method*)mthd);
	if (ret < 0) {
		ret = -1;
		goto mthdfree;
	} else {
		bbusd_logmsg(BBUS_LOG_INFO,
			"Method '%s' successfully registered.\n", path);
		ret = 0;
		goto pathfree;
	}

mthdfree:
	bbus_free(mthd);

pathfree:
	bbus_str_free(path);

metafree:
	bbus_str_free(meta);

respond:
	bbus_hdr_build(&hdr, BBUS_MSGTYPE_SRVACK, ret == 0
				? BBUS_PROT_EGOOD : BBUS_PROT_EMREGERR);
	ret = bbus_client_sendmsg(cli->cli, &hdr, NULL, NULL);
	if (ret < 0) {
		bbusd_logmsg(BBUS_LOG_ERR,
				"Error sending reply to client: %s\n",
				bbus_strerror(bbus_lasterror()));
		ret = -1;
	}

	return ret;
}

static int unregister_service(bbus_client* cli BBUS_UNUSED,
		const struct bbus_msg* msg BBUS_UNUSED)
{
	return 0;
}

static int handle_control_message(bbus_client* cli BBUS_UNUSED,
		const struct bbus_msg* msg BBUS_UNUSED)
{
	return 0;
}

static int pass_srvc_reply(bbus_client* srvc BBUS_UNUSED, struct bbus_msg* msg)
{
	struct bbus_msg_hdr hdr;
	struct bbusd_clientlist_elem* cli;
	bbus_object* obj;
	int ret;

	cli = bbusd_get_caller(bbus_hdr_gettoken(&msg->hdr));
	if (cli == NULL) {
		bbusd_logmsg(BBUS_LOG_ERR, "Caller not found for reply.\n");
		return -1;
	}

	obj = bbus_prot_extractobj(msg);
	if (obj == NULL) {
		bbusd_logmsg(BBUS_LOG_ERR,
			"Error extracting the object from message: %s\n",
			bbus_strerror(bbus_lasterror()));
		bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY,
					BBUS_PROT_EMETHODERR);
		goto respond;
	}

	bbus_hdr_build(&hdr, BBUS_MSGTYPE_CLIREPLY, BBUS_PROT_EGOOD);
	BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASOBJECT);
	bbus_hdr_setpsize(&hdr, bbus_obj_rawsize(obj));

respond:
	ret = bbus_client_sendmsg(cli->cli, &hdr, NULL, obj);
	if (ret < 0) {
		bbusd_logmsg(BBUS_LOG_ERR,
			"Error sending server reply to client: %s\n",
			bbus_strerror(bbus_lasterror()));
		ret = -1;
	}

	bbus_obj_free(obj);
	return ret;
}

static unsigned make_token(void)
{
	static unsigned curtok = 0;

	if (curtok == UINT_MAX)
		curtok = 0;

	return ++curtok;
}

static int client_auth(const struct bbus_client_cred* cred)
{
	char cliname[256];

	memset(cliname, 0, sizeof(cliname));
	(void)bbus_proc_pidtoname(cred->pid, cliname, sizeof(cliname));
	bbusd_logmsg(BBUS_LOG_INFO,
			"Client credentials: pid: %u, uid: %u, gid: %u\n",
			cred->pid, cred->uid, cred->gid);
	bbusd_logmsg(BBUS_LOG_INFO, "Client name: %s\n", cliname);

	return BBUS_SRV_AUTHOK;
}

static void accept_client(bbus_server* server)
{
	bbus_client* cli;
	int r;
	unsigned token;

	/* TODO Client credentials verification. */
	cli = bbus_srv_accept(server, client_auth);
	if (cli == NULL) {
		bbusd_logmsg(BBUS_LOG_ERR,
			"Error accepting incoming client "
			"connection: %s\n",
			bbus_strerror(bbus_lasterror()));
		return;
	}
	bbusd_logmsg(BBUS_LOG_INFO, "Client connected.\n");

	r = bbusd_clientlist_add(cli);
	if (r < 0) {
		bbusd_logmsg(BBUS_LOG_ERR,
			"Error adding new client to the list: %s\n",
			bbus_strerror(bbus_lasterror()));
		return;
	}

	switch (bbus_client_gettype(cli)) {
	case BBUS_CLIENT_CALLER:
		token = make_token();
		bbus_client_settoken(cli, token);
		/* This client is the list's tail at this point. */
		r = bbusd_add_caller(token, bbusd_clientlist_getlast());
		if (r < 0) {
			bbusd_logmsg(BBUS_LOG_ERR,
				"Error adding new client to "
				"the caller map: %s\n",
				bbus_strerror(bbus_lasterror()));
		}
		break;
	case BBUS_CLIENT_MON:
		r = bbusd_monlist_add(cli);
		if (r < 0) {
			bbusd_logmsg(BBUS_LOG_ERR,
				"Error adding new monitor to "
				"the list: %s\n",
				bbus_strerror(bbus_lasterror()));
			return;
		}
		break;
	case BBUS_CLIENT_SERVICE:
	case BBUS_CLIENT_CTL:
		/*
		 * Don't do anything else other than adding these
		 * clients to the main client list.
		 */
		break;
	default:
		break;
	}
}

/*
 * Returns -1 if client connection shall be closed after the function call,
 * and 0 if it must be kept active.
 */
static int handle_client(struct bbusd_clientlist_elem* cli_elem)
{
	bbus_client* cli;
	int r;

	cli = cli_elem->cli;
	bbusd_zeromsgbuf();
	r = bbus_client_rcvmsg(cli, bbusd_getmsgbuf(), bbusd_msgbufsize());
	if (r < 0) {
		bbusd_logmsg(BBUS_LOG_ERR,
			"Error receiving message from client: %s\n",
			bbus_strerror(bbus_lasterror()));
		goto cli_close;
	}

	bbusd_send_to_monitors(bbusd_getmsgbuf());

	/* TODO Common function for error reporting. */
	switch (bbus_client_gettype(cli)) {
	case BBUS_CLIENT_CALLER:
		switch (bbusd_getmsgbuf()->hdr.msgtype) {
		case BBUS_MSGTYPE_CLICALL:
			r = handle_clientcall(cli, bbusd_getmsgbuf());
			if (r < 0) {
				bbusd_logmsg(BBUS_LOG_ERR,
					"Error on client call\n");
				goto cli_close;
			}
			break;
		case BBUS_MSGTYPE_CLOSE:
			goto cli_close;
			break;
		default:
			bbusd_logmsg(BBUS_LOG_ERR,
					"Unexpected message received.\n");
			goto cli_close;
			break;
		}
		break;
	case BBUS_CLIENT_SERVICE:
		switch (bbusd_getmsgbuf()->hdr.msgtype) {
		case BBUS_MSGTYPE_SRVREG:
			r = register_service(cli_elem, bbusd_getmsgbuf());
			if (r < 0) {
				bbusd_logmsg(BBUS_LOG_ERR,
					"Error registering a service\n");
				goto out;
			}
			break;
		case BBUS_MSGTYPE_SRVUNREG:
			r = unregister_service(cli, bbusd_getmsgbuf());
			if (r < 0) {
				bbusd_logmsg(BBUS_LOG_ERR,
					"Error unregistering a service: %s\n",
					bbus_strerror(bbus_lasterror()));
				goto out;
			}
			break;
		case BBUS_MSGTYPE_SRVREPLY:
			r = pass_srvc_reply(cli, bbusd_getmsgbuf());
			if (r < 0) {
				bbusd_logmsg(BBUS_LOG_ERR,
					"Error passing a service reply: %s\n",
					bbus_strerror(bbus_lasterror()));
				goto out;
			}
			break;
		case BBUS_MSGTYPE_CLOSE:
			goto cli_close;
			break;
		default:
			bbusd_logmsg(BBUS_LOG_ERR,
					"Unexpected message received.\n");
			goto cli_close;
			goto out;
		}
		break;
	case BBUS_CLIENT_CTL:
		switch (bbusd_getmsgbuf()->hdr.msgtype) {
		case BBUS_MSGTYPE_CTRL:
			handle_control_message(cli, bbusd_getmsgbuf());
			break;
		case BBUS_MSGTYPE_CLOSE:
			goto cli_close;
			break;
		default:
			bbusd_logmsg(BBUS_LOG_ERR,
					"Unexpected message received.\n");
			goto out;
		}
		break;
	case BBUS_CLIENT_MON:
		switch (bbusd_getmsgbuf()->hdr.msgtype) {
		case BBUS_MSGTYPE_CLOSE:
			bbusd_monlist_rm(cli);
			goto cli_close;
			break;
		default:
			bbusd_logmsg(BBUS_LOG_WARN,
				"Message received from a monitor which should "
				"not be sending any messages - discarding.\n");
			goto cli_close;
			break;
		}
		break;
	default:
		bbusd_logmsg(BBUS_LOG_ERR,
			"Unhandled client type in the received message.\n");
		goto out;
	}

out:
	return 0;

cli_close:
	return -1;
}

static void poll_and_handle_inbound_traffic(bbus_server* server,
						bbus_pollset* pollset)
{
	int retval;
	int numclients;
	struct bbusd_clientlist_elem* tmpcli;
	struct bbusd_clientlist_elem* cli_rm;
	struct bbus_timeval tv;

	memset(&tv, 0, sizeof(struct bbus_timeval));
	bbus_pollset_clear(pollset);
	bbus_pollset_addsrv(pollset, server);
	for (tmpcli = bbusd_clientlist_getfirst(); tmpcli != NULL;
			tmpcli = tmpcli->next) {
		bbus_pollset_addcli(pollset, tmpcli->cli);
	}
	tv.sec = 0;
	tv.usec = 500000;
	retval = bbus_poll(pollset, &tv);
	if (retval < 0) {
		if (bbus_lasterror() == BBUS_EPOLLINTR) {
			return;
		} else {
			bbusd_die("Error polling connections: %s",
					bbus_strerror(bbus_lasterror()));
		}
	} else
	if (retval == 0) {
		/* Timeout. */
		return;
	} else {
		/* Incoming data. */
		numclients = retval;
		if (bbus_pollset_srvisset(pollset, server)) {
			while (bbus_srv_clientpending(server)) {
				accept_client(server);
			}
			--numclients;
		}

		tmpcli = bbusd_clientlist_getfirst();
		while (numclients > 0) {
			if (bbus_pollset_cliisset(pollset, tmpcli->cli)) {
				retval = handle_client(tmpcli);
				--numclients;
			}

			if (retval == 0) {
				tmpcli = tmpcli->next;
			} else {
				bbus_client_close(tmpcli->cli);
				bbus_client_free(tmpcli->cli);
				cli_rm = tmpcli;
				tmpcli = tmpcli->next;
				bbusd_clientlist_rm(&cli_rm);
				bbusd_logmsg(BBUS_LOG_INFO,
						"Client disconnected.\n");
			}
		}
	}
}

int main(int argc, char** argv)
{
	int retval;
	struct bbusd_clientlist_elem* tmpcli;
	static bbus_pollset* pollset;
	bbus_server* server;

	retval = bbus_parse_args(argc, argv, &optlist, NULL);
	if (retval == BBUS_ARGS_HELP)
		return EXIT_SUCCESS;
	else if (retval == BBUS_ARGS_ERR)
		return EXIT_FAILURE;

	bbusd_init_caller_map();
	bbusd_init_service_map();
	bbusd_register_local_methods();

	/* Creating the server object. */
	server = bbus_srv_create();
	if (server == NULL) {
		bbusd_die("Error creating the server object: %s\n",
			bbus_strerror(bbus_lasterror()));
	}

	retval = bbus_srv_listen(server);
	if (retval < 0) {
		bbusd_die("Error opening server for connections: %s\n",
			bbus_strerror(bbus_lasterror()));
	}

	pollset = bbus_pollset_make();
	if (pollset == NULL) {
		bbusd_die("Error creating the poll_set: %s\n",
			bbus_strerror(bbus_lasterror()));
	}

	bbusd_logmsg(BBUS_LOG_INFO, "Busybus daemon starting!\n");
	run = 1;
	(void)signal(SIGTERM, sighandler);
	(void)signal(SIGINT, sighandler);
	/* TODO Ignore SIGPIPE on socket level */
	(void)signal(SIGPIPE, SIG_IGN);

	/*
	 * MAIN LOOP
	 */
	while (do_run()) {
		poll_and_handle_inbound_traffic(server, pollset);
	}

	/* Cleanup. */
	bbus_srv_close(server);

	for (tmpcli = bbusd_clientlist_getfirst(); tmpcli != NULL;
					tmpcli = tmpcli->next) {
		bbus_client_close(tmpcli->cli);
		bbus_client_free(tmpcli->cli);
		bbus_free(tmpcli);
	}

	bbusd_free_service_map();

	bbusd_logmsg(BBUS_LOG_INFO, "Busybus daemon exiting!\n");
	return EXIT_SUCCESS;
}

