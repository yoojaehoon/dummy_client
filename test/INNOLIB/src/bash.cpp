/*
 * Copyright (C) 1987 - 2002 Free Software Foundation, Inc.
 *
 * This file is based on stuff from GNU Bash 1.14.7, the Bourne Again SHell.
 * Everything that was changed is marked with the word `CHANGED'.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02110-1301, USA.
 */

# include <string.h>
# include <sys/types.h>
# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>

# include <sys/stat.h>
#include <pwd.h>
#include "bash.h"

#define AFS (1)  // added by kskim. 2008.9.1
#define GETGROUPS_T int // added by kskim. 2008.9.1

/* Use the type that was determined by configure. */
#define GID_T GETGROUPS_T

/*
 * CHANGED:
 * Perhaps these need new configure.in entries.
 * The following macro's are used in bash, and below:
 */
#ifdef _KSKIM
#define HAVE_GETGROUPS
#undef SHELL
#undef AFS
#undef NOGROUP
#endif

/*
 * CHANGED:
 * - Added prototypes,
 * - used ANSI function arguments,
 * - made all functions static and
 * - changed all occurences of 'char *' into 'char const*' where possible.
 * - changed all occurences of 'gid_t' into 'GID_T'.
 * - exported functions needed in which.c
 */
static char* extract_colon_unit (char const* string, int* p_index);

/*===========================================================================
 *
 * Everything below is from bash-3.2.
 *
 */

/* From bash-3.2 / shell.h / line 105 */
/* Information about the current user. */
struct user_info {
  uid_t uid, euid;
  GID_T gid, egid;
  char *user_name;
  char *shell;          /* shell from the password file */
  char *home_dir;
};

/* From bash-3.2 / shell.c / line 111 */
/* Information about the current user. */
struct user_info current_user =
{
  (uid_t)-1, (uid_t)-1, (GID_T)-1, (GID_T)-1,
  (char *)NULL, (char *)NULL, (char *)NULL
};

/* From bash-3.2 / general.h / line 153 */
#define FREE(s)  do { if (s) free (s); } while (0)

/* From bash-3.2 / shell.c / line 1136 */
/* Fetch the current set of uids and gids and return 1 if we're running
   setuid or setgid. */
int
uidget ()
{
  uid_t u;

  u = getuid ();
  if (current_user.uid != u)
    {
      FREE (current_user.user_name);
      FREE (current_user.shell);
      FREE (current_user.home_dir);
      current_user.user_name = current_user.shell = current_user.home_dir = (char *)NULL;
    }
  current_user.uid = u;
  current_user.gid = getgid ();
  current_user.euid = geteuid ();
  current_user.egid = getegid ();

  /* See whether or not we are running setuid or setgid. */
  return (current_user.uid != current_user.euid) ||
           (current_user.gid != current_user.egid);
}

/* From bash-3.2 / lib/sh/oslib.c / line 245 */
#define DEFAULT_MAXGROUPS 64

/* From bash-3.2 / findcmd.c / line 75 */
/* Return some flags based on information about this file.
   The EXISTS bit is non-zero if the file is found.
   The EXECABLE bit is non-zero the file is executble.
   Zero is returned if the file is not found. */
int
file_status (char const* name)
{
  struct stat finfo;
  int r;

  /* Determine whether this file exists or not. */
  if (stat (name, &finfo) < 0)
    return (0);

  /* If the file is a directory, then it is not "executable" in the
     sense of the shell. */
  if (S_ISDIR (finfo.st_mode))
    return (FS_EXISTS|FS_DIRECTORY);

  r = FS_EXISTS;

  /* We have to use access(2) to determine access because AFS does not
     support Unix file system semantics.  This may produce wrong
     answers for non-AFS files when ruid != euid.  I hate AFS. */
  if (access (name, X_OK) == 0)
    r |= FS_EXECABLE;
  if (access (name, R_OK) == 0)
    r |= FS_READABLE;

  return r;
}

/* From bash-3.2 / general.c / line 534 ; Changes: Using 'strchr' instead of 'xstrchr'. */
/* Return 1 if STRING is an absolute program name; it is absolute if it
   contains any slashes.  This is used to decide whether or not to look
   up through $PATH. */
int
absolute_program (char const* string)
{
  return ((char *)strchr (string, '/') != (char *)NULL);
}

/* From bash-3.2 / stringlib.c / line 124 */
/* Cons a new string from STRING starting at START and ending at END,
   not including END. */
char *
substring (char const* string, int start, int end)
{
  register int len;
  register char *result;

  len = end - start;
  result = (char *)xmalloc (len + 1);
  strncpy (result, string + start, len);
  result[len] = '\0';
  return (result);
}

/* From bash-3.2 / general.c / line 644 ; changes: Return NULL instead of 'string' when string == 0. */
/* Given a string containing units of information separated by colons,
   return the next one pointed to by (P_INDEX), or NULL if there are no more.
   Advance (P_INDEX) to the character after the colon. */
char*
extract_colon_unit (char const* string, int* p_index)
{
  int i, start, len;
  char *value;

  if (string == 0)
    return NULL;

  len = strlen (string);
  if (*p_index >= len)
    return ((char *)NULL);

  i = *p_index;

  /* Each call to this routine leaves the index pointing at a colon if
     there is more to the path.  If I is > 0, then increment past the
     `:'.  If I is 0, then the path has a leading colon.  Trailing colons
     are handled OK by the `else' part of the if statement; an empty
     string is returned in that case. */
  if (i && string[i] == ':')
    i++;

  for (start = i; string[i] && string[i] != ':'; i++)
    ;

  *p_index = i;

  if (i == start)
    {
      if (string[i])
        (*p_index)++;
      /* Return "" in the case of a trailing `:'. */
      value = (char *)xmalloc (1);
      value[0] = '\0';
    }
  else
    value = substring (string, start, i);

  return (value);
}

/* From bash-2.05b / findcmd.c / line 242 */
/* Return the next element from PATH_LIST, a colon separated list of
   paths.  PATH_INDEX_POINTER is the address of an index into PATH_LIST;
   the index is modified by this function.
   Return the next element of PATH_LIST or NULL if there are no more. */
char*
get_next_path_element (char const* path_list, int* path_index_pointer)
{
  char* path;

  path = extract_colon_unit (path_list, path_index_pointer);

  if (path == 0)
    return (path);

  if (*path == '\0')
    {
      free (path);
      path = savestring (".");
    }

  return (path);
}

/* From bash-1.14.7 */
/* Turn PATH, a directory, and NAME, a filename, into a full pathname.
   This allocates new memory and returns it. */
char *
make_full_pathname (const char *path, const char *name, int name_len)
{
  char *full_path;
  int path_len;

  path_len = strlen (path);
  full_path = (char *) xmalloc (2 + path_len + name_len);
  strcpy (full_path, path);
  full_path[path_len] = '/';
  strcpy (full_path + path_len + 1, name);
  return (full_path);
}

/* From bash-3.2 */
static void
get_current_user_info ()
{
  struct passwd *entry;

  /* Don't fetch this more than once. */
  if (current_user.user_name == 0)
    {
      entry = getpwuid (current_user.uid);
      if (entry)
        {
          current_user.user_name = savestring (entry->pw_name);
          current_user.shell = (entry->pw_shell && entry->pw_shell[0])
                                ? savestring (entry->pw_shell)
                                : savestring ("/bin/sh");
          current_user.home_dir = savestring (entry->pw_dir);
        }
      else
        {
          current_user.user_name = "I have no name!";
          current_user.user_name = savestring (current_user.user_name);
          current_user.shell = savestring ("/bin/sh");
          current_user.home_dir = savestring ("/");
        }
      endpwent ();
    }
}

/* This is present for use by the tilde library. */
char* sh_get_env_value (const char* v)
{
  return getenv(v);
}

char* sh_get_home_dir(void)
{
  if (current_user.home_dir == NULL)
    get_current_user_info();
  return current_user.home_dir;
}

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL)
	{
		fprintf(stderr, "xmalloc: Out of memory");
		exit(-1);
	}
	return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
	if (!ptr)
		return xmalloc(size);
	ptr = realloc(ptr, size);
	if (size > 0 && ptr == NULL)
	{
		fprintf(stderr, "xmalloc: Out of memory");
		exit(-1);
	}
	return ptr;
}
