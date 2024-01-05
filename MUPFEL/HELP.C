/*
 * help.c  -  intenal "help" command
 * 15.05.90
 */
 
#include <stdlib.h>
#include <string.h>

#include "chario.h"
#include "commands.h"
#include "environ.h"
#include "mupfel.h"
#include "parse.h"

SCCS(help);

/*
 * m_help(ARGCV)
 * internal "help" command
 * display list of internal commands or help line for each argument
 */
int m_help(ARGCV)
{
	int i,l;
	int maxcols, maxlen, scrncols, col = 0;
	char *ep;

	if (argc>1)
	{
		for (i=1; i<argc; ++i)
			if ((l=findintern(argv[i]))!=-1)
			{
				struct cmds *c = &interncmd[l];
				mprintf("%s %s - %s\n",c->name,c->usage,c->expl);
			}
			else
				eprintf("%s: " HE_NOCMD "\n",argv[i]);
		return 0;
	}
	
	if ((ep=getenv("COLUMNS"))!=NULL)
		scrncols = atoi(ep);
	else
		scrncols = 80;

	maxlen=0;
	for (i=0; i<interncount; ++i)
	{
		l=(int)strlen(interncmd[i].name);
		if (l>maxlen)
			maxlen=l;
	}
	if (maxlen > scrncols)
		scrncols = maxlen+2;
		
	maxcols = scrncols / (maxlen+=2);
	for (i=0; i<interncount && !intr(); ++i)
	{
		l = mprintf(interncmd[i].name);
		if (++col % maxcols == 0)
		{
			col=0;
			crlf();
		}
		else
			rawoutn(' ',maxlen-l);
	}
	if (col % maxcols != 0)
		crlf();
	return 0;
}
