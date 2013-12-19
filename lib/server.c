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
#include "error.h"
#include "socket.h"
#include "protocol.h"
#include "cred.h"
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define DEF_LISTEN_QUEUE 5

struct __bbus_client
{
	int sock;
	int type;
	uint32_t token;
	struct bbus_client_cred cred;
	char* name;
};

struct __bbus_server
{
	int sock;
};

struct __bbus_pollset
{
	fd_set fdset;
	int highsock;
};

uint32_t bbus_client_gettoken(bbus_client* cli)
{
	return cli->token;
}

void bbus_client_settoken(bbus_client* cli, uint32_t tok)
{
	cli->token = tok;
}

int bbus_client_gettype(bbus_client* cli)
{
	return cli->type;
}

const char* bbus_client_getname(bbus_client* cli)
{
	return cli->name;
}

int bbus_client_rcvmsg(bbus_client* cli,
				struct bbus_msg* buf, size_t bufsize)
{
	return __bbus_prot_recvmsg(cli->sock, buf, bufsize);
}

int bbus_client_sendmsg(bbus_client* cli, struct bbus_msg_hdr* hdr,
		const char* meta, bbus_object* obj)
{
	return __bbus_prot_sendvmsg(cli->sock, hdr, meta,
				obj == NULL ? NULL : bbus_obj_rawdata(obj),
				obj == NULL ? 0 : bbus_obj_rawsize(obj));
}

int bbus_client_close(bbus_client* cli)
{
	return __bbus_sock_close(cli->sock);
}

void bbus_client_free(bbus_client* cli)
{
	bbus_str_free(cli->name);
	bbus_free(cli);
}

bbus_server* bbus_srv_create(void)
{
	bbus_server* srv;
	int sock;
	int ret;

	sock = __bbus_sock_un_mksocket();
	if (sock < 0)
		goto err;

	ret = __bbus_sock_un_bind(sock, bbus_prot_getsockpath());
	if (ret < 0)
		goto err;

	srv = bbus_malloc(sizeof(struct __bbus_server));
	if (srv == NULL)
		goto err;

	srv->sock = sock;
	return srv;

err:
	__bbus_sock_close(sock);
	return NULL;
}

int bbus_srv_listen(bbus_server* srv)
{
	return __bbus_sock_listen(srv->sock, DEF_LISTEN_QUEUE);
}

int bbus_srv_clientpending(bbus_server* srv)
{
	struct bbus_timeval tv;

	memset(&tv, 0, sizeof(struct bbus_timeval));
	tv.sec = 0;
	tv.usec = 0;

	return __bbus_sock_rdready(srv->sock, &tv);
}

bbus_client* bbus_srv_accept(bbus_server* srv,
				const struct bbus_accept_callbacks* funcs)
{
	char addrbuf[128]; /* FIXME Should be a constant, not a magic value. */
	char clinamebuf[BBUS_CLIENT_MAXNAMESIZE];
	size_t addrsize;
	int sock;
	int ret;
	bbus_client* cli;
	unsigned char buf[sizeof(struct bbus_msg)];
	struct bbus_msg* msg = (struct bbus_msg*)buf;
	struct bbus_msg_hdr* hdr = &msg->hdr;
	int clitype;
	struct bbus_client_cred cred;

	sock = __bbus_sock_un_accept(srv->sock, addrbuf,
					sizeof(addrbuf), &addrsize);
	if (sock < 0)
		return NULL;

	ret = __bbus_cred_get(sock, &cred);
	if (ret < 0)
		goto errout;

	if (funcs && funcs->auth) {
		ret = funcs->auth(&cred);
		if (ret == BBUS_SRV_AUTHERR) {
			__bbus_seterr(BBUS_ECLIUNAUTH);
			goto errout;
		}
	}

	memset(hdr, 0, sizeof(struct bbus_msg_hdr));
	memset(clinamebuf, 0, BBUS_CLIENT_MAXNAMESIZE);
	ret = __bbus_prot_recvvmsg(sock, hdr, clinamebuf,
					BBUS_CLIENT_MAXNAMESIZE);
	if (ret < 0)
		goto errout;
	if (hdr->msgtype != BBUS_MSGTYPE_SO)
		goto errout;

	if (funcs && funcs->rcvd)
		funcs->rcvd(msg);

	/* TODO Small function to do the convertion. */
	switch (hdr->sotype) {
	case BBUS_SOTYPE_MTHCL: clitype = BBUS_CLIENT_CALLER; break;
	case BBUS_SOTYPE_SRVPRV: clitype = BBUS_CLIENT_SERVICE; break;
	case BBUS_SOTYPE_MON: clitype = BBUS_CLIENT_MON; break;
	case BBUS_SOTYPE_CTL: clitype = BBUS_CLIENT_CTL; break;
	default: goto errout; break;
	}

	hdr->msgtype = BBUS_MSGTYPE_SOOK;
	hdr->psize = 0;
	hdr->flags = 0;
	ret = __bbus_prot_sendvmsg(sock, hdr, NULL, NULL, 0);
	if (ret < 0)
		goto errout;

	if (funcs && funcs->sent)
		funcs->sent(hdr, NULL, NULL);

	cli = bbus_malloc(sizeof(struct __bbus_client));
	if (cli == NULL)
		goto errout;
	cli->sock = sock;
	cli->token = 0;
	cli->type = clitype;
	__bbus_cred_copy(&cli->cred, &cred);
	cli->name = bbus_str_build("%s", strlen(clinamebuf) == 0
					? "<unknown>" : clinamebuf);
	if (cli->name == NULL)
		goto errout;

	return cli;

errout:
	__bbus_sock_close(sock);
	return NULL;
}

int bbus_srv_close(bbus_server* srv)
{
	return __bbus_sock_close(srv->sock);
}

void bbus_srv_free(bbus_server* srv)
{
	bbus_free(srv);
}

bbus_pollset* bbus_pollset_make(void)
{
	bbus_pollset* pset;

	pset = bbus_malloc0(sizeof(struct __bbus_pollset));
	if (pset == NULL)
		return NULL;
	bbus_pollset_clear(pset);

	return pset;
}

void bbus_pollset_clear(bbus_pollset* pset)
{
	FD_ZERO(&pset->fdset);
	pset->highsock = 0;
}

static inline void update_highsock(bbus_pollset* pset, int sock)
{
	if (pset->highsock < (sock+1))
		pset->highsock = sock+1;
}

void bbus_pollset_addsrv(bbus_pollset* pset, bbus_server* src)
{
	FD_SET(src->sock, &pset->fdset);
	update_highsock(pset, src->sock);
}

void bbus_pollset_addcli(bbus_pollset* pset, bbus_client* cli)
{
	FD_SET(cli->sock, &pset->fdset);
	update_highsock(pset, cli->sock);
}

int bbus_poll(bbus_pollset* pset, struct bbus_timeval* tv)
{
	struct timeval stv;
	int ret;

	memset(&stv, 0, sizeof(struct timeval));
	stv.tv_sec = tv->sec;
	stv.tv_usec = tv->usec;

	ret = select(pset->highsock, &pset->fdset, NULL, NULL, &stv);
	tv->sec = stv.tv_sec;
	tv->usec = stv.tv_usec;

	if (ret < 0) {
		__bbus_seterr(errno == EINTR ? BBUS_EPOLLINTR : errno);
		return -1;
	}

	return ret;
}

int bbus_pollset_srvisset(bbus_pollset* pset, bbus_server* srv)
{
	return FD_ISSET(srv->sock, &pset->fdset);
}

int bbus_pollset_cliisset(bbus_pollset* pset, bbus_client* cli)
{
	return FD_ISSET(cli->sock, &pset->fdset);
}

void bbus_pollset_free(bbus_pollset* pset)
{
	bbus_free(pset);
}

