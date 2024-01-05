/*
 * cmdline.c  -  get a new command line
 * 11.03.90
 */

#include <string.h>

#include "alloc.h"
#include "batch.h"
#include "chario.h"
#include "cmdline.h"
#include "lineedit.h"
#include "mupfel.h"

#define LL_ENTRY cmdline
#include "linklist.h";

SCCS(cmdline);

static int cmdlineargs;

/*
 * void pushline(char *line)
 * push one line on current linesource stack
 */
void pushline(char *line)
{
	cmdline cline;
	
	cline.line = strdup(line);
	insert(&cline,&(curline->linestack),sizeof(cmdline));
	++curline->pushedline;
}

/*
 * char *popline(void)
 * return last pushed line from current linesource stack
 */
static char *popline(void)
{
	static char line[LINSIZ];
	cmdline *l = curline->linestack;
	
	strcpy(line,l->line);
	curline->linestack = l->next;
	free(l->line);
	free(l);
	--curline->pushedline;
	return line;
}

/*
 * char *batchline(void)
 * get next line from current batchfile
 * if eof, call closebatch(), return empty line
 */
static char *batchline(void)
{
	int l;

	if (!wasintr() && sgets(lbuf,LINESIZE,&(curline->batch))!=NULL)
	{
		l=(int)strlen(lbuf)-1;
		if (lbuf[l]=='\n')
			lbuf[l]='\0';
		return lbuf;
	}
	else
	{
		closebatch();
		return NULL;
	}
}

/*
 * char *getline(void)
 * if there's something on the line stack, get that.
 * if not batch, get line from console.
 * if batch, read line from current batchfile.
 */
char *getline(int gemini)
{
	char *line;
	static int wascmdline = FALSE;
	
	if (cmdlineargs)	/* Mupfel started with "-c cmd" */
	{
		cmdlineargs = FALSE;
		wascmdline = TRUE;
		return lbuf;
	}

	while (execbatch())
	{
		line = curline->pushedline ? popline() : batchline();
		if (line != NULL)	/* line==NULL: batchfile was closed */
			return line;
		if ((shellcmd && !execbatch()) || gemini)
			return "";
	}
	if (wascmdline)
		return "";
		
	return curline->pushedline ? popline() : readline(TRUE);
}

void setcmdline(ARGCV)
{
	int i;
	
	cmdlineargs = TRUE;
	
	*lbuf = '\0';
	for (i=2; i<argc; ++i)
	{
		strcat(lbuf,argv[i]);
		if (i<argc-1)
			chrcat(lbuf,' ');
	}
}
