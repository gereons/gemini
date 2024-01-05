/*
 * batch.c  -  batch file management
 * 25.09.90
 */

#include <stdio.h> 
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "batch.h"
#include "chario.h"
#include "curdir.h"
#include "filebuf.h"
#include "gemsubs.h"
#include "handle.h"
#include "mupfel.h"
#include "shellvar.h"
#include "stand.h"
#include "toserr.h"

SCCS(batch);

#define DEBUG 0
#if DEBUG
#include "alert.h"
#endif

static linesrc terminal;

int isbatchcmd;
linesrc *curline = &terminal;

#if MERGED
void m_exit(ARGCV);
int venus(void);
#endif

int execbatch(void)
{
	return curline != &terminal;
}

int isautoexec(void)
{
	return curline->batch.autoexec;
}

char *sgets(char *s, int n, batch *b)
{
	char ch, *sp = s;
	int cnt = 0;
	
	while ((ch=*b->bufp)!='\0' && cnt<n-1)
	{
		*sp = ch;
		++b->bufp;
		if (*sp == '\r')
		{
			++b->bufp;
			if (sp!=s)
			{
				*sp = '\n';
				*(sp+1) = '\0';
				return s;
			}
		}
		else
		{
			++sp;
			++cnt;
		}
	}
	if (ch=='\0')
		*sp = '\0';
	return (sp==s) ? NULL : s;
}

static int batchbuf(int handle, batch *b, char *file)
{
	if (filebuf(handle,file,&b->buffer,"batch") == BUF_ERR)
		return FALSE;
	else
	{
		b->bufp = b->buffer;
		b->argv = malloc(MAXARGC*sizeof(char *));
		b->autoexec = FALSE;
		return TRUE;
	}
}

static void addlinesrc(void)
{
#if DEBUG
	alert(1,1,1,"addlinesrc dir=%c:%s","OK",getdrv(),getdir());
#endif
	curline->next = malloc(sizeof(linesrc));
	curline->next->prev = curline;
	curline->batch.dir = strdup(getdir());
	curline->batch.drv = getdrv();
	curline = curline->next;
	
	curline->pushedline = 0;
	curline->linestack = NULL;
}

static void deletelinesrc(void)
{
	linesrc *cl = curline;
	
	curline = curline->prev;
	curline->next = NULL;
#if DEBUG
	alert(1,1,1,"deletelinesrc dir=%c:%s","OK",curline->batch.drv,curline->batch.dir);
#endif
	chdrv(curline->batch.drv);
	chdir(curline->batch.dir);
	free(curline->batch.dir);
	free(cl);
}

static void batcherror(char *filename)
{
	deletelinesrc();
	oserr = READ_FAULT;
	eprintf("Mupfel: " BT_ERROR "\n",filename);
}

/*
 * openbatch(char *cmdpath, int setbatch)
 * try to open batchfile, name is in cmdpath
 * if setbatch==TRUE, the file pointer is stored in the batchfile
 * array
 * return code is TRUE or FALSE, depending on success of fopen()
 */
int openbatch(char *cmdpath, int setbatch)
{
	int handle;

	if ((handle=Fopen(cmdpath,O_RDONLY))<MINHND)
		return FALSE;

	if (!setbatch)
	{
		Fclose(handle);
		return TRUE;
	}

	isbatchcmd = TRUE;

	addlinesrc();
	
	if (!batchbuf(handle,&(curline->batch),cmdpath))
	{
		Fclose(handle);
		batcherror(cmdpath);
		return FALSE;
	}
	curline->batch.in.file = NULL;
	curline->batch.out.file = NULL;
	curline->batch.aux.file = NULL;
	return TRUE;
}

static void endautoexec(void)
{
	lateinit();
#if MERGED
	venus();			/* This is main() in Venus */
	m_exit(0,NULL);	/* The only way to exit is from Venus */
#endif
}

/*
 * closebatch(void)
 * close current batchfile, decrement batchfile index
 * reset values of $#, $0..$9
 * In the merged version, call Venus' main routine when the autoexec
 * file is done.
 */
void closebatch(void)
{
	int i, autoexec;
	char name[2] = "?";
	
	for (i=0; i<curline->batch.argc; ++i)
		free(curline->batch.argv[i]);
	if (curline->batch.argv != NULL)
		free(curline->batch.argv);
	Mfree(curline->batch.buffer);
	if (curline->batch.in.file != NULL)
		free(curline->batch.in.file);
	if (curline->batch.out.file != NULL)
		free(curline->batch.out.file);
	if (curline->batch.aux.file != NULL)
		free(curline->batch.aux.file);

	autoexec = curline->batch.autoexec;

	deletelinesrc();
	if (curline == &terminal)
	{
		setvar("#",NULL);
		for (i=0; i<10; ++i)
		{
			*name = i+'0';
			setvar(name,NULL);
		}
	}
	else
		batchargs(curline->batch.argc,curline->batch.argv,FALSE);

	if (autoexec)
		endautoexec();
}

void setparms(ARGCV)
{
	int i;
	char name[2];
	
	strcpy(name,"X");
	for (i=0; i<10; ++i)
	{
		*name = i+'0';
		if (i<argc)
			setvar(name,argv[i]);
		else
			setvar(name,NULL);
	}
}

static void redircopy(redirinfo *to, redirinfo *from)
{
	to->file = from->file==NULL ? NULL : strdup(from->file);
	to->clobber = from->clobber;
	to->append = TRUE;
}

/*
 * batchargs(ARGCV,int copy)
 * set values of $# and $0..$9 to argc, argv[0..9]
 * if copy==TRUE, copy argc/argv on batchfile stack
 */
void batchargs(ARGCV,int copy)
{
	int i;
	
	if (copy)
	{
		curline->batch.argc = argc;
		for (i=0; i<argc; ++i)
			curline->batch.argv[i] = strdup(argv[i]);
		redircopy(&curline->batch.in,&redirect.in);
		redircopy(&curline->batch.out,&redirect.out);
		redircopy(&curline->batch.aux,&redirect.aux);
	}
	setvar("#",itos(argc-1));
	setparms(argc,argv);
}

void autoexec(char *filename)
{
	int handle;
	char file[128];
	
	strcpy(file,filename);

	if (getvarint("shellcount")>1)
	{
		endautoexec();
		return;
	}
	
	if (!shellfind(file))
	{
		strcpy(file,filename);
		handle = ILLHND;
	}
	else
		handle = Fopen(file,O_RDONLY);
		
	if (handle >= MINHND)
	{
		addlinesrc();
		if (batchbuf(handle,&(curline->batch),file))
		{
			curline->batch.argc = 0;
			curline->batch.autoexec = TRUE;
			curline->batch.in.file = NULL;
			curline->batch.out.file = NULL;
			curline->batch.aux.file = NULL;
		}
		else
		{
			Fclose(handle);
			batcherror(file);
		}
	}
	else
	{
#if STANDALONE
		mprintf(BT_STARTUP "\n",file);
#endif
		endautoexec();
	}
}
