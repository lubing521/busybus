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
#include "protocol.h"
#include "socket.h"
#include "error.h"
#include <string.h>

struct __bbus_client_connection
{
	int sock;
};

struct __bbus_service_connection
{
	int sock;
	char* srvname;
	bbus_hashmap* methods;
};

static int do_session_open(const char* path, int clitype)
{
	int r;
	struct bbus_msg_hdr hdr;
	int sock;

	sock = __bbus_sock_mksocket();
	if (sock < 0)
		goto errout;

	r = __bbus_sock_connect(sock, path);
	if (r < 0)
		goto errout;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	__bbus_prot_hdrsetmagic(&hdr);
	hdr.msgtype = BBUS_MSGTYPE_SO;
	hdr.sotype = clitype;
	r = __bbus_prot_sendvmsg(sock, &hdr, NULL, NULL, 0);
	if (r < 0)
		goto errout_close;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	r = __bbus_prot_recvvmsg(sock, &hdr, NULL, 0);
	if (r < 0)
		goto errout_close;

	if (hdr.msgtype == BBUS_MSGTYPE_SOOK) {
		return sock;
	} else
	if (hdr.msgtype == BBUS_MSGTYPE_SORJCT) {
		/* TODO Communicate the cause of rejections somehow. */
		__bbus_seterr(BBUS_ESORJCTD);
		goto errout_close;
	} else {
		__bbus_seterr(BBUS_EMSGINVTYPRCVD);
		goto errout_close;
	}

errout_close:
	__bbus_sock_close(sock);
errout:
	return -1;
}

static int send_session_close(int sock)
{
	int r;
	struct bbus_msg_hdr hdr;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	__bbus_prot_hdrsetmagic(&hdr);
	hdr.msgtype = BBUS_MSGTYPE_CLOSE;
	r = __bbus_prot_sendvmsg(sock, &hdr, NULL, NULL, 0);
	if (r < 0)
		return -1;
	r = __bbus_sock_close(sock);
	if (r < 0)
		return -1;
	return 0;
}

bbus_client_connection* bbus_connect(void)
{
	int sock;
	bbus_client_connection* conn;

	sock = do_session_open(bbus_prot_getsockpath(), BBUS_SOTYPE_MTHCL);
	if (sock < 0)
		return NULL;

	conn = bbus_malloc(sizeof(struct __bbus_client_connection));
	if (conn == NULL)
		return NULL;
	conn->sock = sock;
	return conn;
}

bbus_object* bbus_callmethod(bbus_client_connection* conn,
		char* method, bbus_object* arg)
{
	int r;
	struct bbus_msg_hdr hdr;
	size_t metasize;
	size_t objsize;
	char buf[BBUS_MAXMSGSIZE];

	metasize = strlen(method) + 1;
	objsize = bbus_obj_rawsize(arg);
	memset(&hdr, 0, sizeof(struct bbus_msg_hdr));
	__bbus_prot_hdrsetmagic(&hdr);
	hdr.msgtype = BBUS_MSGTYPE_CLICALL;
	bbus_hdr_setpsize(&hdr, metasize + objsize);
	BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASMETA);
	BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASOBJECT);

	r = __bbus_prot_sendvmsg(conn->sock, &hdr, method,
			bbus_obj_rawdata(arg), objsize);
	if (r < 0)
		return NULL;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	memset(buf, 0, BBUS_MAXMSGSIZE);
	r = __bbus_prot_recvvmsg(conn->sock, &hdr, buf,
			BBUS_MAXMSGSIZE - BBUS_MSGHDR_SIZE);
	if (r < 0)
		return NULL;

	if (hdr.msgtype == BBUS_MSGTYPE_CLIREPLY) {
		if (hdr.errcode != 0) {
			__bbus_seterr(__bbus_prot_errtoerrnum(hdr.errcode));
			return NULL;
		}
		return bbus_obj_frombuf(buf, BBUS_MAXMSGSIZE);
	} else {
		__bbus_seterr(BBUS_EMSGINVTYPRCVD);
		return NULL;
	}
}

/* TODO Refactor common code for bbus_connect and this. */
bbus_client_connection* bbus_mon_connect(void)
{
	int sock;
	bbus_client_connection* conn;

	sock = do_session_open(bbus_prot_getsockpath(), BBUS_SOTYPE_MON);
	if (sock < 0)
		return NULL;

	conn = bbus_malloc(sizeof(struct __bbus_client_connection));
	if (conn == NULL)
		return NULL;
	conn->sock = sock;
	return conn;
}

int bbus_mon_recvmsg(bbus_client_connection* conn,
		struct bbus_msg* msg, size_t bufsize,
		struct bbus_timeval* tv, bbus_object** obj, const char** meta)
{
	int r;

	r = __bbus_sock_rdready(conn->sock, tv);
	if (r <= 0) {
		return r;
	} else {
		memset(msg, 0, bufsize);
		r = __bbus_prot_recvmsg(conn->sock, msg, bufsize);
		if (r < 0)
			return -1;
		if (msg->hdr.msgtype != BBUS_MSGTYPE_MON) {
			__bbus_seterr(BBUS_EMSGINVTYPRCVD);
			return -1;
		}

		if (meta) {
			if (BBUS_HDR_ISFLAGSET(&msg->hdr, BBUS_PROT_HASMETA)) {
				*meta = bbus_prot_extractmeta(msg);
				if (*meta == NULL)
					return -1;
			}
		}

		if (!BBUS_HDR_ISFLAGSET(&msg->hdr, BBUS_PROT_HASOBJECT)) {
			__bbus_seterr(BBUS_EMSGINVTYPRCVD);
			return -1;
		}
		if (obj == NULL) {
			__bbus_seterr(BBUS_EINVALARG);
			return -1;
		}

		*obj = bbus_prot_extractobj(msg);
		if (*obj == NULL)
			return -1;
	}

	return 1;
}

int bbus_closeconn(bbus_client_connection* conn)
{
	int r;

	r = send_session_close(conn->sock);
	if (r < 0)
		r = -1;
	bbus_free(conn);

	return r;
}

bbus_service_connection* bbus_srvc_connect(const char* name)
{
	int sock;
	bbus_service_connection* conn;

	sock = do_session_open(bbus_prot_getsockpath(), BBUS_SOTYPE_SRVPRV);
	if (sock < 0)
		return NULL;

	conn = bbus_malloc(sizeof(struct __bbus_service_connection));
	if (conn == NULL)
		return NULL;
	conn->sock = sock;
	conn->srvname = bbus_str_cpy(name);
	conn->methods = bbus_hmap_create(BBUS_HMAP_KEYSTR);
	if (conn->methods == NULL) {
		__bbus_sock_close(conn->sock);
		bbus_str_free(conn->srvname);
		bbus_free(conn);
		return NULL;
	}
	return conn;
}

int bbus_srvc_regmethod(bbus_service_connection* conn,
		struct bbus_method* method)
{
	struct bbus_msg_hdr hdr;
	size_t metasize;
	char* meta;
	int r;

	metasize = strlen(conn->srvname) + 1; /* +1 for dot */
	metasize += strlen(method->name) + 1; /* +1 for comma */
	metasize += strlen(method->argdscr) + 1; /* +1 for comma */
	metasize += strlen(method->retdscr) + 1; /* +1 for NULL */
	memset(&hdr, 0, sizeof(struct bbus_msg_hdr));
	__bbus_prot_hdrsetmagic(&hdr);
	hdr.msgtype = BBUS_MSGTYPE_SRVREG;
	bbus_hdr_setpsize(&hdr, metasize);
	BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASMETA);
	meta = bbus_str_build("%s.%s,%s,%s",
					conn->srvname,
					method->name,
					method->argdscr,
					method->retdscr);
	if (meta == NULL) {
		return -1;
	} else
	if (strlen(meta) != (metasize-1)) {
		bbus_str_free(meta);
		__bbus_seterr(BBUS_ELOGICERR);
		return -1;
	}

	r = __bbus_prot_sendvmsg(conn->sock, &hdr, meta, NULL, 0);
	if (r < 0)
		return -1;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	r = __bbus_prot_recvvmsg(conn->sock, &hdr, NULL, 0);
	if (r < 0)
		return -1;

	if (hdr.msgtype != BBUS_MSGTYPE_SRVACK) {
		__bbus_seterr(BBUS_EMSGINVTYPRCVD);
		return -1;
	}
	if (hdr.errcode != 0) {
		__bbus_seterr(__bbus_prot_errtoerrnum(hdr.errcode));
		return -1;
	}

	r = bbus_hmap_setstr(conn->methods, method->name,
			(void*)method->func);
	if (r < 0)
		return -1;

	return 0;
}

int bbus_srvc_listencalls(bbus_service_connection* conn,
		struct bbus_timeval* tv)
{
	int r;
	struct bbus_msg_hdr hdr;
	char buf[BBUS_MAXMSGSIZE];
	const char* meta;
	bbus_object* objarg;
	bbus_object* objret;
	void* callback;
	uint32_t token;
	struct bbus_msg* msg;

	msg = (struct bbus_msg*)buf;
	r = __bbus_sock_rdready(conn->sock, tv);
	if (r < 0) {
		return -1;
	} else
	if (r == 0) {
		/* Timeout */
		return 0;
	} else {
		/* Message incoming */
		memset(buf, 0, BBUS_MAXMSGSIZE);
		r = __bbus_prot_recvmsg(conn->sock, msg, BBUS_MAXMSGSIZE);
		if (r < 0)
			return -1;
		if (msg->hdr.msgtype != BBUS_MSGTYPE_SRVCALL) {
			__bbus_seterr(BBUS_EMSGINVTYPRCVD);
			return -1;
		}

		token = bbus_hdr_gettoken(&msg->hdr);
		meta = bbus_prot_extractmeta(msg);
		if (meta == NULL) {
			__bbus_seterr(BBUS_EMSGINVFMT);
			return -1;
		}

		objarg = bbus_prot_extractobj(msg);
		if (objarg == NULL) {
			__bbus_seterr(BBUS_EMSGINVFMT);
			return -1;
		}

		memset(&hdr, 0, sizeof(struct bbus_msg_hdr));
		__bbus_prot_hdrsetmagic(&hdr);
		hdr.msgtype = BBUS_MSGTYPE_SRVREPLY;
		bbus_hdr_settoken(&hdr, token);
		objret = NULL;
		callback = bbus_hmap_findstr(conn->methods, meta);
		if (callback == NULL) {
			hdr.errcode = BBUS_PROT_ENOMETHOD;
			__bbus_seterr(BBUS_ENOMETHOD);
			goto send_reply;
		}

		objret = ((bbus_method_func)callback)(objarg);
		if (objret == NULL) {
			hdr.errcode = BBUS_PROT_EMETHODERR;
			__bbus_seterr(BBUS_EMETHODERR);
			goto send_reply;
		}

		bbus_hdr_setpsize(&hdr, bbus_obj_rawsize(objret));
		BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASOBJECT);

send_reply:
		r = __bbus_prot_sendvmsg(conn->sock, &hdr, NULL,
			objret == NULL ? NULL : bbus_obj_rawdata(objret),
			objret == NULL ? 0 : bbus_obj_rawsize(objret));
		if (r < 0)
			return -1;

		bbus_obj_free(objret);
		bbus_obj_free(objarg);
	}

	return hdr.errcode == 0 ? 0 : -1;
}

int bbus_srvc_closeconn(bbus_service_connection* conn)
{
	int r;

	r = send_session_close(conn->sock);
	if (r < 0)
		return -1;
	bbus_str_free(conn->srvname);
	bbus_hmap_free(conn->methods);
	bbus_free(conn);
	return 0;
}

