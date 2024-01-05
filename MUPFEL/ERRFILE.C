/*
 * errfile.c  -  errorfile management
 * 01.10.90
 */
 
#include <tos.h>
#include <stdio.h>

#include "alloc.h"
#include "chario.h"
#include "comm.h"
#include "environ.h"
#include "errfile.h"
#include "getopt.h"
#include "handle.h"
#include "mupfel.h"
#include "toserr.h"

SCCS(errfile);

#define DEBUG	0
#define STDAUX 2

static int ehnd = ILLHND;
static int append;
static int orghnd;
static char *efilename = NULL;

void closeerrfile(void)
{
	if (ehnd >= MINHND)
	{
#if DEBUG
		dprint("closing ehnd %d\n",ehnd);
#endif
		Fforce(STDAUX,orghnd);
		Fclose(ehnd);
		Fclose(ehnd);
		Fclose(orghnd);
	}
}

static int openerrfile(const char *file)
{
	if (efilename)
		free(efilename);
	efilename = strdup(file);
	closeerrfile();
	
	if (isdevice(file))
	{
		ehnd = Fopen(file,FO_RW);
		return 0;
	}
	if (!append)
		Fdelete(file);
	
	if (access(file,A_WRITE))
	{
		if (append)
		{
			ehnd = Fopen(file,O_RDWR);
			Fseek(0L,ehnd,SEEK_END);
		}
		else
			if ((ehnd=Fcreate(file,0))<MINHND)
			{
				mprintf("errorfile: " EF_CANTCRT "\n",file);
				return 1;
			}
	}
	else
	{
		if (access(file,A_EXIST))
		{
			mprintf("errorfile: " EF_ISRO "\n",file);
			return 1;
		}
		if ((ehnd=Fcreate(file,0))<MINHND)
		{
			mprintf("%s: " EF_CANTCRT "\n",file);
			return 1;
		}
	}
	
	CommInfo.dirty |= drvbit(file);
	return 0;
}

static int seterrorfile(const char *file)
{
	int retcode;
	char tmp[200];
	
	if (openerrfile(file))
		return 1;
#if DEBUG
	dprint("new errfile %s, handle %d\n",file,ehnd);
#endif
	orghnd = Fopen("AUX:",O_RDWR);
	retcode = Fforce(STDAUX,ehnd)==EIHNDL;
	if (!retcode)
	{
		sprintf(tmp,"STDERR=%s",file);
		putenv(tmp);
	}
	return retcode;
}

int m_errorfile(ARGCV)
{
	GETOPTINFO G;
	int c;
	static int print;
	struct option long_option[] =
	{
		{ "append", FALSE, &append, TRUE},
		{ "print", FALSE, &print, TRUE},
		{ NULL,0,0,0},
	};
	int opt_index = 0;
	
	append = print = FALSE;

	optinit (&G);

	while ((c = getopt_long(&G, argc, argv, "ap", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 'a':
				append = TRUE;
				break;
			case 'p':
				print = TRUE;
				break;
			default:
				return printusage(long_option);
		}
	if (print)
	{
		if (efilename)
			mprintf("errorfile: " EF_STATUS "\n",efilename);
		else
			mprintf("errorfile: " EF_NOFILE "\n");
		return 0;
	}
	if (argc - G.optind != 1)
		return printusage(long_option);
	else
		return seterrorfile (argv[G.optind]);
}

void initerrfile(void)
{
	seterrorfile("CON:");
}

void exiterrfile(void)
{
	closeerrfile();
	if (efilename)
		free(efilename);
}

void restoreerrfile(void)
{
	seterrorfile(efilename);
}
