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

#include "socket.h"
#include "error.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>

int __bbus_local_socket(void)
{
	int s;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return s;
}

int __bbus_bind_local_sock(int sock, const char* path)
{
	static const size_t spath_len = sizeof(
			((struct sockaddr_un*)0)->sun_path);
	static const size_t sfam_len = sizeof(
			((struct sockaddr_un*)0)->sun_family);

	int r;
	unsigned len;
	struct sockaddr_un addr;

	r = unlink(path);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, spath_len);
	len = sfam_len + strnlen(addr.sun_path, spath_len);
	r = bind(sock, (struct sockaddr*)&addr, len);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return 0;
}

int __bbus_sock_listen(int sock, int backlog)
{
	int r;

	r = listen(sock, backlog);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}
	return 0;
}

int __bbus_local_accept(int sock, char* pathbuf,
		size_t bufsize, size_t* pathsize)
{
	int s;
	struct sockaddr_un addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(struct sockaddr_un));
	memset(&addrlen, 0, sizeof(socklen_t));
	s = accept(sock, (struct sockaddr*)&addr, &addrlen);
	if (s < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	strncpy(pathbuf, addr.sun_path, bufsize); //FIXME
	*pathsize = (size_t)addrlen;

	return s;
}

int __bbus_local_connect(int sock, const char* path)
{

}

int __bbus_sock_close(int sock)
{
	int r;

	r = close(sock);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}
	return 0;
}

ssize_t __bbus_send_msg(int sock, const void* buf, size_t size)
{

}

ssize_t __bbus_recv_msg(int sock, void* buf, size_t size)
{

}


