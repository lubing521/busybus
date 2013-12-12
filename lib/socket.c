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

#define SAUN_PATHLEN (sizeof(((struct sockaddr_un*)0)->sun_path))
#define SAUN_FAMLEN (sizeof(((struct sockaddr_un*)0)->sun_family))
#define IO_FLAGS (MSG_DONTWAIT | MSG_NOSIGNAL)

int __bbus_sock_un_mksocket(void)
{
	int s;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	return s;
}

int __bbus_sock_un_bind(int sock, const char* path)
{
	int r;
	socklen_t addrlen;
	struct sockaddr_un addr;

	r = unlink(path);
	if ((r < 0) && (errno != ENOENT)) {
		__bbus_seterr(errno);
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, SAUN_PATHLEN);
	addrlen = SAUN_FAMLEN + strnlen(addr.sun_path, SAUN_PATHLEN);
	r = bind(sock, (struct sockaddr*)&addr, addrlen);
	if (r < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	return 0;
}

int __bbus_sock_un_accept(int sock, char* pathbuf,
		size_t bufsize, size_t* pathsize)
{
	int s;
	struct sockaddr_un addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(struct sockaddr_un));
	memset(&addrlen, 0, sizeof(socklen_t));
	s = accept(sock, (struct sockaddr*)&addr, &addrlen);
	if (s < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	strncpy(pathbuf, addr.sun_path, bufsize);
	*pathsize = (size_t)addrlen;

	return s;
}

int __bbus_sock_un_connect(int sock, const char* path)
{
	int r;
	struct sockaddr_un addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, SAUN_PATHLEN);
	addrlen = SAUN_FAMLEN + strnlen(addr.sun_path, SAUN_PATHLEN);
	r = connect(sock, (struct sockaddr*)&addr, addrlen);
	if (r < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	return 0;
}

int __bbus_sock_un_rm(const char* path)
{
	if (unlink(path) < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	return 0;
}

int __bbus_sock_listen(int sock, int backlog)
{
	int r;

	r = listen(sock, backlog);
	if (r < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	return 0;
}

int __bbus_sock_close(int sock)
{
	int r;

	r = close(sock);
	if (r < 0) {
		__bbus_seterr(errno);
		return -1;
	}
	return 0;
}

static void prepare_msghdr(struct msghdr* hdr,
				const struct iovec* iov, int numiov)
{
	hdr->msg_name = NULL;
	hdr->msg_namelen = 0;
	hdr->msg_iov = (void*)iov;
	hdr->msg_iovlen = numiov;
	hdr->msg_control = NULL;
	hdr->msg_controllen = 0;
	hdr->msg_flags = 0;
}

ssize_t __bbus_sock_send(int sock, const struct iovec* iov, int numiov)
{
	ssize_t b;
	struct msghdr hdr;

	prepare_msghdr(&hdr, iov, numiov);
	b = sendmsg(sock, &hdr, IO_FLAGS);
	if (b < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	return b;
}

ssize_t __bbus_sock_recv(int sock, struct iovec* iov, int numiov)
{
	ssize_t b;
	struct msghdr hdr;

	prepare_msghdr(&hdr, iov, numiov);
	b = recvmsg(sock, &hdr, IO_FLAGS);
	if (b < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	return b;
}

#define SELECT_INIT(FDSET, SOCK, TV, BBTV)				\
	do {								\
		FD_ZERO(&(FDSET));					\
		FD_SET((SOCK), &(FDSET));				\
		(TV).tv_sec = (BBTV)->sec;				\
		(TV).tv_usec = (BBTV)->usec;				\
	} while (0)

#define SELECT_CHECKERR(RETVAL)						\
	do {								\
		if ((RETVAL) < 0) {					\
			__bbus_seterr(errno == EINTR			\
					? BBUS_EPOLLINTR : errno);	\
			return -1;					\
		}							\
	} while (0)

#define SELECT_COPYTV(TV, BBTV)						\
	do {								\
		(BBTV)->sec = (TV).tv_sec;				\
		(BBTV)->usec = (TV).tv_usec;				\
	} while (0)

int __bbus_sock_wrready(int sock, struct bbus_timeval* tv)
{
	fd_set wr_set;
	struct timeval timeout;
	int r;

	SELECT_INIT(wr_set, sock, timeout, tv);
	r = select(sock+1, NULL, &wr_set, NULL, &timeout);
	SELECT_CHECKERR(r);
	SELECT_COPYTV(timeout, tv);
	return r;
}

int __bbus_sock_rdready(int sock, struct bbus_timeval* tv)
{
	fd_set rd_set;
	struct timeval timeout;
	int r;

	SELECT_INIT(rd_set, sock, timeout, tv);
	r = select(sock+1, &rd_set, NULL, NULL, &timeout);
	SELECT_CHECKERR(r);
	SELECT_COPYTV(timeout, tv);
	return r;
}

