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
#include "protocol.h"
#include <string.h>

#define MAX_NUMIOV 3

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
	if (r < (ssize_t)BBUS_MSGHDR_SIZE) {
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

	if (!__bbus_hdr_checkmagic((struct bbus_msg_hdr*)buf)) {
		__bbus_set_err(BBUS_MSGMAGIC);
		return -1;
	}

	return 0;
}

int __bbus_send_msg(int sock, const void* buf, size_t bufsize)
{
	ssize_t r;
	size_t msgsize;
	size_t sent;

	msgsize = BBUS_MSGHDR_SIZE + ((struct bbus_msg_hdr*)buf)->psize;
	if (msgsize > bufsize) {
		__bbus_set_err(BBUS_INVALARG);
		return -1;
	}
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

int __bbus_recvv_msg(int sock, struct bbus_msg_hdr* hdr,
		void* payload, size_t psize)
{
	ssize_t r;
	struct iovec iov[MAX_NUMIOV];
	int numiov;

	numiov = 0;
	iov[numiov].iov_base = hdr;
	iov[numiov].iov_len = BBUS_MSGHDR_SIZE;
	++numiov;
	if (payload == NULL) {
		iov[numiov].iov_base = payload;
		iov[numiov].iov_len = psize;
		++numiov;
	}

	r = __bbus_recvv(sock, iov, 2);
	if (r < 0) {
		return -1;
	} else
	if (r < (ssize_t)BBUS_MSGHDR_SIZE) {
		__bbus_set_err(BBUS_MSGINVFMT);
		return -1;
	} else
	if (r < (ssize_t)(BBUS_MSGHDR_SIZE + hdr->psize)) {
		__bbus_set_err(BBUS_RCVDLESS);
		return -1;
	}

	if (!__bbus_hdr_checkmagic(hdr)) {
		__bbus_set_err(BBUS_MSGMAGIC);
		return -1;
	}

	return 0;
}

int __bbus_sendv_msg(int sock, struct bbus_msg_hdr* hdr,
		char* meta, char* obj, size_t objsize)
{
	ssize_t r;
	size_t msgsize;
	struct iovec iov[MAX_NUMIOV];
	int numiov;
	size_t metasize;

	metasize = meta == NULL ? 0 : strlen(meta)+1;
	msgsize = BBUS_MSGHDR_SIZE + metasize + objsize;
	if (msgsize != (BBUS_MSGHDR_SIZE + hdr->psize)) {
		__bbus_set_err(BBUS_INVALARG);
		return -1;
	}

	numiov = 0;
	iov[numiov].iov_base = hdr;
	iov[numiov].iov_len = BBUS_MSGHDR_SIZE;
	++numiov;
	if (meta != NULL) {
		iov[numiov].iov_base = meta;
		iov[numiov].iov_len = metasize;
		++numiov;
	}
	if (obj != NULL) {
		iov[numiov].iov_base = obj;
		iov[numiov].iov_len = objsize;
		++numiov;
	}

	r = __bbus_sendv(sock, iov, numiov);
	if (r < 0) {
		return -1;
	} else
	if (r != (ssize_t)msgsize) {
		/* TODO Retry? */
		__bbus_set_err(BBUS_SENTLESS);
		return -1;
	}

	return 0;
}

void __bbus_hdr_setmagic(struct bbus_msg_hdr* hdr)
{
	memcpy(&hdr->magic, BBUS_MAGIC, BBUS_MAGIC_SIZE);
}

int __bbus_hdr_checkmagic(struct bbus_msg_hdr* hdr)
{
	return memcmp(&hdr->magic, BBUS_MAGIC, BBUS_MAGIC_SIZE) == 0 ? 1 : 0;
}

int __bbus_proterr_to_errnum(uint8_t errcode)
{
	int errnum;

	switch (errcode)
	{
	case BBUS_PROT_GOOD:
		errnum = BBUS_SUCCESS;
		break;
	case BBUS_PROT_NOMETHOD:
		errnum = BBUS_NOMETHOD;
		break;
	default:
		errnum = BBUS_INVALARG;
		break;
	}

	return errnum;
}



