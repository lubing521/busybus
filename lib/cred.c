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

#include "cred.h"
#include "error.h"
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>

int __bbus_getcred(int sock, struct bbus_client_cred* cred)
{
	struct ucred ucr;
	int ret;
	socklen_t optlen = sizeof(struct ucred);

	memset(&ucr, 0, optlen);
	ret = getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &ucr, &optlen);
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	cred->pid = ucr.pid;
	cred->uid = ucr.uid;
	cred->gid = ucr.gid;

	return 0;
}

int bbus_cred_uidtousername(uid_t uid, char* buf, size_t buflen)
{
	struct passwd pwd;
	struct passwd* pres;
	char* pbuf;
	int pbufsiz;
	int ret;

	pbufsiz = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (pbufsiz < 0)
		pbufsiz = 256;

	pbuf = bbus_malloc0(pbufsiz);
	if (pbuf == NULL)
		return -1;

	memset(&pwd, 0, sizeof(struct passwd));
	ret = getpwuid_r(uid, &pwd, pbuf, pbufsiz, &pres);
	if (ret < 0) {
		__bbus_seterr(errno);
		ret = -1;
		goto out;
	}

	(void)snprintf(buf, buflen, "%s", pres->pw_name);

out:
	bbus_free(pbuf);
	return ret;
}

void __bbus_cred_copy(struct bbus_client_cred* dst,
			const struct bbus_client_cred* src)
{
	dst->pid = src->pid;
	dst->uid = src->uid;
	dst->gid = src->gid;
}

