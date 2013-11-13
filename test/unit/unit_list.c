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

#include "bbus-unit.h"
#include <busybus.h>

struct listelem
{
	struct listelem* next;
	struct listelem* prev;
	int val;
};

struct list
{
	struct listelem* head;
	struct listelem* tail;
};

#define DEF_ELEM(VAL) struct listelem e##VAL = { NULL, NULL, (VAL) }

BBUSUNIT_DEFINE_TEST(list_push)
{
	BBUSUNIT_BEGINTEST;

		struct list l = { NULL, NULL };
		DEF_ELEM(1);
		DEF_ELEM(2);
		DEF_ELEM(3);

		bbus_list_push(&l, &e1);
		bbus_list_push(&l, &e2);
		bbus_list_push(&l, &e3);

		BBUSUNIT_ASSERT_EQ(1, l.head->val);
		BBUSUNIT_ASSERT_EQ(2, l.head->next->val);
		BBUSUNIT_ASSERT_EQ(2, l.tail->prev->val);
		BBUSUNIT_ASSERT_EQ(3, l.tail->val);
		BBUSUNIT_ASSERT_EQ(e2.prev, l.head);
		BBUSUNIT_ASSERT_EQ(e2.next, l.tail);
		BBUSUNIT_ASSERT_NULL(e1.prev);
		BBUSUNIT_ASSERT_NULL(e3.next);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(list_rm)
{
	BBUSUNIT_BEGINTEST;

		struct list l = { NULL, NULL };
		DEF_ELEM(1);
		DEF_ELEM(2);
		DEF_ELEM(3);
		DEF_ELEM(4);
		DEF_ELEM(5);

		bbus_list_push(&l, &e1);
		bbus_list_push(&l, &e2);
		bbus_list_push(&l, &e3);
		bbus_list_push(&l, &e4);
		bbus_list_push(&l, &e5);
		BBUSUNIT_ASSERT_EQ(1, l.head->val);
		BBUSUNIT_ASSERT_EQ(5, l.tail->val);

		bbus_list_rm(&l, &e1);
		bbus_list_rm(&l, &e3);
		bbus_list_rm(&l, &e5);
		BBUSUNIT_ASSERT_EQ(2, l.head->val);
		BBUSUNIT_ASSERT_EQ(4, l.head->next->val);
		BBUSUNIT_ASSERT_EQ(2, l.tail->prev->val);
		BBUSUNIT_ASSERT_EQ(4, l.tail->val);
		BBUSUNIT_ASSERT_EQ(&e2, l.head);
		BBUSUNIT_ASSERT_EQ(&e4, l.tail);
		BBUSUNIT_ASSERT_EQ(e4.prev, l.head);
		BBUSUNIT_ASSERT_EQ(e2.next, l.tail);
		BBUSUNIT_ASSERT_NULL(e2.prev);
		BBUSUNIT_ASSERT_NULL(e4.next);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(list_ins_middle)
{
	BBUSUNIT_BEGINTEST;

		struct list l = { NULL, NULL };
		DEF_ELEM(1);
		DEF_ELEM(2);
		DEF_ELEM(3);

		bbus_list_push(&l, &e1);
		bbus_list_push(&l, &e2);
		bbus_list_insert(&l, &e3, &e1);

		BBUSUNIT_ASSERT_EQ(&e1, l.head);
		BBUSUNIT_ASSERT_EQ(&e2, l.tail);
		BBUSUNIT_ASSERT_EQ(3, e2.prev->val);
		BBUSUNIT_ASSERT_EQ(e1.next->val, 3);
		BBUSUNIT_ASSERT_EQ(&e1, e3.prev);
		BBUSUNIT_ASSERT_EQ(&e2, e3.next);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(list_ins_first)
{
	BBUSUNIT_BEGINTEST;

		struct list l = { NULL, NULL };
		DEF_ELEM(1);

		bbus_list_insert(&l, &e1, NULL);
		BBUSUNIT_ASSERT_EQ(&e1, l.head);
		BBUSUNIT_ASSERT_EQ(&e1, l.tail);
		BBUSUNIT_ASSERT_NULL(e1.prev);
		BBUSUNIT_ASSERT_NULL(e1.next);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(list_ins_push_back)
{
	BBUSUNIT_BEGINTEST;

		struct list l = { NULL, NULL };
		DEF_ELEM(1);
		DEF_ELEM(2);
		DEF_ELEM(3);

		bbus_list_insert(&l, &e1, NULL);
		bbus_list_insert(&l, &e2, &e1);
		bbus_list_insert(&l, &e3, &e2);

		BBUSUNIT_ASSERT_EQ(1, l.head->val);
		BBUSUNIT_ASSERT_EQ(2, l.head->next->val);
		BBUSUNIT_ASSERT_EQ(2, l.tail->prev->val);
		BBUSUNIT_ASSERT_EQ(3, l.tail->val);
		BBUSUNIT_ASSERT_EQ(e2.prev, l.head);
		BBUSUNIT_ASSERT_EQ(e2.next, l.tail);
		BBUSUNIT_ASSERT_NULL(e1.prev);
		BBUSUNIT_ASSERT_NULL(e3.next);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

BBUSUNIT_DEFINE_TEST(list_rm_last)
{
	BBUSUNIT_BEGINTEST;

		struct list l = { NULL, NULL };
		DEF_ELEM(1);

		bbus_list_push(&l, &e1);
		BBUSUNIT_ASSERT_EQ(&e1, l.head);
		BBUSUNIT_ASSERT_EQ(&e1, l.tail);

		bbus_list_rm(&l, &e1);
		BBUSUNIT_ASSERT_NULL(l.head);
		BBUSUNIT_ASSERT_NULL(l.tail);

	BBUSUNIT_FINALLY;
	BBUSUNIT_ENDTEST;
}

