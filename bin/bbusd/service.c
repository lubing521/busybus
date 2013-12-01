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
#include "common.h"
#include "log.h"
#include "service.h"
#include <string.h>

struct service_tree
{
	/* Values are pointers to struct service_map. */
	bbus_hashmap* subsrvc;
	/* Values are pointers to struct method. */
	bbus_hashmap* methods;
};

static struct service_tree* srvc_tree;

static int do_insert_method(const char* path, struct bbusd_method* mthd,
					struct service_tree* node)
{
	char* found;
	struct service_tree* next;
	int ret;
	void* mval;

	found = index(path, '.');
	if (found == NULL) {
		/* Path is the method name. */
		mval = bbus_hmap_findstr(node->methods, path);
		if (mval != NULL) {
			bbusd_logmsg(BBUS_LOG_ERR,
				"Method already exists for this value: %s\n",
				path);
			return -1;
		}
		ret = bbus_hmap_setstr(node->methods, path, mthd);
		if (ret < 0) {
			bbusd_logmsg(BBUS_LOG_ERR,
				"Error registering new method: %s\n",
				bbus_strerror(bbus_lasterror()));
			return -1;
		}

		return 0;
	} else {
		/* Path is the subservice name. */
		*found = '\0';
		next = bbus_hmap_findstr(node->subsrvc, path);
		if (next == NULL) {
			/* Insert new service. */
			next = bbus_malloc(sizeof(struct service_tree));
			if (next == NULL)
				goto err_mknext;

			next->subsrvc = bbus_hmap_create(BBUS_HMAP_KEYSTR);
			if (next->subsrvc == NULL)
				goto err_mksubsrvc;

			next->methods = bbus_hmap_create(BBUS_HMAP_KEYSTR);
			if (next->subsrvc == NULL)
				goto err_mkmethods;

			ret = bbus_hmap_setstr(node->subsrvc, path, next);
			if (ret < 0)
				goto err_setsrvc;
		}

		return do_insert_method(found+1, mthd, next);
	}

err_setsrvc:
	bbus_hmap_free(next->methods);

err_mkmethods:
	bbus_hmap_free(next->subsrvc);

err_mksubsrvc:
	bbus_free(next);

err_mknext:
	return -1;
}

int bbusd_insert_method(const char* path, struct bbusd_method* mthd)
{
	char* mname;
	int ret;

	mname = bbus_str_cpy(path);
	if (mname == NULL)
		return -1;

	ret = do_insert_method(mname, mthd, srvc_tree);
	bbus_str_free(mname);

	return ret;
}

static struct bbusd_method* do_locate_method(char* path, struct service_tree* node)
{
	char* found;
	struct service_tree* next;

	found = index(path, '.');
	if (found == NULL) {
		/* This is a method. */
		return bbus_hmap_findstr(node->methods, path);
	} else {
		*found = '\0';
		/* This is a sub-service. */
		next = bbus_hmap_findstr(node->subsrvc, path);
		if (next == NULL) {
			return NULL;
		}

		return do_locate_method(found+1, next);
	}
}

struct bbusd_method* bbusd_locate_method(const char* path)
{
	char* mname;
	struct bbusd_method* ret;

	mname = bbus_str_cpy(path);
	if (mname == NULL)
		return NULL;

	ret = do_locate_method(mname, srvc_tree);
	bbus_str_free(mname);

	return ret;
}

void bbusd_init_service_map(void)
{
	srvc_tree = bbus_malloc(sizeof(struct service_tree));
	if (srvc_tree == NULL)
		goto err;

	srvc_tree->subsrvc = bbus_hmap_create(BBUS_HMAP_KEYSTR);
	if (srvc_tree->subsrvc == NULL) {
		bbus_free(srvc_tree);
		goto err;
	}

	srvc_tree->methods = bbus_hmap_create(BBUS_HMAP_KEYSTR);
	if (srvc_tree->methods == NULL) {
		bbus_hmap_free(srvc_tree->subsrvc);
		bbus_free(srvc_tree);
		goto err;
	}

	return;

err:
	bbusd_die("Error creating the service map: %s\n",
		bbus_strerror(bbus_lasterror()));
}

void bbusd_free_service_map(void)
{
	bbus_hmap_free(srvc_tree->methods);
	bbus_hmap_free(srvc_tree->subsrvc);
	bbus_free(srvc_tree);
}

