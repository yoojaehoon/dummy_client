/*
 * which v2.x -- print full path of executables
 * Copyright (C) 1999, 2003, 2007, 2008  Carlo Wood <carlo@gnu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "which.h"

static const char *progname;
static void print_fail(const char *name, const char *path_list)
{
	fprintf(stderr, "%s: no %s in (%s)\n", progname, name, path_list);
}

static char home[256];
static size_t homelen = 0;

static int absolute_path_given;
static int found_path_starts_with_dot;
static char *abs_path;

#ifdef _KSKIM
static int skip_dot = 0, skip_tilde = 0, skip_alias = 0, read_alias = 0;
static int show_dot = 0, show_tilde = 0, show_all = 0, tty_only = 0;
static int skip_functions = 0, read_functions = 0;
#else
static int skip_dot = 0, skip_tilde = 0; 
static int show_dot = 0, show_tilde = 0, show_all = 0;
#endif

static char *find_command_in_path(const char *name, const char *path_list, int *path_index)
{
	char *found = NULL, *full_path;
	int status, name_len;

	name_len = strlen(name);

	if (!absolute_program(name))
		absolute_path_given = 0;
	else
	{
		char *p;
		absolute_path_given = 1;

		if (abs_path)
			free(abs_path);

		if (*name != '.' && *name != '/' && *name != '~')
		{
			abs_path = (char *)xmalloc(3 + name_len);
			strcpy(abs_path, "./");
			strcat(abs_path, name);
		}
		else
		{
			abs_path = (char *)xmalloc(1 + name_len);
			strcpy(abs_path, name);
		}

		path_list = abs_path;
		p = strrchr(abs_path, '/');
		*p++ = 0;
		name = p;
	}

	while (path_list && path_list[*path_index])
	{
		char *path = NULL;

		if (absolute_path_given)
		{
			//path = savestring((const char*) path_list);
			path = savestring(path_list);
			*path_index = strlen(path);
		}
		else
			path = get_next_path_element(path_list, path_index);

		if (!path)
			break;

		if (*path == '~')
		{
			char *t = (char*) tilde_expand((const char*) path);
			free(path);
			path = t;

			if (skip_tilde)
			{
				free(path);
				continue;
			}
		}

		if (skip_dot && *path != '/')
		{
			free(path);
			continue;
		}

		found_path_starts_with_dot = (*path == '.');

		full_path = make_full_pathname(path, name, name_len);
		free(path);

		status = file_status(full_path);

		if ((status & FS_EXISTS) && (status & FS_EXECABLE))
		{
			found = full_path;
			break;
		}

		free(full_path);
	}

	return (found);
}

static char cwd[256];
static size_t cwdlen;

static void get_current_working_directory(void)
{
	if (cwdlen)
		return;

	if (!getcwd(cwd, sizeof(cwd)))
	{
		const char *pwd = (char*) getenv("PWD");
		if (pwd && strlen(pwd) < sizeof(cwd))
			strcpy(cwd, pwd);
	}

	if (*cwd != '/')
	{
		fprintf(stderr, "Can't get current working directory\n");
		exit(-1);
	}

	cwdlen = strlen(cwd);

	if (cwd[cwdlen - 1] != '/')
	{
		cwd[cwdlen++] = '/';
		cwd[cwdlen] = 0;
	}
}

static char *path_clean_up(const char *path)
{
	static char result[256];

	const char *p1 = path;
	char *p2 = result;

	int saw_slash = 0, saw_slash_dot = 0, saw_slash_dot_dot = 0;

	if (*p1 != '/')
	{
		get_current_working_directory();
		strcpy(result, cwd);
		saw_slash = 1;
		p2 = &result[cwdlen];
	}

	do
	{
		/*
		 * Two leading slashes are allowed, having an OS implementation-defined meaning.
		 * See http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap04.html#tag_04_11
		 */
		if (!saw_slash || *p1 != '/' || (p1 == path + 1 && p1[1] != '/'))
			*p2++ = *p1;
		if (saw_slash_dot && (*p1 == '/'))
			p2 -= 2;
		if (saw_slash_dot_dot && (*p1 == '/'))
		{
			int cnt = 0;
			do
			{
				if (--p2 < result)
				{
					strcpy(result, path);
					return result;
				}
				if (*p2 == '/')
					++cnt;
			}
			while (cnt != 3);
			++p2;
		}
		saw_slash_dot_dot = saw_slash_dot && (*p1 == '.');
		saw_slash_dot = saw_slash && (*p1 == '.');
		saw_slash = (*p1 == '/');
	}
	while (*p1++);

	return result;
}

int path_search(const char *cmd, const char *path_list, char *szFullPath,
		int nFullPathSize)
{
	char *result = NULL;
	int found_something = 0;

	memset(szFullPath, 0, nFullPathSize);

	if (path_list && *path_list != '\0')
	{
		int next;
		int path_index = 0;
		do
		{
			next = show_all;
			result = find_command_in_path(cmd, path_list, &path_index);
			if (result)
			{
				const char *full_path = path_clean_up(result);
				int in_home = (show_tilde || skip_tilde) && !strncmp(full_path, home, homelen);
				if (!(skip_tilde && in_home) && show_dot && found_path_starts_with_dot && !strncmp(full_path, cwd, cwdlen))
				{
					full_path += cwdlen;
					fprintf(stdout, "./");
				}
				else if (in_home)
				{
					if (skip_tilde)
					{
						next = 1;
						free(result);
						continue;
					}
					if (show_tilde)
					{
						full_path += homelen;
						fprintf(stdout, "~/");
					}
				}
#ifdef _KSKIM
				fprintf(stdout, "%s\n", full_path);
#endif
				strncpy(szFullPath, full_path, nFullPathSize);
				free(result);
				found_something = 1;
			}
			else
				break;
		}
		while (next);
	}

	return found_something;
}

enum opts {
	opt_version,
	opt_skip_dot,
	opt_skip_tilde,
	opt_skip_alias,
	opt_read_functions,
	opt_skip_functions,
	opt_show_dot,
	opt_show_tilde,
	opt_tty_only,
	opt_help
};

#ifdef _KSKIM
#ifdef __TANDEM
/* According to Tom Bates, <tom.bates@hp.com> */
static uid_t const superuser = 65535;
#else
static uid_t const superuser = 0;
#endif
#endif

int GetExecPath(const char *cmd, char *pszPath, int nPathSize)
{
	int nRet = 1;
	const char *path_list = (char*) getenv("PATH");
	uidget();
	if(!path_search(cmd, path_list, pszPath, nPathSize)) {
		nRet = 0;
		printf("path: %s\n", path_list);
		print_fail(absolute_path_given ? strrchr(cmd, '/') + 1 : cmd,
				absolute_path_given ? abs_path : path_list);
	}
	if(nRet) {
		int i=0;
		for(i=strlen(pszPath)-1; i>0; i--) {
			if(pszPath[i] == '/') {
				pszPath[i] = '\0';
				break;
			}
		}
	}
	
	return nRet;
}
#ifdef _WHICH_TEST
int main(int argc, char *argv[])
{
	const char *path_list = (char*) getenv("PATH");
#ifdef _KSKIM
	printf(" skip_dot = %d, skip_tilde = %d, skip_alias = %d, read_alias = %d\n"
			"show_dot = %d, show_tilde = %d, show_all = %d, tty_only = %d\n"
			"skip_functions = %d, read_functions = %d\n ",
			skip_dot, skip_tilde, skip_alias, read_alias,
			show_dot, show_tilde, show_all, tty_only,
			skip_functions, read_functions);
#endif
	uidget();

	argv++;

	printf("argc : %d, %s\n", argc, *argv);
	if(!path_search(0, *argv, path_list))
	{
		printf("path: %s\n", path_list);
		print_fail(absolute_path_given ? strrchr(*argv, '/') + 1 : *argv,
				absolute_path_given ? abs_path : path_list);
	}

	return 0;
}
#endif

