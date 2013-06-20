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
#include "socket.h"
#include "error.h"

int __bbus_recv_msg(int sock, void* buf, size_t bufsize)
{
	ssize_t r;
	size_t rcvd;
	size_t msgsize;

	r = __bbus_recv(sock, buf, bufsize);
	if (r < 0) {
		return -1;
	} else
	if (r == 0) {
		__bbus_set_err(BBUS_CONNCLOSED);
		return -1;
	} else
	if (r < BBUS_MSGHDR_SIZE) {
		__bbus_set_err(BBUS_MSGINVFMT);
		return -1;
	}

	msgsize = ((struct bbus_msg_hdr*)buf)->psize + BBUS_MSGHDR_SIZE;
	if (msgsize > bufsize) {
		__bbus_set_err(BBUS_NOSPACE);
		return -1;
	}

	rcvd = r;
	buf += r;
	while (rcvd != msgsize) {
		r = __bbus_recv(sock, buf, bufsize-rcvd);
		if (r < 0) {
			return -1;
		} else
		if (r == 0) {
			__bbus_set_err(BBUS_CONNCLOSED);
			return -1;
		}
		rcvd += r;
		buf += r;
	}

	return 0;
}

int __bbus_send_msg(int sock, const void* buf, size_t bufsize)
{
	ssize_t r;
	size_t msgsize;
	size_t sent;

	msgsize = BBUS_MSGHDR_SIZE + ((struct bbus_msg_hdr*)buf)->psize;
	sent = 0;

	do {
		r = __bbus_send(sock, buf, msgsize-sent);
		if (r < 0)
			return -1;
		sent += r;
		buf += r;
	} while (msgsize != sent);

	return 0;
}




