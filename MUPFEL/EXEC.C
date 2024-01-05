/*
 * exec.c  -  execute external command
 * 20.02.91
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "alert.h"
#include "alias.h"
#include "alloc.h"
#include "attrib.h"
#include "batch.h"
#include "chario.h"
#include "check.h"
#include "curdir.h"
#include "comm.h"
#include "environ.h"
#include "exec.h"
#include "exit.h"
#include "fkey.h"
#include "gemsubs.h"
#include "hash.h"
#include "imv.h"
#include "mupfel.h"
#include "parse.h"
#include "redir.h"
#include "shellvar.h"
#include "stand.h"
#include "sysvec.h"
#include "toserr.h"
#include "vt52.h"

#define DEBUG		0
#define RUN_DEBUG	0

SCCS(exec);

typedef struct
{
	char		xarg_magic[4];
	int		xargc;
	char		**xargv;
	char		*xiovector;
	BASPAG	*xparent;
} XARG;

typedef struct
{
	char	mouseon;		/* M: turn Mouse on? */
	char	cursoron;		/* C: turn alpha Cursor on? */
	char	greyback;		/* B: draw a grey Background? */
	char	aesargs;		/* S: pass args using Shel_write()? */
	char	chdir;		/* D: Cd to where the program lives? */
	char	exarg;		/* X: use Atari's EXARG scheme? */
	char	inwindow;		/* W: only used for GEMINI */
	char	waitkey;		/* K: wait for Key after TOS pgms? */
	char	overlay;		/* O: Overlay mupfel with program (using RUNNER)? */
	char	cblink;		/* L: cursor bLinking? */
	char silentxarg;	/* implicit value from PRGNAME_EXT/???DEFAULT */
} startparms;

int venuscmd;

static XARG xarg;
static IMV imv;
static char nextargs[CMDLINESIZE];
static char nextcmd[CMDLINESIZE];		/* for shel_read */
static char cmddup[CMDLINESIZE];

static int appprg(char *cmd)
{
	char *dot = strrchr(cmd,'.');

	++dot;
	return !strcmp(dot,"APP");
}

/*
 * gemprg(char *cmd)
 * return TRUE if cmd ends with .PRG, .APP or .ACC, FALSE otherwise
 */
static int gemprg(char *cmd)
{
	char *dot;
	char *s, *gs;

	dot = strrchr(cmd,'.');
	++dot;
	
	if ((gs=getenv("GEMSUFFIX"))==NULL)	
		return !strcmp(dot,"PRG") || !strcmp(dot,"APP") ||
		       !strcmp(dot,"ACC") || !strcmp(dot,"GTP");
	else
	{
		gs = strupr(strdup(gs));
		s = strtok(gs,";,");
		while (s != NULL)
		{
			if (!strcmp(dot,s))
			{
				free(gs);
				return TRUE;
			}
			s = strtok(NULL,";,");
		}
	}
	free(gs);
	return FALSE;
}

static void buildcmd(int argc, char **argv,COMMAND *c,startparms *parms)
{
	int i;
	
	if (argc>1)
	{
		strcpy(c->command_tail,argv[1]);
		i=2;
		while (i<argc && strlen(c->command_tail)+strlen(argv[i])+1 < 125)
		{
			chrcat(c->command_tail,' ');
			strcat(c->command_tail,argv[i]);
			++i;
		}
		if (i<argc)
		{
			if (!shellcmd && !parms->silentxarg)
				mprintf(EX_USEXARG "\n");
			strcpy(c->command_tail,"");
		}
		c->length = (char)strlen(c->command_tail);
		if (parms->exarg)
		{
			if (c->length == 0)
				memset(c->command_tail,0,sizeof(c->command_tail));
			c->length = (char)127;
		}
	}
	else
	{
		strcpy(c->command_tail,"");
		c->length = 0;
	}
}

static void getstartparms(char *prgname,startparms *parms)
{
	char *parval, *pv, *pv2, *varname = strdup(prgname);
	int val;

	if ((pv=strchr(varname,'.'))!=NULL)
		*pv='_';
	strupr(varname);
	
	if ((parval=getenv(varname))!=NULL)
	{
		pv2 = strdup(parval);
		pv = strtok(pv2,";,");
		while (pv!=NULL)
		{
			if (strlen(pv)>2 && pv[1]==':')
			{
				val = toupper(pv[2])=='Y';
				switch (toupper(pv[0]))
				{
					case 'M':
						parms->mouseon = val;
						break;
					case 'C':
						parms->cursoron = val;
						break;
					case 'B':
						parms->greyback = val;
						break;
					case 'S':
						parms->aesargs = val;
						break;
					case 'D':
						parms->chdir = val;
						break;
					case 'W':
						parms->inwindow = val;
						break;
					case 'X':
						parms->exarg = val;
						parms->silentxarg = TRUE;
						break;
					case 'K':
						parms->waitkey = val;
						break;
					case 'O':
						parms->overlay = val;
						break;
					case 'L':
						parms->cblink = val;
						break;
				}
			}
			pv=strtok(NULL,";,");
		}
		free(pv2);
	}
	free(varname);
}

static void initparams(int isgem,int isapp,char *cmdpath,startparms *parms)
{
	/* set default params */
	if (isgem)
	{
		parms->mouseon = TRUE;
		parms->greyback = TRUE;
		parms->cursoron = FALSE;
		parms->aesargs = TRUE;
		parms->chdir = TRUE;
		parms->inwindow = FALSE;
	}
	else
	{
		parms->mouseon = FALSE;
		parms->greyback = FALSE;
		parms->cursoron = TRUE;
		parms->aesargs = FALSE;
		parms->chdir = FALSE;
		parms->inwindow = TRUE;
	}
	parms->waitkey = parms->overlay = parms->cblink = FALSE;
	parms->exarg = TRUE;
	parms->silentxarg = FALSE;
	/* get user defaults */
	getstartparms(isgem ? (isapp ? "APPDEFAULT" : "GEMDEFAULT") :
		"TOSDEFAULT",parms);
	/* get this programs settings */
	getstartparms(strfnm(cmdpath),parms);
}

/* set up xArg structure */
static void setupxarg(int argc, char **argv)
{
	char xenv[32];

	strncpy(xarg.xarg_magic,"xArg",4);
	xarg.xargc = argc;
	xarg.xargv = argv;
	xarg.xiovector = NULL;
	xarg.xparent = getactpd();
	sprintf(xenv,"xArg=%08lX",(ulong)&xarg);
	putenv(xenv);
}

static void setupimv(void)
{
	char imvenv[32];
	
	strcpy(imv.magic,"IMV");
	imv.alias = aliasaddress();
	imv.fkey = fkeyaddress();
	imv.hash = hashaddress();
	imv.shvar = shvaraddress();
	sprintf(imvenv,"IMV=%08lX",(ulong)&imv);
	putenv(imvenv);
}

/* get aes shell args */
static void getshellargs(char *newdir)
{
	char *savecmd, *nextdir;

	*nextcmd = *nextargs = '\0';
	shellread(nextcmd,nextargs);
	nextdir = strdnm(nextcmd);
	if (*nextdir=='\0')
	{
		savecmd=strdup(nextcmd);
		strcpy(nextcmd,newdir);
		if (*savecmd!='\\')
			chrapp(nextcmd,'\\');
		strcat(nextcmd,savecmd);
		free(savecmd);
	}
	free(nextdir);
}

static void initcmd(char *cmdpath,int *isgem,int *isapp,startparms *parms)
{
	strupr(cmdpath);
	*isgem = gemprg(cmdpath);
	*isapp = appprg(cmdpath);
	initparams(*isgem,*isapp,cmdpath,parms);
}

static void checkrunner(char *path,COMMAND *cmd,int isgem)
{
	char runpath[256], *end;
	char cmdpath2[256];
	int backslash;

	if (*path == '.')
		sprintf(cmdpath2,"%c:%s\\%s",
			getdrv(),getdir(),
			path[1] != '\\' ? path : &path[2]);
	else
		strcpy(cmdpath2,path);
#if RUN_DEBUG
	alert(1,1,1,"cmdpath2 = %s","OK",cmdpath2);
#endif
	strcpy(runpath,cmd->command_tail);
#if MERGED
	strcpy(cmd->command_tail,"G ");
#else
	strcpy(cmd->command_tail,"M ");
#endif
	strcat(cmd->command_tail,isgem ? "G " : "T ");
	strcat(cmd->command_tail,cmdpath2);
	chrcat(cmd->command_tail,' ');
	end = strchr(cmd->command_tail,0);
	strncpy(end,runpath,124-strlen(cmd->command_tail));
	cmd->length = strlen(cmd->command_tail);

#if RUN_DEBUG
	alert(1,1,1,"cmd tail = %s","OK",cmd->command_tail);
#endif

	strcpy(runpath,CommInfo.PgmPath);
	backslash = strrpos(runpath,'\\');	/* find last \ */
	if (backslash > -1)
	{
		/* if 1st char or root dir on drive, increment */
		if (backslash==0 || (backslash==2 && runpath[1]==':'))
			++backslash;
		/* terminate string after dir name */
		runpath[backslash]='\0';
	}
	else
		runpath[0] = '\0';
	chrapp(runpath,'\\');
	strcat(runpath,"RUNNER.APP");

#if RUN_DEBUG
	alert(1,1,1,"runpath = %s","OK",runpath);
#endif
	if (!access(runpath,A_EXEC))
		alertstr(EX_RUNNER);
	else
	{
		while (execbatch())
			closebatch();
		shellwrite(1,1,1,runpath,(char *)cmd);
#if STANDALONE
		terminate(TRUE);
		allocexit();
		exit(0);
#else
		CommInfo.cmd = overlay;
		return;
#endif
 	}
}

/*
 * execcmd(ARGC)
 * Execute the command argv[0]. rebuild commandline from argv,
 * fill xArg structure.
 * For gem programs, turn cursor off, draw grey background, turn
 * mouse on, change dir to where the program is.
 * Call Pexec(), undo all the stuff.
 * For non-interactive use (i.e. shellcmd==TRUE), skip the fancy
 * stuff.
 */
int execcmd(char *cmdpath, ARGCV)
{
	COMMAND cmd;					/* command line args */
	COMMAND notail = { 0, "" };		/* for shel_write */
	char *newdir;					/* dir for GEM programs */
	char *execenv;					/* TOS environment */
	int isgem, isapp, failed;
	long excode;					/* Pexec() return code */
	startparms parms;				/* tuneable parameters */

#if MERGED
	int venusexec;
	
	if (isautoexec())
	{
		alert(1,1,1,EX_CANTRUN,EX_TOOBAD);
		return 0;
	}
#endif

	initcmd(cmdpath,&isgem,&isapp,&parms);
	buildcmd(argc,argv,&cmd,&parms);
 	setupxarg(argc,argv);
 	setupimv();

	if (!shellcmd && parms.overlay)
	{
		checkrunner(cmdpath,&cmd,isgem);
		gotovenus = TRUE;
		putenv("xArg");
		putenv("IMV");
		return 0;
	}

restart:

	initcmd(cmdpath,&isgem,&isapp,&parms);
	strcpy(cmddup,cmdpath);

#if MERGED
	venusexec = isgem || parms.greyback || !parms.inwindow;
	
	if (!shellcmd && venusexec && !curline->batch.autoexec)
	{
		gotovenus = TRUE;
		venuscmd = TRUE;
		storevenuscmd(cmdpath,argc,argv);
		/*
		 * decrement hit counter, as Venus is going to execute this pgm
		 * via system(), and that will increment it again 
		 */
		hashadjust(cmdpath); 
		putenv("xArg");
		putenv("IMV");
		return 0;
	}
#endif
	venuscmd = FALSE;

	if (!shellcmd)
	{
		if (!parms.cursoron)
			cursoroff();
		if (isgem)
			savescreen();
		if (parms.greyback)
		{
			/* 
			 * show the real name, in case the program was started
			 * using wildcards
			 */
			DTA dta, *olddta = Fgetdta();

			Fsetdta(&dta);
			Fsfirst(cmdpath,FA_FILES);
			gemgimmick(dta.d_fname,TRUE);
			Fsetdta(olddta);
		}
		if (parms.mouseon)
			mouseon();
	}

	if (parms.chdir)
	{
		savedir();
		newdir = strdnm(cmdpath);
		if (*newdir!='\0' && !chdir(newdir))
			alert(NOTE_ICON,1,1,EX_BADCD,EX_OOPS,newdir);
	}
	else
		newdir = getdir(); /* free() only when parms.chdir==TRUE */

	shellwrite(1,isgem,1,cmdpath,
		parms.aesargs ? (char *)&cmd : (char *)&notail);

	/* Build the environment. Allocate space via Malloc() */
	execenv = makeenv(parms.exarg && argc>1,argc,argv,cmdpath);

	/* Set cursor blinking */
	if (parms.cblink)
		Cursconf(2,0);
	else
		Cursconf(3,0);
	
#if MERGED
	if (parms.inwindow)
	{
		int x,y;
		/* from conio.s */
		extern void storeLAcursor(int x,int y);
		
		getcursor(&x,&y);
		storeLAcursor(x,y);
	}
#endif

	if (!shellcmd)
		windupdate(FALSE);

	if (!isgem)
		criticinit();
	etvtermexit();
	excode = Pexec(0,parms.chdir ? strfnm(cmdpath) : cmdpath,
		&cmd,execenv);
	if (shellcmd)
		CommInfo.errmsg = NULL;
	etvterminit();
	if (!isgem)
		criticexit();

	if (!shellcmd)
		windupdate(TRUE);

	if (!shellcmd && !isgem && parms.waitkey)
	{
		mprintf("\n" EX_PRESSCR);
		inchar();
	}
	
	checkcode();

	Mfree(execenv);
	/* undo redirection. */
	endredir(FALSE);
	/* remove $xArg and $IMV from env */
	putenv("xArg");
	putenv("IMV");

#if MERGED
	Cursconf(0,0);		/* Definitely turn BIOS cursor off */
#endif
	
	if (!shellcmd)
	{
		cursoroff();	/* just in case */
		if (isgem)
			windnew();
		if (parms.mouseon)
			mouseoff();
		if (parms.greyback)
			clearscreen();
	}

	getshellargs(newdir);
#if DEBUG
	alert(1,1,1,"next cmd: %s","OK",nextcmd);
#endif

	if (parms.chdir)
	{
		restoredir();
		free(newdir);
	}
	if (!shellcmd)
	{
		if (isgem)
			restorescreen();
		cursoron();
		wrapon();
	}
	
	failed = FALSE;
	if (excode < 0L)
	{
		switch (oserr=(int)excode)
		{
			case EFILNF:
				eprintf("%s: " EX_CANTEXEC "\n",argv[0],cmdpath);
				failed = TRUE;
				break;
			case ENSMEM:
				eprintf(EX_NOMEM "\n",argv[0]);
				failed = TRUE;
				break;
			case EPLFMT:
				eprintf(EX_NOTTOS "\n",argv[0]);
				failed = TRUE;
				break;
			default:
				if (bioserror(excode))
				{
					ioerror(argv[0],cmdpath,NULL,(int)excode);
					failed = TRUE;
				}
				break;
		}
	}
	else
	{
		/* Pexec()==(int)-1  ->  crash */
		if ((int)excode == ERROR && pgmcrash())
		{
			if (alert(STOP_ICON,1,2,EX_CRASH,EX_OK,EX_REBOOT,cmdpath)==2)
				reboot();
				/* NOTREACHED */
			else
				clearcrashflag();
		}
		failed = (int)excode != 0;
	}
	oserr=0;

#if DEBUG
	alert(1,1,1,"next=%s, cmd=%s","OK",nextcmd,cmddup);
#endif

	if (!failed && strcmp(strfnm(nextcmd),strfnm(cmddup)) && 
		canexec(nextcmd)!=EXEC_NOTFOUND)
	{
		if (!windnew())
			delaccwindows();
		gemgimmick(nextcmd,FALSE);
		strcpy(cmdpath,nextcmd);
		cmd.length = nextargs[0];
		strcpy(cmd.command_tail,&nextargs[1]);
#if DEBUG
		if (alert(1,1,2,"Should exec %s","OK","Cancel",cmdpath)==1)
#endif
		goto restart;
	}

	CommInfo.dirty = (size_t)Drvmap();
	return (int)excode;
}
