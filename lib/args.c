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
#include <getopt.h>
#include <stdio.h>

static char* make_shortopts(const struct bbus_opt_list* optlist)
{
	unsigned i;
	size_t size = 0;
	char* ret;
	char* tmp;

	for (i = 0; i < optlist->numopts; ++i) {
		if (optlist->opts[i].shortopt != 0) {
			switch (optlist->opts[i].hasarg) {
			case BBUS_OPT_NOARG: ++size; break;
			case BBUS_OPT_ARGREQ: size += 2; break;
			case BBUS_OPT_ARGOPT: size += 3; break;
			default: return NULL; /* Error notification. */
			}
		}
	}

	ret = bbus_malloc0(size);
	if (ret == NULL)
		return NULL;

	tmp = ret;
	for (i = 0; i < optlist->numopts; ++i) {
		if (optlist->opts[i].shortopt != 0) {
			switch (optlist->opts[i].hasarg) {
			case BBUS_OPT_NOARG:
				*tmp++ = (char)optlist->opts[i].shortopt;
				break;
			case BBUS_OPT_ARGREQ:
				*tmp++ = (char)optlist->opts[i].shortopt;
				*tmp++ = ':';
				break;
			case BBUS_OPT_ARGOPT:
				*tmp++ = (char)optlist->opts[i].shortopt;
				*tmp++ = ':';
				*tmp++ = ':';
				break;
			default:
				return NULL; /* Error notification. */
			}
		}
	}

	return ret;
}

static struct option* make_longopts(const struct bbus_opt_list* optlist,
								int* flag)
{
	struct option* ret;
	unsigned i, j;
	size_t size = 0;

	for (i = 0; i < optlist->numopts; ++i)
		if (optlist->opts[i].longopt != NULL)
			++size;

	ret = bbus_malloc0((size+1) * sizeof(struct option));
	if (ret == NULL)
		return NULL;

	j = 0;
	for (i = 0; i < optlist->numopts; ++i) {
		if (optlist->opts[i].longopt != NULL) {
			/*
			 * This makes getopt_long store the index of the
			 * corresponding bbus_option at the address specified
			 * by 'flag'. Later, after getting a long option we
			 * will be able to locate proper bbus_option structs.
			 */
			ret[j].name = optlist->opts[i].longopt;
			ret[j].flag = flag;
			ret[j].val = i;
			switch (optlist->opts[i].hasarg) {
			case BBUS_OPT_NOARG:
				ret[j].has_arg = no_argument;
				break;
			case BBUS_OPT_ARGREQ:
				ret[j].has_arg = required_argument;
				break;
			case BBUS_OPT_ARGOPT:
				ret[j].has_arg = optional_argument;
				break;
			}
			++j;
		}
	}

	return ret;
}

static char* make_info_string(const struct bbus_opt_list* optlist BBUS_UNUSED)
{
	return bbus_str_build("dummy");
}

void bbus_free_nonopts(struct bbus_nonopts* nonopts)
{
	bbus_free(nonopts->args);
	bbus_free(nonopts);
}

static struct bbus_nonopts* find_nonopts(int argc, char** argv)
{
	int i;
	int j = 0;
	struct bbus_nonopts* nonopts;
	size_t arrsize = 0;

	/* We expect argv to properly permuted and optind set. */
	arrsize = argc - optind;
	nonopts = bbus_malloc0(sizeof(struct bbus_nonopts));
	if (nonopts == NULL)
		return NULL;
	nonopts->args = bbus_malloc0(arrsize * sizeof(char*));
	if (nonopts->args == NULL) {
		bbus_free(nonopts);
		return NULL;
	}
	nonopts->numargs = arrsize;

	for (i = optind; i < argc; ++i, ++j) {
		nonopts->args[j] = argv[i];
	}

	return nonopts;
}

int bbus_parse_args(int argc, char** argv, const struct bbus_opt_list* optlist,
						struct bbus_nonopts** nonopts)
{
	char* shortopts = NULL;
	struct option* longopts = NULL;
	const struct bbus_option* curopt = NULL;
	int opt;
	int ind = 0;
	int ret = 0;
	int flag;
	char* info = NULL;
	int cbret;
	unsigned i;

	info = make_info_string(optlist);
	if (info == NULL) {
		/* TODO print error. */
		ret = -1;
		goto out;
	}

	shortopts = make_shortopts(optlist);
	if (shortopts == NULL) {
		/* TODO print error. */
		ret = -1;
		goto out;
	}

	longopts = make_longopts(optlist, &flag);
	if (longopts == NULL) {
		/* TODO print error. */
		ret = -1;
		goto out;
	}

	//opterr = 0;
	while ((opt = getopt_long(argc, argv, shortopts,
					longopts, &ind)) != -1) {
		switch (opt) {
		case '?':
		case ':':
			fprintf(stdout, "%s\n", info);
			ret = -1;
			goto out;
			break;
		case 0:
			/* Long option. */
			curopt = &optlist->opts[flag];
			break;
		default:
			/*
			 * Short option.
			 *
			 * FIXME Probably should find a better way to find
			 * the corresponding structure.
			 */
			for (i = 0; i < optlist->numopts; ++i) {
				if (optlist->opts[i].shortopt != 0) {
					if (optlist->opts[i].shortopt == opt) {
						curopt = &optlist->opts[i];
					}
				}
			}
			break;
		}

		/* Now actually handle the option. */
		switch (curopt->action) {
		case BBUS_OPTACT_NOTHING:
			break;
		case BBUS_OPTACT_SETFLAG:
			*((int*)curopt->actdata) = 1;
			break;
		case BBUS_OPTACT_GETOPTARG:
			*((char**)curopt->actdata) = optarg;
			break;
		case BBUS_OPTACT_CALLFUNC:
			cbret = ((bbus_opt_callback)curopt->actdata)(optarg);
			if (cbret < 0)
				/* TODO Error indication. */
				return -1;
			break;
		default:
			ret = -1;
			goto out;
		}
	}

	if (nonopts != NULL) {
		*nonopts = find_nonopts(argc, argv);
		if (*nonopts == NULL) {
			ret = -1;
			goto out;
		}
	}

out:
	bbus_free(shortopts);
	bbus_free(longopts);
	bbus_free(info);
	return ret;
}
