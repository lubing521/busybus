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
#include "spinlock.h"

void __bbus_spinlock_init(struct __bbus_spinlock* lock)
{
	lock->val = 0;
}

void __bbus_spinlock_lock(struct __bbus_spinlock* lock)
{
	do {
		while (lock->val);
	} while (BBUS_ATOMIC_LOCK_TEST_AND_SET(lock->val, 1));
}

void __bbus_spinlock_unlock(struct __bbus_spinlock* lock)
{
	(void)BBUS_ATOMIC_LOCK_RELEASE(lock->val);
}

