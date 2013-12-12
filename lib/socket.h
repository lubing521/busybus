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

#ifndef __BBUS_SOCKET__
#define __BBUS_SOCKET__

#include <busybus.h>
#include <stdlib.h>
#include <sys/uio.h>

/* Unix domain specific functions. */
int __bbus_sock_un_mksocket(void);
int __bbus_sock_un_bind(int sock, const char* path);
int __bbus_sock_un_accept(int sock, char* pathbuf,
		size_t bufsize, size_t* pathsize);
int __bbus_sock_un_connect(int sock, const char* path);
int __bbus_sock_un_rm(const char* path);

/* Common socket functions. */
int __bbus_sock_listen(int sock, int backlog);
int __bbus_sock_close(int sock);
ssize_t __bbus_sock_send(int sock, const struct iovec* iov, int numiov);
ssize_t __bbus_sock_recv(int sock, struct iovec* iov, int numiov);
int __bbus_sock_wrready(int sock, struct bbus_timeval* tv);
int __bbus_sock_rdready(int sock, struct bbus_timeval* tv);

#endif /* __BBUS_SOCKET__ */
