/*
	@(#)Tools/error.c
	@(#)Julian F. Reschke, 10. August 1990
*/


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void error (int status, int errnum, char *message, ...)
{
	va_list args;
	extern char *program_name;

	fprintf (stderr, "%s: ", program_name);
	va_start (args, message);
	vfprintf (stderr, message, args);
	va_end (args);

	if (errnum)
		fprintf (stderr, ": %s", strerror (errnum));
	putc ('\n', stderr);
	fflush (stderr);
	if (status)
		exit (status);
}

