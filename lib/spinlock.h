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

#ifndef __BBUS_SPINLOCK__
#define __BBUS_SPINLOCK__

struct __bbus_spinlock
{
	volatile int val;
};

#define __BBUS_SPINLOCK_INITIALIZER { 0 }

void __bbus_spinlock_init(struct __bbus_spinlock* lock);
void __bbus_spinlock_lock(struct __bbus_spinlock* lock);
void __bbus_spinlock_unlock(struct __bbus_spinlock* lock);

#endif /* __BBUS_SPINLOCK__ */

