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

#include <busybus.h>
#include "error.h"
#include <regex.h>

int bbus_regex_match(const char* pattern, const char* str)
{
	int ret;
	regex_t regex;

	ret = regcomp(&regex, pattern, REG_EXTENDED | REG_NEWLINE);
	if (ret != REG_NOERROR) {
		if (ret == REG_ESPACE) {
			__bbus_seterr(BBUS_ENOMEM);
		} else {
			__bbus_seterr(BBUS_EREGEXPTRN);
		}
		return -1;
	}

	ret = regexec(&regex, str, 0, NULL, 0);
	if (ret != REG_NOERROR) {
		if (ret == REG_NOMATCH) {
			ret = BBUS_FALSE;
		} else {
			/*
			 * This seems to be the only error
			 * regexec can return.
			 */
			__bbus_seterr(BBUS_ENOMEM);
			ret = -1;
		}
		goto out;
	}

	ret = BBUS_TRUE;

out:
	regfree(&regex);
	return ret;
}
