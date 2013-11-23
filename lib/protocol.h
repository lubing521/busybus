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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __BBUS_PROTO__
#define __BBUS_PROTO__

#include <busybus.h>

int __bbus_prot_recvmsg(int sock, struct bbus_msg* buf, size_t bufsize);
int __bbus_prot_sendmsg(int sock, const struct bbus_msg* buf);
int __bbus_prot_recvvmsg(int sock, struct bbus_msg_hdr* hdr,
		void* payload, size_t psize);
int __bbus_prot_sendvmsg(int sock, const struct bbus_msg_hdr* hdr,
		const char* meta, const char* obj, size_t objsize);
void __bbus_prot_hdrsetmagic(struct bbus_msg_hdr* hdr);
int __bbus_prot_errtoerrnum(uint8_t errcode);

#endif /* __BBUS_PROTO__ */
