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

#include <stdio.h>
#include "log.h"
#include "common.h"

/* TODO Setters for logmask and maybe log levels. */
#define LOG_CONSOLE	(1 << 0)
#define LOG_SYSL	(1 << 1)
static int logmask = LOG_CONSOLE;

static inline int loglvl_to_sysloglvl(enum bbusd_loglevel lvl)
{
	return (int)lvl;
}

#define SYSLOG_IDENT "bbusd"

void bbusd_logmsg(enum bbusd_loglevel lvl, const char* fmt, ...)
{
	va_list va;

	if (logmask & LOG_CONSOLE) {
		va_start(va, fmt);
		switch (lvl) {
		case BBUSD_LOG_EMERG:
		case BBUSD_LOG_ALERT:
		case BBUSD_LOG_CRIT:
		case BBUSD_LOG_ERR:
		case BBUSD_LOG_WARN:
			vfprintf(stderr, fmt, va);
			break;
		case BBUSD_LOG_NOTICE:
		case BBUSD_LOG_INFO:
		case BBUSD_LOG_DEBUG:
			vfprintf(stdout, fmt, va);
			break;
		default:
			bbusd_die("Invalid log level\n");
			break;
		}
		va_end(va);
	}

	if (logmask & LOG_SYSL) {
		va_start(va, fmt);
		openlog(SYSLOG_IDENT, LOG_PID, LOG_DAEMON);
		vsyslog(loglvl_to_sysloglvl(lvl), fmt, va);
		closelog();
		va_end(va);
	}
}

