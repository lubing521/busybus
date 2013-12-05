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
#include "socket.h"
#include "error.h"
#include "protocol.h"
#include "spinlock.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_NUMIOV 3

static struct __bbus_spinlock sockpath_lock = __BBUS_SPINLOCK_INITIALIZER;
static char sockpath[BBUS_PROT_SOCKPATHMAX] = BBUS_PROT_DEFSOCKPATH;
static BBUS_THREAD_LOCAL char localpath[BBUS_PROT_SOCKPATHMAX];

void bbus_prot_setsockpath(const char* path)
{
	__bbus_spinlock_lock(&sockpath_lock);
	(void)snprintf(sockpath, BBUS_PROT_SOCKPATHMAX, "%s", path);
	__bbus_spinlock_unlock(&sockpath_lock);
}

const char* bbus_prot_getsockpath(void)
{
	__bbus_spinlock_lock(&sockpath_lock);
	(void)snprintf(localpath, BBUS_PROT_SOCKPATHMAX, "%s", sockpath);
	__bbus_spinlock_unlock(&sockpath_lock);
	return localpath;
}

static int hdr_check_magic(const struct bbus_msg_hdr* hdr)
{
	return memcmp(&hdr->magic, BBUS_MAGIC, BBUS_MAGIC_SIZE) == 0
						? BBUS_TRUE : BBUS_FALSE;
}

int __bbus_prot_recvmsg(int sock, struct bbus_msg* buf, size_t bufsize)
{
	ssize_t r;
	ssize_t rcvd;
	ssize_t msgsize;
	struct bbus_msg_hdr* hdr;

	if (bufsize > BBUS_MAXMSGSIZE) {
		__bbus_seterr(BBUS_EINVALARG);
		return -1;
	}

	r = __bbus_sock_recv(sock, buf, bufsize);
	if (r < 0) {
		return -1;
	} else
	if (r == 0) {
		__bbus_seterr(BBUS_ECONNCLOSED);
		return -1;
	} else
	if (r < (ssize_t)BBUS_MSGHDR_SIZE) {
		__bbus_seterr(BBUS_EMSGINVFMT);
		return -1;
	}

	hdr = (struct bbus_msg_hdr*)buf;
	msgsize = bbus_hdr_getpsize(hdr) + BBUS_MSGHDR_SIZE;
	if (msgsize > (ssize_t)bufsize) {
		__bbus_seterr(BBUS_ENOSPACE);
		return -1;
	}

	rcvd = r;
	buf += r;
	while (rcvd != msgsize) {
		r = __bbus_sock_recv(sock, buf, bufsize-rcvd);
		if (r < 0) {
			return -1;
		} else
		if (r == 0) {
			__bbus_seterr(BBUS_ECONNCLOSED);
			return -1;
		}
		rcvd += r;
		buf += r;
	}

	if (!hdr_check_magic(hdr)) {
		__bbus_seterr(BBUS_EMSGMAGIC);
		return -1;
	}

	return 0;
}

int __bbus_prot_sendmsg(int sock, const struct bbus_msg* buf)
{
	ssize_t r;
	size_t msgsize;
	size_t sent;

	msgsize = BBUS_MSGHDR_SIZE + bbus_hdr_getpsize(
					(struct bbus_msg_hdr*)buf);
	if (msgsize > BBUS_MAXMSGSIZE) {
		/* FIXME Is this check needed? */
		__bbus_seterr(BBUS_EINVALARG);
		return -1;
	}
	sent = 0;

	do {
		r = __bbus_sock_send(sock, buf, msgsize-sent);
		if (r < 0)
			return -1;
		sent += r;
		buf += r;
	} while (msgsize != sent);

	return 0;
}

int __bbus_prot_recvvmsg(int sock, struct bbus_msg_hdr* hdr,
					void* payload, size_t psize)
{
	ssize_t r;
	struct iovec iov[MAX_NUMIOV];
	int numiov;

	if ((BBUS_MSGHDR_SIZE + psize) > BBUS_MAXMSGSIZE) {
		__bbus_seterr(BBUS_EINVALARG);
		return -1;
	}

	numiov = 0;
	iov[numiov].iov_base = hdr;
	iov[numiov].iov_len = BBUS_MSGHDR_SIZE;
	++numiov;
	if (payload != NULL) {
		iov[numiov].iov_base = payload;
		iov[numiov].iov_len = psize;
		++numiov;
	}

	r = __bbus_sock_recvv(sock, iov, numiov);
	if (r < 0) {
		return -1;
	} else
	if (r < (ssize_t)BBUS_MSGHDR_SIZE) {
		__bbus_seterr(BBUS_EMSGINVFMT);
		return -1;
	} else
	if (r < (ssize_t)(BBUS_MSGHDR_SIZE + bbus_hdr_getpsize(hdr))) {
		__bbus_seterr(BBUS_ERCVDLESS);
		return -1;
	}

	if (!hdr_check_magic(hdr)) {
		__bbus_seterr(BBUS_EMSGMAGIC);
		return -1;
	}

	return 0;
}

int __bbus_prot_sendvmsg(int sock, const struct bbus_msg_hdr* hdr,
			const char* meta, const char* obj, size_t objsize)
{
	ssize_t r;
	size_t msgsize;
	struct iovec iov[MAX_NUMIOV];
	int numiov;
	size_t metasize;

	metasize = meta == NULL ? 0 : strlen(meta)+1;
	msgsize = BBUS_MSGHDR_SIZE + metasize + objsize;
	if ((msgsize != (BBUS_MSGHDR_SIZE + bbus_hdr_getpsize(hdr)))
				|| (msgsize > BBUS_MAXMSGSIZE)) {
		__bbus_seterr(BBUS_EINVALARG);
		return -1;
	}

	numiov = 0;
	iov[numiov].iov_base = (void*)hdr;
	iov[numiov].iov_len = BBUS_MSGHDR_SIZE;
	++numiov;
	if (meta != NULL) {
		iov[numiov].iov_base = (void*)meta;
		iov[numiov].iov_len = metasize;
		++numiov;
	}
	if (obj != NULL) {
		iov[numiov].iov_base = (void*)obj;
		iov[numiov].iov_len = objsize;
		++numiov;
	}

	r = __bbus_sock_sendv(sock, iov, numiov);
	if (r < 0) {
		return -1;
	} else
	if (r != (ssize_t)msgsize) {
		/* TODO Retry? */
		__bbus_seterr(BBUS_ESENTLESS);
		return -1;
	}

	return 0;
}

void __bbus_prot_hdrsetmagic(struct bbus_msg_hdr* hdr)
{
	memcpy(&hdr->magic, BBUS_MAGIC, BBUS_MAGIC_SIZE);
}

int __bbus_prot_errtoerrnum(uint8_t errcode)
{
	int errnum;

	switch (errcode)
	{
	case BBUS_PROT_EGOOD:
		errnum = BBUS_ESUCCESS;
		break;
	case BBUS_PROT_ENOMETHOD:
		errnum = BBUS_ENOMETHOD;
		break;
	case BBUS_PROT_EMETHODERR:
		errnum = BBUS_EMETHODERR;
		break;
	case BBUS_PROT_EMREGERR:
		errnum = BBUS_EMREGERR;
		break;
	default:
		errnum = BBUS_EINVALARG;
		break;
	}

	return errnum;
}

const char* bbus_prot_extractmeta(const struct bbus_msg* msg)
{
	const void* payload;
	size_t msgsize;

	msgsize = bbus_hdr_getpsize(&msg->hdr);
	if (msg->hdr.flags & BBUS_PROT_HASMETA) {
		payload = msg->payload;
		if (memmem(payload, msgsize, "\0", 1) == NULL) {
			goto err;
		} else {
			return payload;
		}
	}

err:
	__bbus_seterr(BBUS_EOBJINVFMT);
	return NULL;
}

bbus_object* bbus_prot_extractobj(const struct bbus_msg* msg)
{
	const char* meta;
	const void* payload;
	size_t psize;
	size_t offset;

	if (msg->hdr.flags & BBUS_PROT_HASOBJECT) {
		psize = bbus_hdr_getpsize(&msg->hdr);
		payload = msg->payload;
		if (msg->hdr.flags & BBUS_PROT_HASMETA) {
			meta = bbus_prot_extractmeta(msg);
			if (meta != NULL) {
				offset = strlen(meta)+1;
				payload += offset;
				psize -= offset;
			}
		}
	} else {
		__bbus_seterr(BBUS_EOBJINVFMT);
		return NULL;
	}

	return bbus_obj_frombuf(payload, psize);
}

void bbus_hdr_build(struct bbus_msg_hdr* hdr, int typ, int err)
{
	memset(hdr, 0, sizeof(struct bbus_msg_hdr));
	__bbus_prot_hdrsetmagic(hdr);
	hdr->msgtype = (uint8_t)typ;
	hdr->errcode = (uint8_t)err;
}

unsigned bbus_hdr_gettoken(const struct bbus_msg_hdr* hdr)
{
	return (unsigned)ntohl(hdr->token);
}

void bbus_hdr_settoken(struct bbus_msg_hdr* hdr, unsigned tok)
{
	hdr->token = (uint32_t)htonl(tok);
}

size_t bbus_hdr_getpsize(const struct bbus_msg_hdr* hdr)
{
	return (size_t)ntohs(hdr->psize);
}

void bbus_hdr_setpsize(struct bbus_msg_hdr* hdr, size_t size)
{
	hdr->psize = size > UINT16_MAX ? UINT16_MAX : htons((uint16_t)size);
}

