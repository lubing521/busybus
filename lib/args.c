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
			case BBUS_OPT_ARGREQ:
				size += 2;
				break;
			case BBUS_OPT_ARGOPT:
				size += 3;
				break;
			case BBUS_OPT_NOARG:
			default:
				++size;
				break;
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
			case BBUS_OPT_ARGREQ:
				*tmp++ = (char)optlist->opts[i].shortopt;
				*tmp++ = ':';
				break;
			case BBUS_OPT_ARGOPT:
				*tmp++ = (char)optlist->opts[i].shortopt;
				*tmp++ = ':';
				*tmp++ = ':';
				break;
			case BBUS_OPT_NOARG:
			default:
				*tmp++ = (char)optlist->opts[i].shortopt;
				break;
			}
		}
	}

	return ret;
}

/* Values for the '--help' and '--version' options. */
#define OPT_HELP	-1
#define OPT_VERSION	-2

static struct option* make_longopts(const struct bbus_opt_list* optlist,
								int* flag)
{
	struct option* ret;
	unsigned i;
	/* Set those below to 2 for 'help' and 'version' options. */
	unsigned j = 2;
	size_t size = 2;

	for (i = 0; i < optlist->numopts; ++i)
		if (optlist->opts[i].longopt != NULL)
			++size;

	ret = bbus_malloc0((size+1) * sizeof(struct option));
	if (ret == NULL)
		return NULL;

	ret[0].name = "help";
	ret[0].has_arg = no_argument;
	ret[0].flag = flag;
	ret[0].val = OPT_HELP;
	ret[1].name = "version";
	ret[1].has_arg = no_argument;
	ret[1].flag = flag;
	ret[1].val = OPT_VERSION;

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
			case BBUS_OPT_ARGREQ:
				ret[j].has_arg = required_argument;
				break;
			case BBUS_OPT_ARGOPT:
				ret[j].has_arg = optional_argument;
				break;
			case BBUS_OPT_NOARG:
			default:
				ret[j].has_arg = no_argument;
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

	/* We expect argv to be properly permuted and optind set. */
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
	unsigned i;

	info = make_info_string(optlist);
	if (info == NULL)
		goto out_of_memory;

	shortopts = make_shortopts(optlist);
	if (shortopts == NULL)
		goto out_of_memory;

	longopts = make_longopts(optlist, &flag);
	if (longopts == NULL)
		goto out_of_memory;

	while ((opt = getopt_long(argc, argv, shortopts,
					longopts, &ind)) != -1) {
		switch (opt) {
		case '?':
		case ':':
			fprintf(stderr, "try %s --help\n", argv[0]);
			ret = -1;
			goto out;
			break;
		case 0:
			/* Long option. */
			if (flag < 0) {
				if (flag == OPT_VERSION) {
					fprintf(stdout, "%s %s\n",
							optlist->progname,
							optlist->version);
				} else {
					fprintf(stdout, "%s\n", info);
				}
				ret = -1;
				goto out;
			} else {
				curopt = &optlist->opts[flag];
			}
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
			((bbus_opt_callback)curopt->actdata)(optarg);
			break;
		default:
			ret = -1;
			goto out;
		}
	}

	/* TODO Parse and interpret positional arguments as well? */
	if (nonopts != NULL) {
		*nonopts = find_nonopts(argc, argv);
		if (*nonopts == NULL)
			goto out_of_memory;
	}

	goto out;

out_of_memory:
	fprintf(stderr, "%s: %s\n", __FUNCTION__, bbus_strerror(BBUS_ENOMEM));
	ret = -1;

out:
	bbus_free(shortopts);
	bbus_free(longopts);
	bbus_free(info);
	return ret;
}
