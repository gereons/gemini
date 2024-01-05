/*
 * curdir.c  -  cd, pwd, pushd, popd and dirs commands
 * 09.02.91
 */
 
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <tos.h>

#include "alert.h"
#include "alloc.h"
#include "attrib.h"
#include "batch.h"
#include "chario.h"
#include "curdir.h"
#include "comm.h"
#include "environ.h"
#include "getopt.h"
#include "df.h"
#include "mupfel.h"
#include "shellvar.h"
#include "sysvec.h"
#include "toserr.h"

#define MAXSAVE	50	/* max. number of saved directories */
#define MAXSTACK	50	/* max. number of dir stack entries */

SCCS(curdir);

#define DEBUG	0

typedef struct
{
	char *dir;
	char drv;
} dir;

static char curdrv, curdir[300];
static dir dstack[MAXSTACK];
static dir sdir[MAXSAVE];
static int savep, dstackp;

int setcurdir(void)
{
	int err;
	char cwd[300];
	
	curdrv = Dgetdrv();
	err = Dgetpath(curdir,curdrv + 1);
	if (bioserror(err))
		return ioerror("setcurdir",NULL,NULL,err);
	curdrv += 'A';
	if (*curdir=='\0')
		strcpy(curdir,"\\");
	cwd[0] = tolower(curdrv);
	cwd[1] = ':';
	strcpy(&cwd[2],strlwr(curdir));
	setvar("cwd",cwd);
	return TRUE;
}

#if MERGED
void getMupfelWD(char *path,size_t maxlen)
{
	if (strlen(getdrv())+3 > maxlen)
		*path='\0';
	else
		sprintf(path,"%c:%s",getdrv(),getdir());
}
#endif

void curdirinit()
{
	savep = dstackp = 0;
#if MERGED
	CommInfo.getMupfelWD = getMupfelWD;
#endif
	setcurdir();
}

int legaldrive(char drv)
{
	drv = toupper(drv);
	
	if (drv<'A' || drv>'Z')
		return FALSE;
	if (((Drvmap()>>(drv-'A'))&1)!=1)
		return FALSE;
	else
		return TRUE;
}

/*
 * normalize(char *d)
 * Build an absolute dir specifier from the possibly weird construct
 * in d. Understands about things like a\b\..\c\.\d\.. (which means
 * a\c).
 * Returns pointer to allocated copy of normalized pathname.
 * Written by Steve and Gereon, with help from the thirsty man :-)
 */
char *normalize(const char *d)
{
	size_t dirlen = 256;		/* "enough" */
	char *dir, *normdir, *dirtok, *backslash;
	int lastback;

	/* remember if last char is \ */
	lastback = lastchr (d)=='\\';

	dir = malloc(dirlen);		/* temp storage */
	normdir = malloc(dirlen);	/* resulting dir */
	
	*normdir = *dir = '\0';		/* make empty strings */
	
	/* changed by jr: first: look for drive! */
	
	if (d[1] != ':')
	{
		chrcat (dir, curdrv);	/* insert drive name */
		chrcat (dir, ':');		/* delimiter */
	}
	else
	{
		dir[0] = *d++;			/* copy existing drive identifier */
		dir[1] = *d++;
		dir[2] = 0;
	}

	if (!legaldrive (dir[0]))	/* valid drive name? */
	{
		free (dir);
		free (normdir);
		return NULL;
	}	
	
	if (d[0] != '\\')			/* absolute path? */
	{
		int olddrv = curdrv;
		
		chdrv (dir[0]);
		strcat (dir, curdir);
		chdrv (olddrv);	

		if (lastchr (dir) != '\\')
			chrapp (dir, '\\');
	}
	
	strcat (dir, d);

	
	/* tokenize dir at \; uses mstrtok because CDPATH uses strtok */
	dirtok = mstrtok(dir,"\\");
	while (dirtok != NULL)
	{
		if (strcmp(dirtok,"."))				/* throw away ./ */
		{
			/* it's not . */
			if (strcmp(dirtok,".."))
			{
				/* it's not ".." or "." */

				/* append \ only if not first token */
				
				if (dirtok[0] && (dirtok[1] != ':'))
					chrcat (normdir, '\\');
	
				strcat (normdir, dirtok);
			}
			else
			{
				/* it's .., delete previous entry */
				backslash = strrchr(normdir,'\\');
				if (backslash == NULL)
				{
					/* No backslash -> too many ..'s */
					free(dir);
					free(normdir);
					return NULL;
				}
				else
						*backslash = '\0';

				/* why not? */

#if COMMENT
				{
					/* don't delete first backslash */
					if (backslash!=strchr(normdir,'\\'))
					else
						*(backslash+1) = '\0';
				}
#endif
			}
		}
		dirtok = mstrtok (NULL,"\\");
	}

	/* restore trailing \ if we had one or drive specfier resulted */
	if (lastback || (strlen(normdir)==2 && normdir[1]==':'))
		chrcat(normdir,'\\');

	free(dir);
	return normdir;
}

/*
 * dirstat(char *dir)
 * see if Fsfirst knows about dir as a directory
 */
static int dirstat(const char *dir)
{
	DTA dta, *olddta = Fgetdta();
	int fstat;

	Fsetdta(&dta);
	fstat = Fsfirst(dir,FA_ATTRIB);
	Fsetdta(olddta);

	if (bioserror(fstat))
	{
		bioserr = fstat;
		return 0;
	}
	return fstat==0 && tstbit(dta.d_attrib,FA_DIREC);
}

/*
 * checkdir(char *dir)
 * see if dir names an existing directory
 */
static int checkdir(const char *dir)
{
	char *mydir;
	int d, dirlen;

	/* This is the easy case */
	if (dirstat(dir))
		return TRUE;
	if (bioserr)
		return FALSE;
			
	if ((mydir = normalize(dir))==NULL)	/* illegal anyway ? */
		return FALSE;
	/* Root dir is special: Fsfirst doesn't like it */
	if (!strcmp(mydir,"\\") || (strlen(mydir)==3 && strstr(mydir,":\\")))
	{
		free(mydir);
		return TRUE;
	}

	/* remove trailing backslash if not root dir */
	dirlen = (int)strlen(mydir);
	if (!(dirlen==1 || (dirlen==3 && mydir[1]==':')))
		if (lastchr(mydir)=='\\')
			mydir[dirlen-1] = '\0';

	/* ask Fsfirst about normalized dir */
	d = dirstat(mydir);
	free(mydir);
	return d;
}

int chdir(const char *dir)
{
	int offs=0;

#if DEBUG
	alert(1,1,1,"chdir(%s)","OK",dir);
#endif
	bioserr = 0;
	if (*dir=='\0')	/* Should never happen */
	{
		alertstr("chdir(\"\")");
		return FALSE;
	}

	if (dir[1]==':' && !legaldrive(dir[0]))
		return FALSE;
	if (!checkdir(dir))	/* No such dir */
		return FALSE;
		
	if (dir[1]==':')
	{
		if (chdrv(toupper(dir[0])))
		{
			offs=2;
			if (dir[offs]=='\0')
				return TRUE;
		}
		else
			return FALSE;
	}

	if (Dsetpath(&dir[offs])<0)
	{
		alert(1,1,1,"Can't chdir to %s","Oops",dir);
		return FALSE;
	}
	setcurdir();
	return TRUE;
}

int chdrv(char drv)
{
	int d, olddrv, oldbioserr;

	bioserr = 0;
	d = toupper(drv)-'A';
	olddrv = curdrv;

	if (legaldrive(drv))
	{
		Dsetdrv(d);
		if (Mediach(d))
		{
			if (!chdir("\\"))
			{
				oldbioserr = bioserr;
				chdrv(olddrv);
				bioserr = oldbioserr;
				return FALSE;
			}
		}
		setcurdir();
		return TRUE;
	}
	return FALSE;
}

static char *find_dirarg(ARGCV)
{
	int i, found = FALSE;
	char *founddir = NULL;
	
	for (i=1; i<argc; ++i)
	{
		if (isdir(argv[i]))
		{
			if (!found)
			{
				found = TRUE;
				founddir = argv[i];
			}
			else
				return NULL;
		}
	}
	return founddir;
}

int m_cd(ARGCV)
{
	char *home, *cdp, *cdpath, newdir[100], savdrv, *savdir, *dir;
	int m_pwd(ARGCV);
	int offs;

	if (CommInfo.isGemini && isautoexec())
	{
		alert(1,1,1,CD_NOCD,CD_TOOBAD);
		return 0;
	}
	
	if (argc==1)
	{
		if ((home=getenv("HOME"))!=NULL)
		{
			int ok = chdir(home);
		
			if (!ok)
				eprintf("cd: " CD_BADDIR "\n",home);
			return !ok;
		}
		else
		{
			eprintf(CD_NOHOME "\n");
			return 1;
		}
	}
	
	if (*argv[1]=='\0')
	{
		eprintf("cd: " CD_NOEMPTY "\n");
		return 1;
	}
	
	if (argc>2)
	{
		dir = find_dirarg(argc,argv);
		if (dir == NULL)
		{
			eprintf("cd: " CD_MANYARGS "\n");
			return printusage(NULL);
		}
	}
	else
		dir = argv[1];

	if (chdir(dir))
		return 0;

	savdrv = curdrv;
	savdir = strdup(curdir);

	if (*dir != '\\' && *dir != '.')
	{
		if ((cdp=getenv("CDPATH"))!=NULL)
		{
			cdpath=strdup(cdp);
			cdp=strtok(cdpath,";,");
			while (cdp != NULL)
			{
				strcpy(newdir,cdp);
				chrapp(newdir,'\\');
				offs = (dir[0]=='\\') ? 1 : 0;
				strcat(newdir,&dir[offs]);
				if (chdir(newdir))
				{
					m_pwd(1,NULL);
					free(cdpath);
					free(savdir);
					return 0;
				}
				cdp=strtok(NULL,";,");
			}
			free(cdpath);
		}
		chdrv(savdrv);
		chdir(savdir);
	}
	eprintf("cd: " CD_BADDIR "\n",dir);
	free(savdir);
	return 1;
}

void savedir(void)
{
#if DEBUG
	alert(1,1,1,"savedir %c:%s","OK",curdrv,curdir);
#endif
	sdir[savep].dir = strdup(curdir);
	sdir[savep].drv = curdrv;
	++savep;
	if (savep>=MAXSAVE)
		alertstr("savedir() stack overflow");
}

void restoredir(void)
{
	--savep;
	if (savep < 0)
	{
		alertstr("unbalanced save/restoredir() calls");
		return;
	}
#if DEBUG
	alert(1,1,1,"restoredir %c:%s","OK",sdir[savep].drv,sdir[savep].dir);
#endif
	chdrv(sdir[savep].drv);
	chdir("\\");
	chdir(sdir[savep].dir);
	free(sdir[savep].dir);
}

char getdrv(void)
{
	return curdrv;
}

void setdrv(char drv)
{
	curdrv = drv;
}

char *getdir(void)
{
	return curdir;
}

void setdir(char *dir)
{
	strcpy(curdir,dir);
}

static void printcurdir(char drv)
{
	chdrv(drv);
	mprintf("%c:%s\n",tolower(curdrv),curdir);
}

int m_pwd(ARGCV)
{
	GETOPTINFO G;
	char olddrv, *drvmap;
	static int allflag;
	struct option long_option[] =
	{
		{ "all", FALSE, &allflag, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index = 0, c;
	int retcode = 0;

	optinit (&G);	
	allflag = FALSE;
	if (argv)
		while ((c = getopt_long (&G, argc, argv, "a", long_option,
			&opt_index))!=EOF)
			switch (c)
			{
				case 0:
					break;
				case 'a':
					allflag = TRUE;
					break;
				default:
					return printusage(long_option);
			}

	if (allflag)
	{
		olddrv = curdrv;
		drvmap = drivemap();
		while (*drvmap && !intr())
		{
			if (legaldrive(*drvmap))
				printcurdir(*drvmap);
			++drvmap;
		}
		chdrv(olddrv);
	}
	else
	{		
		if (argc>1)
		{
			if (isdrive(argv[1]))
			{
				olddrv = curdrv;
				printcurdir(toupper(*argv[1]));
				chdrv(olddrv);
			}
			else
			{
				eprintf("pwd: " CD_NODRV "\n",argv[1]);
				retcode = 1;
			}
		}
		else
			printcurdir(curdrv);
	}
	return retcode;
}

void direxit(void)
{
	int i;
	
	for (i=dstackp-1; i>=0; --i)
		free(dstack[i].dir);
}

static void pushdir(void)
{
	dstack[dstackp].dir = strdup(curdir);
	dstack[dstackp].drv = curdrv;
	++dstackp;
	if (dstackp>=MAXSTACK)
		eprintf("pushd: " CD_MANYPUSH "\n");
}

static int popdir(void)
{
	int retcode;
	
	--dstackp;
	if (dstackp<0)
	{
		dstackp=0;
		eprintf("popd: " CD_NOPOP "\n");
		return FALSE;
	}
	
	if (!chdrv(dstack[dstackp].drv) || !chdir(dstack[dstackp].dir))
	{
		eprintf("popd: " CD_BADPOP "\n",
			dstack[dstackp].drv,dstack[dstackp].dir);
		retcode = FALSE;
	}
	else
		retcode = TRUE;
	free(dstack[dstackp].dir);
	return retcode;
}

int m_pushd(ARGCV)
{
	int dirok;
	
	if (argc>2)
		return printusage(NULL);
		
	savedir();
	dirok=chdir((argc>1) ? argv[1] : ".");
	if (dirok)
		pushdir();
	else
		eprintf("pushd: " CD_BADDIR "\n",argv[1]);
	restoredir();
	return !dirok;
}

int m_popd(void)
{
	return !popdir();
}

int m_dirs(void)
{
	int i;

	if (dstackp==0)
	{
		mprintf("dirs: " CD_NOSTACK "\n");
		return 1;
	}	
	for (i=dstackp-1; i>=0; --i)
		mprintf("%2d: %c:%s\n",
			i,dstack[i].drv,dstack[i].dir);
	return 0;
}
