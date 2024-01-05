/*
 * commands.h  -  prototypes for internal commands
 * 23.01.91
 */

#ifndef _M_COMMANDS
#define _M_COMMANDS

#include "mupfel.h"

struct cmds
{
	char *name;			/* name of command */
	int	needexp;			/* need expanded wildcards? */
	int  (*func)(ARGCV);	/* function to call */
	char *usage;			/* arguments */
	char *expl;			/* explanation */
	char *unexp;			/* err message when unexpanded wildcards remain */
						/* when NULL, a standard message is printed */
};

extern struct cmds interncmd[];
extern long interncount;

#endif