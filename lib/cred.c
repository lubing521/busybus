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

