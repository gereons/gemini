/*
 * rm.c  -  internal "rm" and "rmdir" functions
 * 11.09.90
 */

#include <ctype.h> 
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "chmod.h"
#include "comm.h"
#include "getopt.h"
#include "mupfel.h"
#include "rm.h"
#include "tree.h"
#include "toserr.h"

SCCS(rm);

static int fail, verbose, bruteforce;

static void vcrlf(void)
{
	if (verbose)
		crlf();
}

int rmdir(char *dir)
{
	int retcode;
	
	strlwr(dir);
	if (lastchr(dir)=='\\' && strlen(dir)>1)
		dir[strlen(dir)-1] = '\0';
		
	if (verbose)
		mprintf(RM_SHOWRMD,dir);
	if (isdrive(dir))
	{
		vcrlf();
		oserr = EPTHNF;
		eprintf("rmdir: " RM_NODIR "\n",dir);
		return 1;
	}
	if (!strcmp(dir,".") || !strcmp(dir,".."))
	{
		vcrlf();
		oserr = EACCDN;
		eprintf("rmdir: " RM_ILLDIR "\n",dir);
		return 1;
	}
	
	switch (oserr=Ddelete(dir))
	{
		case EPTHNF:
			vcrlf();
			if (access(dir,A_EXIST))
				eprintf("rmdir: " RM_ISFILE "\n",dir);
			else
				eprintf("rmdir: " RM_NODIR "\n",dir);
			retcode = 1;
			break;
		case EACCDN:
			vcrlf();
			eprintf("rmdir: " RM_NOTEMPTY "\n",dir);
			retcode = 1;
			break;
		case EINTRN:
			vcrlf();
			eprintf("rmdir: " RM_INTRN "\n",dir);
			retcode = 1;
			break;
		default:
			if (bioserror(oserr))
			{
				vcrlf();
				retcode = ioerror("rmdir",dir,NULL,oserr);
			}
			else
			{
				CommInfo.dirty |= drvbit(dir);
				retcode = oserr = 0;
			}
			break;
	}
	if (verbose && !retcode)
		mprintf(RM_DONE "\n");
	return retcode;
}

int m_rmdir(ARGCV)
{
	int i;
	
	verbose = FALSE;
	
	if (argc==1)
		return printusage(NULL);

	for (i=1; i<argc; ++i)
		rmdir(argv[i]);
	return 0;
}

static char askrm(char *file)
{
	char answer;
	
	if (shellcmd)
		return 'Y';	
	mprintf(RM_CONFIRM,strlwr(file));
	answer = charselect(RM_CHOICE);
	if (answer == 'J')
		answer = 'Y';
	return answer;
}

int remove(const char *filename)
{
	char *file = strlwr(strdup(filename));
	int rmstat, retcode;
	
	if (verbose)
		mprintf(RM_SHOWRM,file);

	if (bruteforce && access(file,A_RDONLY))
		setwritemode(file,TRUE);
		
	rmstat = Fdelete(file);
	if (fail)
		switch (oserr=rmstat)
		{
			case EPTHNF:
			case EFILNF:
				vcrlf();
				if (isdir(file))
					eprintf("rm: " RM_ISDIR "\n",file);
				else
					eprintf("rm: " RM_NOFILE "\n",file);
				retcode = 1;
				break;
			case EACCDN:
				vcrlf();
				eprintf("rm: " RM_CANTRM "\n",file);
				retcode = 1;
				break;
			default:
				if (bioserror(rmstat))
				{
					vcrlf();
					retcode = ioerror("rm",file,NULL,rmstat);
				}
				else
				{
					CommInfo.dirty |= drvbit(file);
					retcode = 0;
				}
				break;
		}
	else
		retcode = 0;
	if (verbose && retcode==0)
		mprintf(RM_DONE "\n");
	free(file);
	return retcode;
}

static int rmall(char *path)
{
	return (isdir(path)) ? rmtree(path) : remove(path);
}

int m_rm(ARGCV)
{
	GETOPTINFO G;
	int c, retcode;
	static int interactive, recurflag;
	char ch;
	struct option long_option[] =
	{
		{ "recursive", FALSE, &recurflag, TRUE },
		{ "failure", FALSE, &fail, FALSE },
		{ "interactive", FALSE, &interactive, TRUE },
		{ "verbose", FALSE, &verbose, TRUE },
		{ "bruteforce", FALSE, &bruteforce, TRUE},
		{ NULL,0,0,0 },
	};
	int opt_index = 0;

	fail = TRUE;
	interactive = recurflag = FALSE;
	verbose = bruteforce = FALSE;
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "rfivb", long_option,
		&opt_index)) != EOF)	
		switch (c)
		{
			case 0:
				break;
			case 'i':
				interactive = TRUE;
				break;
			case 'r':
				recurflag = TRUE;
				interactive = FALSE;
				break;
			case 'f':
				fail = FALSE;
				break;
			case 'v':
				verbose = TRUE;
				break;
			case 'b':
				bruteforce = TRUE;
				break;
			default:
				return printusage(long_option);
		}

	if (G.optind == argc)
		return printusage(long_option);

	for (; G.optind<argc && !intr(); ++G.optind)
	{
		if (interactive)
		{
			ch = askrm(argv[G.optind]);
			switch (ch)
			{
				case 'Y':
				case 'A':
					if (recurflag)
						retcode = rmall(argv[G.optind]);
					else
						retcode = remove(argv[G.optind]);
					if (ch=='A')
						interactive = FALSE;
					break;
				case 'N':
					break;
				case 'Q':
					return 0;
			}
		}
		else
			if (recurflag)
				retcode = rmall(argv[G.optind]);
			else
				retcode = remove(argv[G.optind]);
	}
	return retcode;
}
