/*
 * rmmkdir.c  -  internal "rmdir" and "mkdir" functions
 * 20.02.90
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
#include "rmmkdir.h"
#include "tree.h"
#include "toserr.h"

SCCS(rmmkdir);

static int fail, verbose, bruteforce;

static void vcrlf(void)
{
	if (verbose)
		crlf();
}

int mkdir(char *dir)
{
	if (access(dir,A_READ) || isdir(dir))
	{
		oserr=EACCDN;
		mprint("mkdir: %s already exists\n",strlwr(dir));
		return FALSE;
	}
	if (bioserr)
		return ioerror("mkdir",dir,NULL,bioserr);
		
	if (Dcreate(dir)<0)
	{
		oserr=EACCDN;
		mprint("mkdir: %s: can't create\n",strlwr(dir));
		return FALSE;
	}
	else
	{
		CommInfo.dirty |= drvbit(dir);
		return TRUE;
	}
}

int m_mkdir(ARGCV)
{
	int i;
	
	if (argc==1)
		return printusage();

	for (i=1; i<argc; ++i)
		if (!mkdir(argv[i]))
			return 1;
	return 0;
}

int rmdir(char *dir)
{
	int retcode;
	
	if (verbose)
		mprint("removing dir %s ...",strlwr(dir));
	switch (oserr=Ddelete(dir))
	{
		case EPTHNF:
			vcrlf();
			mprint("rmdir: %s: no such directory\n",
				strlwr(dir));
			retcode = 1;
			break;
		case EACCDN:
			vcrlf();
			mprint("rmdir: %s: not empty\n",strlwr(dir));
			retcode = 1;
			break;
		case EINTRN:
			vcrlf();
			mprint("rmdir: %s: GEMDOS internal error -65\n",strlwr(dir));
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
		mprint(" done.\n");
	return retcode;
}

int m_rmdir(ARGCV)
{
	int i;
	
	verbose = FALSE;
	
	if (argc==1)
		return printusage();

	for (i=1; i<argc; ++i)
		rmdir(argv[i]);
	return 0;
}

static char askrm(char *file)
{
	if (shellcmd)
		return 'Y';	
	mprint("rm %s (y/n/a/q)? ",strlwr(file));
	return charselect("YNAQ");
}

int remove(const char *filename)
{
	char *file = strlwr(strdup(filename));
	int rmstat, retcode;
	
	if (verbose)
		mprint("removing %s ...",file);

	if (bruteforce && access(file,A_RDONLY))
		setwritemode(file,TRUE);
		
	rmstat = Fdelete(file);
	if (fail)
		switch (oserr=rmstat)
		{
			case EFILNF:
				vcrlf();
				if (isdir(file))
					mprint("rm: can't remove directory %s\n",file);
				else
					mprint("rm: %s: no such file\n",file);
				retcode = 1;
				break;
			case EACCDN:
				vcrlf();
				mprint("rm: %s: can't remove (read-only?)\n",file);
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
		mprint(" done.\n");
	free(file);
	return retcode;
}

static int rmall(char *path)
{
	return (isdir(path)) ? rmtree(path) : remove(path);
}

int m_rm(ARGCV)
{
	int c, retcode, interactive, recurflag;
	char ch;

	fail = TRUE;
	interactive = recurflag = FALSE;
	verbose = bruteforce = FALSE;
	optinit();
	while ((c=getopt(argc,argv,"rfivb"))!=EOF)	
		switch (c)
		{
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
				return printusage();
		}
	if (optind==argc)
		return printusage();
	for (; optind<argc && !intr(); ++optind)
	{
		if (interactive)
		{
			ch = askrm(argv[optind]);
			switch (ch)
			{
				case 'Y':
				case 'A':
					if (recurflag)
						retcode = rmall(argv[optind]);
					else
						retcode = remove(argv[optind]);
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
				retcode = rmall(argv[optind]);
			else
				retcode = remove(argv[optind]);
	}
	return retcode;
}
