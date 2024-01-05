/*
	@(#)Mupfel/getopt.h
	@(#)Julian F. Reschke, 14. Februar 1991
	
	GETOPTINFO eingebaut
*/

/* declarations for getopt
   Copyright (C) 1989 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _M_GETOPT
#define _M_GETOPT

/* Describe the long-named options requested by the application.
   _GETOPT_LONG_OPTIONS is a vector of `struct option' terminated by an
   element containing a name which is zero.
   The field `has_arg' is:
   0 if the option does not take an argument,
   1 if the option requires an argument,
   2 if the option takes an optional argument.
   If the field `flag' is nonzero, it points to a variable that is set to
   the value given in the field `val' when the option is found, but
   left unchanged if the option is not found.  */

struct option
{
  char *name;
  int has_arg;
  int *flag;
  int val;
};

typedef struct
{

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

char *optarg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns EOF, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

int optind;

/* Callers store zero here to inhibit the error message `getopt' prints
   for unrecognized options.  */

int opterr;



const struct option *_getopt_long_options;

/* If nonzero, tell getopt that '-' means a long option.
   Set by getopt_long_only.  */
int _getopt_long_only;

/* The index in GETOPT_LONG_OPTIONS of the long-named option found.
   Only valid when a long-named option has been found by the most
   recent call to `getopt'.  */

int option_index;

/* The next char to be scanned in the option-element
   in which the last option character we returned was found.
   This allows us to pick up the scan where we left off.

   If this is zero, or a null string, it means resume the scan
   by advancing to the next ARGV-element.  */

char *nextchar;

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
   been skipped.  `first_nonopt' is the index in ARGV of the first of them;
   `last_nonopt' is the index after the last of them.  */

int first_nonopt;
int last_nonopt;

} GETOPTINFO;

int
getopt (GETOPTINFO *G, int argc, char **argv, const char *shortopts);

int
getopt_long (GETOPTINFO *G, int argc, char **argv, const char *shortopts,
		 const struct option *longopts, int *longind);
int
getopt_long_only (GETOPTINFO *G, int argc, char **argv, const char *shortopts,
		      const struct option *longopts, int *longind);

void
envopt (GETOPTINFO *G, int *pargc, char ***pargv, char *optstr);

void
optinit (GETOPTINFO *G);

#endif
