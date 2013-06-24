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

#ifndef __BBUS_PROTO__
#define __BBUS_PROTO__

int __bbus_recv_msg(int sock, void* buf, size_t bufsize);
int __bbus_send_msg(int sock, const void* buf, size_t bufsize);
void __bbus_hdr_setmagic(struct bbus_msg_hdr* hdr);
int __bbus_hdr_checkmagic(struct bbus_msg_hdr* hdr);

#endif /* __BBUS_PROTO__ */
