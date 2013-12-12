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

#include "monitor.h"
#include "log.h"
#include <string.h>

static struct bbusd_clientlist monitors = { NULL, NULL };

int bbusd_monlist_add(bbus_client* cli)
{
	return __bbusd_clientlist_add(cli, &monitors);
}

void bbusd_monlist_rm(bbus_client* cli)
{
	struct bbusd_clientlist_elem* mon;

	for (mon = monitors.head; mon != NULL; mon = mon->next) {
		if (mon->cli == cli) {
			__bbusd_clientlist_rm(&mon, &monitors);
			return;
		}
	}

	bbusd_logmsg(BBUS_LOG_WARN,
		"Monitor not found in the list, "
		"this should not happen.\n");
}

static bbus_object* pack_msg(const struct bbus_msg_hdr* hdr, const char* meta)
{
	bbus_object* obj;

	obj = bbus_obj_build("bbbuubs",
				hdr->msgtype,
				hdr->sotype,
				hdr->errcode,
				bbus_hdr_gettoken(hdr),
				bbus_hdr_getpsize(hdr),
				hdr->flags,
				meta);
	if (obj == NULL) {
		bbusd_logmsg(BBUS_LOG_ERR,
			"Error creating the message for monitors: %s\n",
			bbus_strerror(bbus_lasterror()));
	}

	return obj;
}

/*
 * Send a prepared message to all monitors. Deletes the object before
 * returning.
 */
static void send_to_monitors(const char* meta, bbus_object* obj)
{
	struct bbus_msg_hdr hdr;
	int ret;
	struct bbusd_clientlist_elem* mon;

	bbus_hdr_build(&hdr, BBUS_MSGTYPE_MON, BBUS_PROT_EGOOD);
	bbus_hdr_setpsize(&hdr, (meta == NULL ? 0 : strlen(meta)+1) +
						bbus_obj_rawsize(obj));
	if (meta)
		BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASMETA);
	if (obj)
		BBUS_HDR_SETFLAG(&hdr, BBUS_PROT_HASOBJECT);

	for (mon = monitors.head; mon != NULL; mon = mon->next) {
		ret = bbus_client_sendmsg(mon->cli, &hdr, meta, obj);
		if (ret < 0) {
			bbusd_logmsg(BBUS_LOG_ERR,
				"Error sending a message to monitor: %s\n",
				bbus_strerror(bbus_lasterror()));
		}
	}
}

void bbusd_mon_notify_recvd(const struct bbus_msg* msg)
{
	bbus_object* obj;
	const char* meta;

	if (BBUS_HDR_ISFLAGSET(&msg->hdr, BBUS_PROT_HASMETA)) {
		meta = bbus_prot_extractmeta(msg);
		if (meta == NULL) {
			bbusd_logmsg(BBUS_LOG_ERR,
				"Error extracting the meta string from "
				"message: %s\n",
				bbus_strerror(bbus_lasterror()));
			return;
		}
	} else {
		meta = "";
	}

	obj = pack_msg(&msg->hdr, meta);
	if (obj == NULL)
		return;

	send_to_monitors("received", obj);
}

void bbusd_mon_notify_sent(const struct bbus_msg_hdr* hdr,
				const char* meta, bbus_object* obj)
{
	obj = pack_msg(hdr, meta == NULL ? "" : meta);
	if (obj == NULL)
		return;

	send_to_monitors("sent", obj);
}

