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
#include <sys/select.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>

static const size_t saun_pathlen = sizeof(((struct sockaddr_un*)0)->sun_path);
static const size_t saun_famlen = sizeof(((struct sockaddr_un*)0)->sun_family);

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
	int r;
	socklen_t addrlen;
	struct sockaddr_un addr;

	r = unlink(path);
	if ((r < 0) && (errno != ENOENT)) {
		__bbus_set_err(errno);
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, saun_pathlen);
	addrlen = saun_famlen + strnlen(addr.sun_path, saun_pathlen);
	r = bind(sock, (struct sockaddr*)&addr, addrlen);
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

	strncpy(pathbuf, addr.sun_path, bufsize);
	*pathsize = (size_t)addrlen;

	return s;
}

int __bbus_local_connect(int sock, const char* path)
{
	int r;
	struct sockaddr_un addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, saun_pathlen);
	addrlen = saun_famlen + strnlen(addr.sun_path, saun_pathlen);
	r = connect(sock, (struct sockaddr*)&addr, addrlen);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return 0;
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

ssize_t __bbus_send(int sock, const void* buf, size_t size)
{
	ssize_t b;

	b = send(sock, buf, size, MSG_DONTWAIT);
	if (b < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return b;
}

ssize_t __bbus_recv(int sock, void* buf, size_t size)
{
	ssize_t b;

	b = recv(sock, buf, size, MSG_DONTWAIT);
	if (b < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return b;
}

ssize_t __bbus_sendv(int sock, const struct iovec* iov, int numiov)
{
	ssize_t b;

	b = writev(sock, iov, numiov);
	if (b < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return b;
}

ssize_t __bbus_recvv(int sock, struct iovec* iov, int numiov)
{
	ssize_t b;

	b = readv(sock, iov, numiov);
	if (b < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	return b;
}

int __bbus_sock_wr_ready(int sock, struct bbus_timeval* tv)
{
	fd_set wr_set;
	struct timeval timeout;
	int r;

	FD_ZERO(&wr_set);
	FD_SET(sock, &wr_set);
	timeout.tv_sec = tv->sec;
	timeout.tv_usec = tv->usec;
	r = select(sock+1, NULL, &wr_set, NULL, &timeout);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	tv->sec = timeout.tv_sec;
	tv->usec = timeout.tv_usec;
	return r;
}

int __bbus_sock_rd_ready(int sock, struct bbus_timeval* tv)
{
	fd_set rd_set;
	struct timeval timeout;
	int r;

	FD_ZERO(&rd_set);
	FD_SET(sock, &rd_set);
	timeout.tv_sec = tv->sec;
	timeout.tv_usec = tv->usec;
	r = select(sock+1, &rd_set, NULL, NULL, &timeout);
	if (r < 0) {
		__bbus_set_err(errno);
		return -1;
	}

	tv->sec = timeout.tv_sec;
	tv->usec = timeout.tv_usec;
	return r;
}

