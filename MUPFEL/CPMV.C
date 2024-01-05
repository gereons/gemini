/*
 * cpmv.c  -  cp, mv, backup and rename commands
 * 20.02.91
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "attrib.h"
#include "chario.h"
#include "chmod.h"
#include "comm.h"
#include "cpmv.h"
#include "curdir.h"
#include "getopt.h"
#include "handle.h"
#include "mupfel.h"
#include "shellvar.h"
#include "strsel.h"
#include "toserr.h"
#include "tree.h"
#include "version.h"

SCCS(cpmv);

static char *extreplace(char *filename,const char *ext);

#define H_DEBUG	0
#define NUMHANDLES	30
static int handle[NUMHANDLES];
static int acthandle = -1;

static int iscp;
static int archive, confirm, datekeep, recursive, verbose;
static int interactive, securecopy;
static int bruteforce, nonexist, keepopen;
static DTA *cpdta;		/* dta for current file; for cp -r */

/* used for cpfile in tree.c */
void setcpdta(DTA *d)
{
	cpdta = d;
}

/* Used for cpdir in tree.c */
int cpverbose(void)
{
	return verbose;
}

static void cpoptinit(void)
{
	confirm = datekeep = recursive = verbose = 
	archive = interactive = securecopy = bruteforce =
	nonexist = keepopen = FALSE;
}

static int cpmvusage(struct option *long_option)
{
	char *cmd = iscp ? "cp" : "mv";
	char *opt = iscp ? "[-cdrviasbnM]" : "[-cvib]";
	char *opt2 = iscp ? "[-rviasbnM]" : "[-cv]";
             	
	eprintf(CP_USAGE  ": %s %s file1 file2\n",cmd,opt);
	eprintf(CP_INDENT "  %s %s files... dir\n",cmd,opt);
	eprintf(CP_INDENT "  %s %s dir1 dir2\n",cmd,opt2);
	print_longoption(opt,long_option);
	return 1;
}

/*
 * ask for confirmation if destination file alread exists
 */
static char confcpmv(const char *cmd, char *file)
{
	char answer;
	
	if (shellcmd)
		return 'Y';
	mprintf(CP_CONFCP,cmd,strlwr(file));	
	answer = charselect(CP_CHOICE);
	if (answer=='J')
		answer = 'Y';
	return answer;
}

/*
 * ask for confirmation to copy file
 */
static char askcpmv(const char *cmd,char *from,char *to)
{
	char answer;
	
	if (shellcmd)
		return 'Y';
	mprintf(CP_ASKCP,cmd,strlwr(from),strlwr(to));
	answer = charselect(CP_CHOICE);
	if (answer=='J')
		answer = 'Y';
	return answer;
}

/*
 * show what we're doing
 */
static void cpmvshow(char *from, char *to)
{
	mprintf(CP_SHOWCP,iscp ? CP_COPYING : CP_MOVING,
		strlwr(from),strlwr(to));
}

/*
 * copy modification date & time
 */
static void cpdate(const char *from, const char *to)
{
	int fromhnd, tohnd;
	DOSTIME dtime;
	
	fromhnd = Fopen(from,0);
	tohnd = Fopen(to,2);
	Fdatime(&dtime,fromhnd,GETDATE);
	Fdatime(&dtime,tohnd,SETDATE);
	Fclose(fromhnd);
	Fclose(tohnd);
}

/*
 * copy file attributes
 */
static void cpattrib(const char *src,const char *dest)
{
	int attrib;
	
	attrib = Fattrib(src,GETATTRIB,0);
	Fattrib(dest,SETATTRIB,attrib);
}

/*
 * check if the target disk is full
 */
static int checkdiskfull(const char *file)
{
	DISKINFO dspace;
	ulong clustsize, freespace;
	char drv;
	
	if (file[1]==':')
		drv = toupper(file[0]);
	else
		drv = getdrv();
		
	Dfree(&dspace,drv-'A'+1);
	clustsize = dspace.b_secsiz * dspace.b_clsiz;
	freespace = dspace.b_free * clustsize / 1024L;
	if (freespace == 0L)
		return TRUE;
	else
		return FALSE;
}

/*
 * after a secure copy (cp -s), rename the temp file to the
 * original destination
 */
static int endsecurecopy(char *to,char *origto)
{
	int retcode = 0;
	int err;
	
	if (verbose)
		mprintf(CP_ENDSEC,strlwr(to),strlwr(origto));

	if ((err=Fdelete(origto))==0 || err==EFILNF)
		Frename(0,to,origto);
	else
	{
		if (verbose)
			crlf();
		mprintf("cp: " CP_OVERWRT "\n",strlwr(origto));
		Fdelete(to);
		retcode = 1;
	}
	return retcode;
}

static void closehandles(void)
{
	int i;
	
	if (keepopen)
	{
		for (i=0; i<=acthandle; ++i)
		{
	#if H_DEBUG
			dprint("cp: closing handle %d (%d)\n",handle[i],i);
	#endif
			if (handle[i] < MINHND)
				eprintf("cp: strange handle[%d] = %d!\n",i,handle[i]);
			else
				Fclose(handle[i]);
			handle[i] = ILLHND;
		}
		acthandle = -1;
	}
}

static int newhandle(const char *file)
{
	int hnd;

	if (keepopen)
	{	
		if (acthandle == NUMHANDLES-1)
			closehandles();
		if ((hnd = Fcreate(file,0))==ENHNDL)
		{
	#if H_DEBUG
			dprint("cp: no more handles\n");
	#endif
			closehandles();
			hnd = Fcreate(file,0);
		}
	#if H_DEBUG
		dprint("cp: got handle %d (%d)\n",hnd,acthandle);
	#endif
		if (!bioserror(hnd) && !toserror(hnd))
			handle[++acthandle] = hnd;
	}
	else
		hnd = Fcreate(file,0);
	return hnd;
}

/*
 * do the basic work of copying the file
 */
static int docp(int inhnd, int outhnd, long bufsize, char *buffer,
	char *to, char *from)
{
	long rd, wr;
	int retcode;
	
	retcode = 0;
	while ((rd=Fread(inhnd,bufsize,buffer))>0)
	{
		wr=Fwrite(outhnd,rd,buffer);
		if (wr!=rd)
		{
			oserr=WRITE_FAULT;
			if (verbose)
				crlf();
			if (checkdiskfull(to))
				eprintf("cp: " CP_FULLDISK "\n");
			else
				eprintf("cp: " CP_WRITEERR "\n",strlwr(to));
			retcode = 1;
			break;
		}
	}
	if (rd < 0)
	{
		oserr=READ_FAULT;
		if (verbose)
			crlf();
		eprintf("cp: " CP_READERR "\n",strlwr(from));
		retcode = 1;
	}
	Fclose(inhnd);
	if (!keepopen)
		Fclose(outhnd);

	return retcode;
}

int cp(char *from,char *to)
{
	char *buffer, *origto = NULL, choice;
	int inhnd, outhnd;
	int retcode;
	long bufsize, keepfree;
	int srcattrib;

	retcode = 0;
	if (!strcmp(from,to))
	{
		oserr = EACCDN;
		eprintf("cp: " CP_DUPFILE "\n",strlwr(from));
		return 1;
	}

	if (interactive)
	{
		choice=askcpmv("cp",from,to);
		switch (choice)
		{
			case 'N':
			case 'Q':
				return (choice=='N') ? 0 : -1;
			case 'A':
				interactive = FALSE;
				break;
		}
	}

	/* allocate space for the copy buffer */	
	keepfree = (long)getvarint("keepfree");
	if (keepfree == 0L)
		keepfree = 8192L;

	bufsize = (long)Malloc(-1L);
	if (bufsize > keepfree)
		bufsize -= keepfree;
	while ((buffer=Malloc(bufsize))==NULL)
	{
		bufsize /= 2;
		if (bufsize<1024)
		{
			oserr=ENSMEM;
			eprintf("cp: " CP_NOMEM "\n");
			return 1;
		}
	}
	
	if ((inhnd=Fopen(from,O_RDONLY))>=MINHND)
	{
		int destexists = TRUE;
		int filechanged = FALSE;
		
		if (confirm || nonexist)
			destexists = access(to,A_EXIST);

		if (archive)
		{
			/* do nothing if archive bit is not set */
			if (cpdta != NULL)
				srcattrib = cpdta->d_attrib;
			else
				srcattrib = Fattrib(from,GETATTRIB,0);
			filechanged = (srcattrib & FA_ARCH);
			if (!filechanged && destexists)
			{
				/* file hasn't changed and target exists */
				Fclose(inhnd);
				Mfree(buffer);
				return 0;
			}
		}

		/* confirm copy if destination exists */
		if (confirm && destexists)
		{
			choice=confcpmv("cp",to);
			switch (choice)
			{
				case 'N':
				case 'Q':
					Fclose(inhnd);
					Mfree(buffer);
					return (choice=='N') ? 0 : -1;
				case 'A':
					confirm = FALSE;
					break;
			}
		}

		if (nonexist && destexists && !filechanged)
		{
			Fclose(inhnd);
			Mfree(buffer);
			return 0;
		}
		
		/* allow overwriting of read-only files */
		if (bruteforce && access(to,A_RDONLY))
			setwritemode(to,TRUE);
			
		/* build a temp filename for secure copy */
		if (securecopy)
		{
			origto = to;
			to = extreplace(to,"$$$");
		}

		if ((outhnd=newhandle(to))>=MINHND)
		{
			if (verbose)
				cpmvshow(from,to);
			
			/* at last. copy the file */	
			retcode = docp(inhnd,outhnd,bufsize,buffer,to,from);
			
			if (datekeep && retcode==0)
				cpdate(from,to);

			/* delete the target if copy failed */
			if (retcode != 0)
				Fdelete(to);
				
			if (retcode==0 && securecopy)
				retcode = endsecurecopy(to,origto);
			if (retcode==0)
			{
				if (archive)
					Fattrib(from,SETATTRIB,srcattrib & ~FA_ARCH);
				/* 
				 * Fattrib won't work on just-created Files, so with -M
				 * copying of attributes is impossible
				 */
				if (!keepopen)
					cpattrib(from,(securecopy) ? origto : to);
			}
		}
		else
		{
			/* couldn't create destination file */
			Fclose(inhnd);
			oserr=EACCDN;
			if (verbose)
				crlf();
			eprintf("cp: " CP_CANTCRT "\n",strlwr(to));
			retcode=1;
		}
	}
	else
	{
		/* couldn't open source file */
		oserr=EFILNF;
		if (isdir(from))
			eprintf("cp: " CP_CPDIR "\n",strlwr(from));
		else
			eprintf("cp: " CP_CANTOPEN "\n",strlwr(from));
		retcode = 1;
	}

	/* clean up */
	if (securecopy && origto)
		free(to);
	Mfree(buffer);
	if (verbose && retcode==0)
		mprintf(CP_CPDONE "\n");
	if (retcode == 0)
	{
		CommInfo.dirty |= drvbit(from);
		CommInfo.dirty |= drvbit(securecopy ? origto : to);
	}
	return retcode;
}
	
static int mv(char *from,char *to)
{
	int retcode, err, wasrdonly = FALSE;
	char todrv, fromdrv, choice;
	
	retcode = 0;
	if (!strcmp(from,to))
	{
		oserr = EACCDN;
		eprintf("mv: " CP_DUPFILE "\n",strlwr(from));
		return 1;
	}

	/* ask for confirmation */
	if (interactive)
	{
		choice=askcpmv("mv",from,to);
		switch (choice)
		{
			case 'N':
			case 'Q':
				return (choice=='N') ? 0 : -1;
			case 'A':
				interactive = FALSE;
				break;
		}
	}
	
	/* check for source file */
	if (!access(from,A_READ))
	{
		oserr = EFILNF;
		eprintf("mv: " CP_NOFILE "\n",strlwr(from));
		return 1;
	}
	if (bioserr)
		return ioerror("mv",from,NULL,bioserr);

	/* allow renaming of read-only files */
	if (bruteforce && access(from,A_RDONLY))
	{
		setwritemode(from,TRUE);
		/* remember that the file was read-only */
		wasrdonly = TRUE;
	}
				
	if (!access(from,A_RDWR))
	{
		oserr = EACCDN;
		eprintf("mv: " CP_READONLY "\n",strlwr(from));
		return 1;
	}
	if (bioserr)
		return ioerror("mv",from,NULL,bioserr);

	/* confirm renaming the file */
	bioserr = 0;	
	if (confirm && access(to,A_EXIST))
	{
		switch (confcpmv("mv",to))
		{
			case 'N':
				return 0;
			case 'Q':
				return -1;
			case 'A':
				confirm = FALSE;
				break;
		}
	}
	if (bioserr)
		return ioerror("mv",to,NULL,bioserr);
	
	/* allow overwriting the target file */
	if (bruteforce && access(to,A_RDONLY))
		setwritemode(to,TRUE);
	
	/* try to delete the target file */		
	err = Fdelete(to);
	if (bioserror(err))
		return ioerror("mv",to,NULL,err);
		
	/* determine the drives for source and target files */
	fromdrv = (from[1]==':') ? toupper(*from) : getdrv();
	todrv = (to[1]==':') ? toupper(*to) : getdrv();
	if (fromdrv==todrv)
	{
		/* on the same drive, a simple rename does the job */
		if (verbose)
			cpmvshow(from,to);
		if (Frename(0,from,to)<0)
		{
			oserr=EACCDN;
			if (verbose)
				crlf();
			eprintf("mv: " CP_RENAME "\n",strlwr(from),strlwr(to));
			retcode = 1;
		}
		CommInfo.dirty |= drvbit(from);
		if (verbose && retcode==0)
			mprintf(CP_CPDONE "\n");
	}
	else
	{
		/* for different drives, copy the file */
		if (cp(from,to)==0)
		{
			/* it worked, remove the source */
			Fdelete(from);
			CommInfo.dirty |= drvbit(from) | drvbit(to);
		}
		else
			retcode = 1;
	}

	/* did we move a read-only file? */
	if (wasrdonly && retcode==0)
		/* reset the attribute */
		setwritemode(to,FALSE);
		
	return retcode;
}

/*
 * copy/move file "src" into "dir"
 */
static int cpinto(char *src,const char *dir,int (*filecopy)(char *src,char *to))
{
	/* new target pathname */
	char *dest = malloc(strlen(src)+strlen(dir)+2);
	int retcode;

	strcpy(dest,dir);
	/*
	 * if dir is a drive name, don't append a backslash so the target
	 * will be in the current dir of that drive
	 */
	if (!isdrive(dest))
		chrapp(dest,'\\');
	strcat(dest,strfnm(src));
	retcode = filecopy(src,dest);
	free(dest);
	return retcode;
}

static int cpmvdir(char *src, char *dest)
{
	if (iscp)
	{
		/* silly if not recursive copy */
		if (!recursive)
		{
			oserr=EACCDN;
			eprintf("cp: " CP_RECUR "\n");
			return 1;
		}
		else
			return cptree(src,dest);
	}
	else
	{
		/* it's a directory rename */
		if (tosversion() < 0x104)
		{
			eprintf("mv: " CP_NEED14 "\n");
			return 1;
		}
		else
		{
			int err;
			
			err = Frename(0,src,dest);
			if (bioserror(err))
				return ioerror("mv",src,NULL,err);
			if (err==0)
				CommInfo.dirty |= drvbit(src);
			else
				eprintf("mv: " CP_RENAME "\n",src,dest);
			return err==0 ? 0 : 1;
		}
	}
}

static int docpmv(ARGCV,	int (*filecopy)(char *src,char *to))
{
	GETOPTINFO G;
	char *cmd = iscp ? "cp" : "mv";
	int i, c;
	int firstisdir, lastisdir, argsleft, lastarg;
	struct option cp_long_option[] = 
	{
		{ "confirm", FALSE, &confirm, TRUE},
		{ "datekeep", FALSE, &datekeep, TRUE},
		{ "recursive", FALSE, &recursive, TRUE},
		{ "verbose", FALSE, &verbose, TRUE},
		{ "interactive", FALSE, &interactive, TRUE},
		{ "archive", FALSE, &archive, TRUE},
		{ "secure", FALSE, &securecopy, TRUE},
		{ "bruteforce", FALSE, &bruteforce, TRUE},
		{ "nonexistent", FALSE, &nonexist, TRUE},
		{ "multifile", FALSE, &keepopen, TRUE},
		{ NULL,0,0,0},
	};
	struct option mv_long_option[] =
	{
		{ "confirm", FALSE, &confirm, TRUE},
		{ "verbose", FALSE, &verbose, TRUE},
		{ "interactive", FALSE, &interactive, TRUE},
		{ "bruteforce", FALSE, &bruteforce, TRUE},
		{ NULL,0,0,0},
	};
	struct option *long_option;
	int opt_index = 0;
	
	cpoptinit();
	cpdta = NULL;

	optinit (&G);

	long_option = iscp ? cp_long_option : mv_long_option;
	while ((c=getopt_long(&G,argc,argv,iscp ? "cdrviasbnM" : "cvib",
		long_option,&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
			case 'c':
				confirm=TRUE;
				break;
			case 'd':
				datekeep=TRUE;
				break;
			case 'r':
				recursive = TRUE;
				break;
			case 'v':
				verbose = TRUE;
				break;
			case 'i':
				interactive = TRUE;
				break;
			case 'a':
				archive = TRUE;
				break;
			case 's':
				securecopy = TRUE;
				break;
			case 'b':
				bruteforce = TRUE;
				break;
			case 'n':
				nonexist = TRUE;
				break;
			case 'M':
				keepopen = TRUE;
				break;
			default:
				return cpmvusage(long_option);
		}
	/* mv always keeps the date */
	if (!iscp)
		datekeep=TRUE;
	/* less than two args left? */
	if (argc-G.optind<2)
		return cpmvusage(long_option);

	if (archive && tosversion() < 0x104)
		eprintf("cp: " CP_ARCHWARN "\n");

	argsleft = argc - G.optind;
	lastarg = argc - 1;
	
	if (recursive && argsleft!=2)
		return cpmvusage(long_option);
	
	firstisdir = isdir(argv[G.optind]);
	if (bioserr)
		return ioerror(cmd,argv[G.optind],NULL,bioserr);
	
	lastisdir = isdir(argv[lastarg]);
	if (bioserr)
		return ioerror(cmd,argv[lastarg],NULL,bioserr);

	if (argsleft == 2)
	{
		/* just two args left */
		if (firstisdir)
			return cpmvdir(argv[G.optind],argv[lastarg]);
		if (lastisdir)
			return cpinto(argv[G.optind],argv[lastarg],filecopy);
		else
			return filecopy(argv[G.optind],argv[lastarg]);
	}		
	else
	{
		int retcode = 0;
		
		/* more than 2 args, so last one should be a dir */
		if (lastisdir)
		{
			/* cp/mv files into directory */
			for (i=G.optind; i<lastarg && !intr(); ++i)
			{
				int err = cpinto(argv[i],argv[lastarg],filecopy);
				
				if (err == -1 || intr())	/* user abort */
					return 2;
				else
					retcode = 1;
			}
		}
		else
		{
			/* oops, it isn't */
			eprintf(CP_NODIR "\n",cmd,strlwr(argv[lastarg]));
			retcode = 1;
		}
		return retcode;
	}
}

int m_cpmv(ARGCV)
{
	int retcode = 1;
	
	STRSELECT(strlwr(argv[0]))
	WHEN("cp")
		iscp = TRUE;
		retcode = docpmv(argc,argv,cp);
	WHEN("mv")
		iscp = FALSE;
		retcode = docpmv(argc,argv,mv);
	ENDSEL
	closehandles();
	return retcode;
}

/*
 * replace the extension of `filename' with `ext'. If `ext' is empty,
 * the extension is removed from `filename'
 */
static char *extreplace(char *filename,const char *ext)
{
	char *dot, *f;
	
	f = malloc(strlen(filename)+strlen(ext)+2);
	strcpy(f,filename);
	if (*ext=='.')
		++ext;
	dot = strrchr(f,'.');
	if (dot==NULL || dot < strrchr(f,'\\'))
	{
		if (*ext)
		{
			chrcat(f,'.');
			strcat(f,ext);
		}
	}
	else
	{
		if (*ext)
			strcpy(dot+1,ext);
		else
			*dot = '\0';
	}
	return f;
}

static struct option ren_long_option[] =
{
	{ "interactive", FALSE, &interactive, TRUE},
	{ "confirm", FALSE, &confirm, TRUE},
	{ "verbose", FALSE, &verbose, TRUE},
	{ NULL,0,0,0},
};

static int renameoptions (GETOPTINFO *G, int argc, char **argv)
{
	int c, opt_index = 0;

	cpoptinit();
	optinit (G);
	while ((c=getopt_long(G,argc,argv,"icv",ren_long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
			case 'i':
				interactive = TRUE;
				break;
			case 'c':
				confirm = TRUE;
				break;
			case 'v':
				verbose = TRUE;
				break;
			default:
				return printusage(ren_long_option);
		}
	return 0;
}	

int m_rename(ARGCV)
{
	GETOPTINFO G;
	int i, extlen, retcode = 0;
	char *newext, *newname;
	
	iscp = FALSE;
	if (renameoptions(&G,argc,argv)==1)
		return 1;
	if (argc-G.optind<2)
		return printusage(ren_long_option);

	newext = argv[G.optind];
	if (*newext=='.')
		++newext;
	extlen = (int)strlen(newext);
	if (extlen>3)
	{
		eprintf("rename: " CP_EXTERR "\n");
		return 1;
	}

	for (i=G.optind+1; i<argc; ++i)
	{
		if (isdir(argv[i]))
		{
			eprintf("rename: " CP_RENAMDIR "\n",strlwr(argv[i]));
			continue;
		}
		if (!access(argv[i],A_READ))
		{
			eprintf("rename: " CP_NOFILE "\n",strlwr(argv[i]));
			retcode = 1;
			continue;
		}
		newname = extreplace(argv[i],newext);
		if (!confirm && !interactive)
		{
			if (access(newname,A_READ))
			{
				eprintf("rename: " CP_EXISTS "\n",strlwr(newname));
				free(newname);
				retcode = 1;
				continue;
			}
		}
		if (!mv(argv[i],newname))
			retcode = 1;
		free(newname);
	}
	return retcode;
}

int m_backup(ARGCV)
{
	GETOPTINFO G;
	char *newname;
	int i;

	iscp = TRUE;
	if (renameoptions(&G,argc,argv)==1)
		return 1;
	
	if (argc==G.optind)
		return printusage(ren_long_option);
	
	for (i=G.optind; i<argc; ++i)
	{
		if (!isdir(argv[i]))
		{
			newname = extreplace(argv[i],"bak");
			cp(argv[i],newname);
			free(newname);
		}
		else
			eprintf("backup: " CP_BACKUP "\n",strlwr(argv[i]));
	}
	closehandles();
	return 0;
}
