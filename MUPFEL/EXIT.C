/*
 * exit.c - internal "exit" command
 * 25.09.90
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "alias.h"
#include "alloc.h"
#include "batch.h"
#include "chario.h"
#include "exit.h"
#include "curdir.h"
#include "comm.h"
#include "environ.h"
#include "errfile.h"
#include "fkey.h"
#include "gemsubs.h"
#include "hash.h"
#include "lineedit.h"
#include "mupfel.h"
#include "redir.h"
#include "shellvar.h"
#include "shrink.h"
#include "stand.h"
#include "sysvec.h"
#include "toserr.h"
#include "version.h"

SCCS(exit);

#define CHECKOPEN	1

static int lastmupfel;

#if CHECKOPEN
static void checkopen(int verbose)
{
	int i, wait = FALSE;
	long retcode;

	for (i=6; i<30; ++i)
	{
		retcode = Fseek(0L,i,1);
		if (verbose && retcode != EIHNDL)
		{
			dprint("\nexit: handle %d still open",i);
			wait = TRUE;
		}
			
	}
	if (wait)
	{
		dprint("\nCR to cont. ");
		inchar();
	}
}
#endif

void terminate(int verbose)
{
#if MERGED
	if (conwindow)	/* if console i/o still in window */
	{
		conexit();	/* restore i/o vectors */
		cursexit();
	}
#endif
	shellexit();
	lastmupfel = (getvarint("shellcount")==1);
	etvtermexit();
	fkeyexit();
	removecookie();
	shrinkexit();
	lineedexit();
	direxit();
	freevar();
	clearhash();
	aliasexit();
	exiterrfile();
#if CHECKOPEN
	checkopen(verbose);
#else
	++verbose;
#endif
	Cursconf(2,0);
}

void fatal(char *fmt,...)
{
	va_list argpoint;
	char tmp[200];
	
	va_start(argpoint,fmt);
	vsprintf(tmp,fmt,argpoint);
	va_end(argpoint);
	
	endredir(FALSE);

	mprintf("\nPanic: %s\n",tmp);
	inchar();
	crlf();

	terminate(FALSE);
	allocexit();
	exit(1);
}

/*
 * locate EXIT.PRG, put it's complete name into shcmd
 */
static void findexit(char *shcmd)
{
	char *ep;
	
	ep = getenv("EXIT");
	if (ep == NULL)
		ep = getenv("HOME");
		
	if (ep != NULL)
	{
		strcpy(shcmd,ep);
		chrapp(shcmd,'\\');
		strcat(shcmd,"EXIT.PRG");
	}
	if (ep==NULL || !access(shcmd,A_EXEC))
	{
		if ((ep=getenv("SHELL"))!=NULL)
			strcpy(shcmd,ep);
		else
			strcpy(shcmd,"EXIT.PRG");
	}
	strupr(shcmd);
}

/*
 * m_exit(ARGCV) - internal "exit" command.
 * If called from a batch file, just close that file.
 * If called from the commandline, terminate.
 * Merged version only:
 *   When argc>=1, just set the flag to go back to Venus.
 *   When argc==0, terminate (called from closebatch after venus()
 *	terminates). 
 */
int m_exit(ARGCV)
{
	char shcmd[100];
	COMMAND shtail = { 2, "-q" };
	int i, excode;

	if (execbatch())
	{
		closebatch();
		return 0;
	}

#if MERGED
	/*
	 * argc == 0 can't happen normally, but closebatch() calls
	 * m_exit that way.
	 */
	if (argc>0)
	{
		mprintf(ET_NOEXIT "\n");
		gotovenus = TRUE;
		return 0;
	}
#endif

	endredir(FALSE);
	excode = (argc>1) ? atoi(argv[1]) : getvarint("?");
	for (i=0; i<argc; ++i)
		free(argv[i]);
	terminate(TRUE);

	if (lastmupfel)
	{
		if (tosversion() < 0x104)
			findexit(shcmd);
	}
	else
		strcpy(shcmd,"");
		
	if (CommInfo.cmd != overlay)
		if (tosversion() < 0x104)
			shellwrite(1,1,1,shcmd,(char *)&shtail);
		else
			shellwrite(0,1,1,"",(char *)&shtail);

	exitgem();
	envexit();
	if (!alloccheck())
	{
		mprintf("\npress any key...");
		inchar();
	}
	allocexit();
	exit(excode);
	return 0;
}
