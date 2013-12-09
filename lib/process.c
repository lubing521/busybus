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
#include "error.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int bbus_proc_pidtoname(pid_t pid, char* buf, size_t buflen)
{
	FILE* fd;
	char* filepath;
	int ret;

	ret = access("/proc/", F_OK);
	if (ret < 0) {
		__bbus_seterr(errno);
		return -1;
	}

	filepath = bbus_str_build("/proc/%u/cmdline", pid);
	if (filepath == NULL)
		return -1;

	fd = fopen(filepath, "r");
	bbus_str_free(filepath);
	if (fd == NULL) {
		__bbus_seterr(errno);
		return -1;
	}

	clearerr(fd);
	(void)fgets(buf, buflen, fd);
	if (ferror(fd) != 0) {
		fclose(fd);
		__bbus_seterr(errno);
		return -1;
	}
	fclose(fd);

	if (strlen(buf) == 0)
		snprintf(buf, buflen, "<unknown>");

	return 0;
}

