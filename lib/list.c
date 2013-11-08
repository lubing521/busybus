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

#include <busybus.h>

void bbus_list_push(void* list, void* elem)
{
	struct bbus_list_elem* el = (struct bbus_list_elem*)elem;
	struct bbus_list* lst = (struct bbus_list*)list;

	if (lst->head == NULL) {
		lst->head = lst->tail = el;
		el->next = NULL;
		el->prev = NULL;
	} else {
		el->next = NULL;
		el->prev = lst->tail;
		lst->tail->next = el;
		lst->tail = el;
	}
}

void bbus_list_insert(void* list, void* elem, void* prev)
{
	struct bbus_list_elem* el = (struct bbus_list_elem*)elem;
	struct bbus_list_elem* pr = (struct bbus_list_elem*)prev;
	struct bbus_list* lst = (struct bbus_list*)list;

	if (pr == NULL) {
		/* Assume the list is empty. */
		el->next = el->prev = NULL;
		lst->head = lst->tail = el;
	} else {
		el->prev = pr;
		el->next = pr->next;
		if (pr->next != NULL)
			pr->next->prev = el;
		pr->next = el;
		if (pr == lst->tail)
			lst->tail = el;
	}
}

void bbus_list_rm(void* list, void* elem)
{
	struct bbus_list_elem* el = (struct bbus_list_elem*)elem;
	struct bbus_list_elem* next = el->next;
	struct bbus_list_elem* prev = el->prev;
	struct bbus_list* lst = (struct bbus_list*)list;

	if (lst->head == lst->tail) {
		lst->head = lst->tail = NULL;
	} else {
		if (next != NULL)
			next->prev = prev;
		if (prev != NULL)
			prev->next = next;
		if (el == lst->head)
			lst->head = el->next;
		if (el == lst->tail)
			lst->tail = el->prev;
	}
}

