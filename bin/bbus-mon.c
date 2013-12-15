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
#include <stdio.h>
#include <signal.h>

static unsigned char __msgbuf[BBUS_MAXMSGSIZE];
static struct bbus_msg* msgbuf = (struct bbus_msg*)__msgbuf;
static volatile int run;

static void BBUS_PRINTF_FUNC(1, 2) BBUS_NORETURN die(const char* format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	exit(EXIT_FAILURE);
}

static int do_run(void)
{
	return BBUS_ATOMIC_GET(run);
}

static void do_stop(void)
{
	BBUS_ATOMIC_SET(run, 0);
}

static void sighandler(int signum)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		do_stop();
		break;
	}
}

#define PRES_CASE_PROPVAL(VAL)	case (VAL): return #VAL
#define PRES_DEF_WRONGVAL	default: return "<unknown value>"

static const char* str_msgtype(unsigned char msgtype)
{
	switch (msgtype) {
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SO);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SOOK);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SORJCT);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SRVREG);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SRVUNREG);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SRVACK);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_CLICALL);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_CLIREPLY);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SRVCALL);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_SRVREPLY);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_CLOSE);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_CTRL);
	PRES_CASE_PROPVAL(BBUS_MSGTYPE_MON);
	PRES_DEF_WRONGVAL;
	}
}

static const char* str_sotype(unsigned char sotype)
{
	switch (sotype) {
	PRES_CASE_PROPVAL(BBUS_SOTYPE_NONE);
	PRES_CASE_PROPVAL(BBUS_SOTYPE_MTHCL);
	PRES_CASE_PROPVAL(BBUS_SOTYPE_SRVPRV);
	PRES_CASE_PROPVAL(BBUS_SOTYPE_MON);
	PRES_CASE_PROPVAL(BBUS_SOTYPE_CTL);
	PRES_DEF_WRONGVAL;
	}
}

static const char* str_errcode(unsigned char errcode)
{
	switch (errcode) {
	PRES_CASE_PROPVAL(BBUS_PROT_EGOOD);
	PRES_CASE_PROPVAL(BBUS_PROT_ENOMETHOD);
	PRES_CASE_PROPVAL(BBUS_PROT_EMETHODERR);
	PRES_CASE_PROPVAL(BBUS_PROT_EMREGERR);
	PRES_DEF_WRONGVAL;
	}
}

static const char* str_flags(unsigned char flags)
{
	if (flags & (BBUS_PROT_HASMETA | BBUS_PROT_HASOBJECT)) {
		return "BBUS_PROT_HASMETA | BBUS_PROT_HASOBJECT";
	} else
	if (flags & BBUS_PROT_HASMETA) {
		return "BBUS_PROT_HASMETA";
	} else
	if (flags & BBUS_PROT_HASOBJECT) {
		return "BBUS_PROT_HASOBJECT";
	}

	return "<no flags set>";
}

static void print_msg_info(const char* meta, bbus_object* obj)
{
	unsigned char msgtype;
	unsigned char sotype;
	unsigned char errcode;
	unsigned token;
	size_t psize;
	unsigned char flags;
	char* msgmeta;
	int ret;

	ret = bbus_obj_parse(obj, "bbbuubs", &msgtype, &sotype, &errcode,
					&token, &psize, &flags, &msgmeta);
	if (ret < 0) {
		die("Error extracting message data from object: %s\n",
					bbus_strerror(bbus_lasterror()));
	}

	printf("Message %s\n", meta);
	printf("{\n");
	printf("\tmsgtype = %s\n", str_msgtype(msgtype));
	printf("\tsotype = %s\n", str_sotype(sotype));
	printf("\terrcode = %s\n", str_errcode(errcode));
	printf("\ttoken = %u\n", token);
	printf("\tpsize = %u\n", psize);
	printf("\tflags = %s\n", str_flags(flags));
	printf("\tmeta = \"%s\"\n", msgmeta);
	printf("}\n");
}

int main(int argc BBUS_UNUSED, char** argv BBUS_UNUSED)
{
	bbus_client_connection* conn;
	int ret;
	struct bbus_timeval tv;
	bbus_object* obj;
	const char* meta;

	conn = bbus_mon_connect();
	if (conn == NULL) {
		die("Error connecting to bbusd: %s\n",
			bbus_strerror(bbus_lasterror()));
	}

	(void)signal(SIGTERM, sighandler);
	(void)signal(SIGINT, sighandler);

	run = 1;
	while (do_run()) {
		tv.sec = 0;
		tv.usec = 500000;

		ret = bbus_mon_recvmsg(conn, msgbuf, BBUS_MAXMSGSIZE,
							&tv, &meta, &obj);
		if (ret < 0) {
			if (bbus_lasterror() == BBUS_EPOLLINTR) {
				continue;
			} else {
				die("Error receiving message: %s\n",
					bbus_strerror(bbus_lasterror()));
			}
		} else
		if (ret == 0) {
			/* Timeout. */
			continue;
		} else {
			print_msg_info(meta, obj);
		}
	}

	bbus_closeconn(conn);

	return 0;
}

