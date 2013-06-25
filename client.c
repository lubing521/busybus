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
#include "protocol.h"
#include <string.h>

struct __bbus_client_connection
{
	int sock;
};

bbus_client_connection* bbus_client_connect(void)
{
	return bbus_client_connect_wpath(BBUS_DEF_DIRPATH BBUS_DEF_SOCKNAME);
}

bbus_client_connection* bbus_client_connect_wpath(const char* path)
{
	struct __bbus_client_connection* conn;
	int r;
	struct bbus_msg_hdr hdr;

	conn = bbus_malloc(sizeof(struct __bbus_client_connection));
	if (conn == NULL)
		goto errout;

	conn->sock = __bbus_local_socket();
	if (conn->sock < 0)
		goto errout;

	r = __bbus_local_connect(conn->sock, path);
	if (r < 0)
		goto errout;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	__bbus_set_magic(&hdr);
	hdr->msgtype = BBUS_MSGTYPE_SOCLI;
	r = __bbus_sendv_msg(conn->sock, hdr, NULL, NULL, 0);
	if (r < 0)
		goto errout_close;

	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	r = __bbus_recvv_msg(conn->sock, &hdr, NULL, 0);
	if (r < 0)
		goto errout_close;

	if (hdr.msgtype == BBUS_MSGTYPE_SOOK) {
		return conn;
	} else
	if (hdr->msgtype == BBUS_MSGTYPE_SORJCT) {
		__bbus_set_err(BBUS_SORJCTD);
		goto errout_close;
	} else {
		__bbus_set_err(BBUS_MSGINVTYPERCVD);
		goto errout_close;
	}

errout_close:
	__bbus_sock_close(conn->sock);
errout:
	bbus_free(conn);
	return NULL;
}

bbus_object* bbus_call_method(bbus_client_connection* conn,
		const char* method, bbus_object* arg)
{
	int r;
	struct bbus_msg_hdr hdr;
	struct iovec data[3];
	size_t mlen;

	mlen = strlen(method) + 1;
	memset(&hdr, 0, BBUS_MSGHDR_SIZE);
	__bbus_set_magic(&hdr);
	hdr->msgtype = BBUS_MSGTYPE_CLICALL;
	hdr->psize = mlen + bbus_obj_rawdata_size(obj);
	hdr->flags |= BBUS_PROT_HASMETA;
	hdr->flags |= BBUS_PROT_HASOBJECT;
	data[0].iov_base = &hdr;
	data[0].iov_len = BBUS_MSGHDR_SIZE;
	data[1].iov_base = method;
	data[1].iov_len = mlen;
	data[2].iov_base = bbus_obj_rawdata(obj);
	data[2].iov_len = bbus_obj_rawdata_size(obj);
}

int bbus_close_client_conn(bbus_client_connection* conn)
{
	int r;

	r = __bbus_sock_close(conn->sock);
	if (r < 0)
		return -1;
	bbus_free(conn);

	return 0;
}


