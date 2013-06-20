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

	conn = bbus_malloc(sizeof(struct __bbus_client_connection));
	if (conn == NULL)
		goto errout;

	conn->sock = __bbus_local_socket();
	if (conn->sock < 0)
		goto errout_free;

	r = __bbus_local_connect(conn->sock, path);
	if (r < 0)
		goto errout_free;

	return conn;

errout_free:
	bbus_free(conn);
errout:
	return NULL;
}

bbus_object* bbus_call_method(bbus_client_connection* conn,
		const char* method, bbus_object* arg)
{
}

int bbus_close_client_conn(bbus_client_connection* conn)
{
	int r;

	r = __bbus_sock_close(conn->sock);
	if (r < 0)
		return -1;
	bbus_free(conn);
}


