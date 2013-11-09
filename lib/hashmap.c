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
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <search.h>
#include <stdio.h>

#define DEF_MAP_SIZE 32

struct map_entry
{
	struct map_entry* next;
	struct map_entry* prev;
	void* key;
	size_t ksize;
	void* val;
};

struct __bbus_hashmap
{
	size_t size;
	size_t numstored;
	struct map_entry** bucket_heads;
	enum bbus_hmap_type type;
};

static bbus_hashmap* create_hashmap(enum bbus_hmap_type type, size_t size)
{
	bbus_hashmap* hmap;
	unsigned i;

	hmap = bbus_malloc(sizeof(struct __bbus_hashmap));
	if (hmap == NULL)
		return NULL;

	hmap->bucket_heads = bbus_malloc(
				size * sizeof(struct map_entry*));
	if (hmap->bucket_heads == NULL) {
		bbus_free(hmap);
		return NULL;
	}

	for (i = 0; i < size; ++i)
		hmap->bucket_heads[i] = NULL;
	hmap->size = size;
	hmap->numstored = 0;
	hmap->type = type;

	return hmap;
}

bbus_hashmap* bbus_hmap_create(enum bbus_hmap_type type)
{
	return create_hashmap(type, DEF_MAP_SIZE);
}

/* Prototype for enlarge_map(). */
static int hmap_set(bbus_hashmap* hmap, const void* key,
					size_t ksize, void* val);

static int enlarge_map(bbus_hashmap* hmap)
{
	bbus_hashmap* newmap;
	struct map_entry* el;
	unsigned i;
	int r;

	newmap = create_hashmap(hmap->type, hmap->size * 2);
	if (newmap == NULL)
		return -1;
	for (i = 0; i < hmap->size; ++i) {
		for (el = hmap->bucket_heads[i]; el != NULL; el = el->next) {
			r = hmap_set(newmap, el->key,
						el->ksize, el->val);
			if (r < 0) {
				bbus_hmap_free(newmap);
				return -1;
			}
		}
	}

	bbus_hmap_reset(hmap);
	bbus_free(hmap->bucket_heads);
	hmap->size = newmap->size;
	hmap->numstored = newmap->numstored;
	hmap->bucket_heads = newmap->bucket_heads;
	bbus_free(newmap); /* Free only the pointer to avoid a memory leak. */

	return 0;
}

static int hmap_set(bbus_hashmap* hmap, const void* key,
					size_t ksize, void* val)
{
	uint32_t crc;
	unsigned ind;
	int r;
	struct map_entry* tmpel;
	struct map_entry* tail;
	struct map_entry* newel;

	if (hmap->numstored == hmap->size) {
		r = enlarge_map(hmap);
		if (r < 0)
			return -1;
	}

	crc = bbus_crc32(key, ksize);
	ind = crc % hmap->size;
	if (hmap->bucket_heads[ind] == NULL) {
		hmap->bucket_heads[ind] = bbus_malloc(sizeof(
						struct map_entry));
		if (hmap->bucket_heads[ind] == NULL)
			return -1;
		hmap->bucket_heads[ind]->next = NULL;
		hmap->bucket_heads[ind]->prev = NULL;
		hmap->bucket_heads[ind]->key = bbus_memdup(key, ksize);
		if (hmap->bucket_heads[ind]->key == NULL) {
			bbus_free(hmap->bucket_heads[ind]);
			hmap->bucket_heads[ind] = NULL;
			return -1;
		}
		hmap->bucket_heads[ind]->ksize = ksize;
		hmap->bucket_heads[ind]->val = val;
	} else {
		for (tmpel = hmap->bucket_heads[ind];
				tmpel != NULL; tmpel = tmpel->next) {
			if (memcmp(tmpel->key, key,
					BBUS_MIN(tmpel->ksize, ksize)) == 0) {
				tmpel->val = val;
				return 0;
			}
			if (tmpel->next == NULL)
				tail = tmpel;
		}
		newel = bbus_malloc(sizeof(struct map_entry));
		if (newel == NULL)
			return -1;
		newel->key = bbus_memdup(key, ksize);
		if (newel->key == NULL) {
			bbus_free(newel);
			return -1;
		}
		newel->ksize = ksize;
		newel->val = val;
		insque(newel, tail);
	}

	hmap->numstored++;
	return 0;
}

static struct map_entry* locate_entry(bbus_hashmap* hmap,
		const void* key, size_t ksize)
{
	uint32_t crc;
	unsigned ind;
	struct map_entry* entr;

	crc = bbus_crc32(key, ksize);
	ind = crc % hmap->size;
	if (hmap->bucket_heads[ind] == NULL)
		goto noelem;

	for (entr = hmap->bucket_heads[ind];
			entr != NULL; entr = entr->next) {
		if (memcmp(entr->key, key, BBUS_MIN(entr->ksize, ksize)) == 0)
			return entr;
	}

noelem:
	return NULL;
}

static void* hmap_find(bbus_hashmap* hmap, const void* key,
		size_t ksize)
{
	struct map_entry* entr;

	entr = locate_entry(hmap, key, ksize);
	if (entr == NULL)
		return NULL;
	return entr->val;
}

static void* hmap_rm(bbus_hashmap* hmap, const void* key, size_t ksize)
{
	struct map_entry* entr;
	void* ret;

	entr = locate_entry(hmap, key, ksize);
	if (entr == NULL)
		return NULL;
	ret = entr->val;
	remque(entr);
	bbus_free(entr->key);
	bbus_free(entr);
	hmap->numstored--;

	return ret;
}

#define CHECK_HMAP_TYPE(MAP, EXPECTED, RET)				\
	do {								\
		if ((MAP)->type != (EXPECTED)) {			\
			__bbus_seterr(BBUS_EHMAPINVTYPE);		\
			return (RET);					\
		}							\
	} while (0)

int bbus_hmap_setstr(bbus_hashmap* hmap, const char* key, void* val)
{
	CHECK_HMAP_TYPE(hmap, BBUS_HMAP_KEYSTR, -1);
	return hmap_set(hmap, key, strlen(key), val);
}

void* bbus_hmap_findstr(bbus_hashmap* hmap, const char* key)
{
	CHECK_HMAP_TYPE(hmap, BBUS_HMAP_KEYSTR, NULL);
	return hmap_find(hmap, key, strlen(key));
}

void* bbus_hmap_rmstr(bbus_hashmap* hmap, const char* key)
{
	CHECK_HMAP_TYPE(hmap, BBUS_HMAP_KEYSTR, NULL);
	return hmap_rm(hmap, key, strlen(key));
}

int bbus_hmap_setuint(bbus_hashmap* hmap, unsigned key, void* val)
{
	CHECK_HMAP_TYPE(hmap, BBUS_HMAP_KEYUINT, -1);
	return hmap_set(hmap, &key, sizeof(unsigned), val);
}

void* bbus_hmap_finduint(bbus_hashmap* hmap, unsigned key)
{
	CHECK_HMAP_TYPE(hmap, BBUS_HMAP_KEYUINT, NULL);
	return hmap_find(hmap, &key, sizeof(unsigned));
}

void* bbus_hmap_rmuint(bbus_hashmap* hmap, unsigned key)
{
	CHECK_HMAP_TYPE(hmap, BBUS_HMAP_KEYUINT, NULL);
	return hmap_rm(hmap, &key, sizeof(unsigned));
}

void bbus_hmap_reset(bbus_hashmap* hmap)
{
	unsigned i;
	struct map_entry* el;
	struct map_entry* tmpel;

	for (i = 0; i < hmap->size; ++i) {
		el = hmap->bucket_heads[i];
		while (el != NULL) {
			tmpel = el;
			el = el->next;
			bbus_free(tmpel->key);
			bbus_free(tmpel);
		}
		hmap->bucket_heads[i] = NULL;
	}
}

void bbus_hmap_free(bbus_hashmap* hmap)
{
	if (hmap) {
		bbus_hmap_reset(hmap);
		bbus_free(hmap->bucket_heads);
		bbus_free(hmap);
	}
}

static int BBUS_PRINTF_FUNC(3, 4) dump_append(char** buf,
		size_t* bufsize, const char* fmt, ...)
{
	int r;
	va_list va;

	va_start(va, fmt);
	r = vsnprintf(*buf, *bufsize, fmt, va);
	if (r >= (int)(*bufsize)) {
		__bbus_seterr(BBUS_ENOSPACE);
		return -1;
	}
	*buf += r;
	*bufsize -= r;

	return 0;
}

static const char* keyrepr(const void* src, size_t srcsize BBUS_UNUSED,
				char* dst BBUS_UNUSED,
				size_t dstsize BBUS_UNUSED)
{
	return (const char*)src; /* TODO Do a proper conversion. */
}

int bbus_hmap_dump(bbus_hashmap* hmap, char* buf, size_t bufsize)
{
	unsigned i;
	int r;
	struct map_entry* el;
	char reprbuf[1024];

	memset(buf, 0, bufsize);
	r = dump_append(&buf, &bufsize,
			"Hashmap size: %u, objects stored: %u\n",
			(unsigned)hmap->size, (unsigned)hmap->numstored);
	if (r < 0)
		return -1;

	for (i = 0; i < hmap->size; ++i) {
		r = dump_append(&buf, &bufsize, "Bucket nr %u:\n%s", i,
				hmap->bucket_heads[i] == NULL ? "" : "| ");
		if (r < 0)
			return -1;
		el = hmap->bucket_heads[i];
		while (el != NULL) {
			r = dump_append(&buf, &bufsize,
				" [\"%s\"]->[0x%p] |%s",
				keyrepr(el->key, el->ksize, reprbuf,
						sizeof(reprbuf)),
				el->val,
				el->next == NULL ? "\n" : "");
			if (r < 0)
				return -1;
			el = el->next;
		}
	}

	return 0;
}



