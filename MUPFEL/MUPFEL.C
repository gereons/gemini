/*
 * mupfel.c  -  initialisation & main loop
 * 15.10.90
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "alert.h"
#include "alias.h"
#include "alloc.h"
#include "batch.h"
#include "chario.h"
#include "check.h"
#include "cmdline.h"
#include "curdir.h"
#include "comm.h"
#include "date.h"
#include "environ.h"
#include "errfile.h"
#include "exec.h"
#include "fkey.h"
#include "gemsubs.h"
#include "hash.h"
#include "imv.h"
#include "lineedit.h"
#include "mkargcv.h"
#include "mglob.h"
#include "mupfel.h"
#include "mversion.h"
#include "parse.h"
#include "redir.h"
#include "shellvar.h"
#include "stand.h"
#include "sysvec.h"
#include "timer.h"
#include "version.h"

#define DEBUG 0

#if MERGED
#include "flydial\flydial.h"
#endif

SCCS(mupfel);

int shellcmd;			/* command started via shellcall */
int bioserr;			/* BIOS/XBIOS error code */ 
int oserr;			/* TOS error code */
int sysret;			/* shellcall() return code */
int gotovenus;
int comefromvenus = FALSE;
const char *actcmd;		/* name of current internal command */
struct CommInfo CommInfo;

static int cmdlineargs;
static char venuscmdline[LINSIZ];
static char savedrive = '\0';

/* 
 * doshellcall(char *cmd)
 */
int doshellcall(char *shcmd)
{
	int ac, retcode;
	int oldshellcmd;
	char *av[MAXARGC];
	char olddrv, drv, *dir;
	char *cmd;
	linesrc *ls = curline;
	size_t oldpush = curline->pushedline;
	redirection saveredir;

#if DEBUG
	alert(1,1,1,"system(%s)","OK",shcmd);
#endif

	oldshellcmd = shellcmd;
	shellcmd = TRUE;

	saveredir = redirect;
	retcode = -1;
	olddrv = '\0';
	cmd = Malloc(strlen(shcmd)+300);
	strcpy(cmd,shcmd);
	drv = getdrv();
	dir = strdup(getdir());
	if (savedrive != '\0')
	{
		olddrv = drv;
		chdrv(savedrive);
	}
	setcurdir();
	sysret = 0;

	while (strlen(cmd)>0)
	{
		int newpush;
		
		ac = mkargcv(cmd,av);
		*cmd = '\0';
		if (ac>0)
			retcode = parse(ac,av);
		newpush = (ls == curline && curline->pushedline > oldpush);
		if (ls != curline || newpush)
			strcpy(cmd,getline(FALSE));
	}

	setdrv(drv);
	setdir(dir);
	if (olddrv!='\0')
		chdrv(olddrv);
	free(dir);
	Mfree(cmd);

	shellcmd = oldshellcmd;
	redirect = saveredir;

	if (sysret != 0)
		retcode = sysret;
	return retcode;
}

void copyright(void)
{
	mprintf("Mupfel Version %s [%s]\n",MUPFELVERSION,MUPFELDATE);
	mprintf("Ω 1990 by Gereon Steffens\n");
}

/*
 * initialization after execution of autoexec-file
 */
void lateinit(void)
{
	char tmpstr[100];
	
	if (getenv("HOME")==NULL)
	{
		sprintf(tmpstr,"HOME=%c:%s",tolower(getdrv()),getdir());
		putenv(tmpstr);
	}
	if (getenv("PATH")==NULL)
	{
		sprintf(tmpstr,"PATH=%c:\\;.",tolower(getdrv()));
		putenv(tmpstr);
	}
	if (getenv("SUFFIX")==NULL)
		putenv("SUFFIX=APP;PRG;GTP;TOS;TTP;MUP");
	if (getvarint("shellcount")>1)
	{
		char *imv;
		IMV *i;
		
		if ((imv=getenv("IMV"))!=NULL)
		{
			i = (IMV *)strtoul(imv,NULL,16);
			if (i!=NULL && !strcmp(i->magic,"IMV"))
			{
				inheritalias(i);
				inheritfkey(i);
				inherithash(i);
				inheritvars(i);
			}
		}
	}
	histinit();
}

/*
 * mupfelinit(ARGCV,char *envp)
 * initialize all internal data structures etc.
 */
static void
mupfelinit(ARGCV,char **envp)
{
	/* from conio.s */
	extern void LAinit(void);
	
	oserr = bioserr = 0;
	shellcmd = cmdlineargs = FALSE;
	CommInfo.isGemini = MERGED;
	CommInfo.PgmName = PGMNAME;
	CommInfo.fun.fmt_progress = NULL;
	CommInfo.errmsg = NULL;
	
	initsysvar();
	allocinit();
	initoutput();
	redirinit();
	initgem();
#if MERGED
	DialInit(Malloc, Mfree);
	/* Merged Version won't work with TOS 1.0 */
	if (tosversion()<0x102)
	{
		alert(STOP_ICON,1,1,MU_WRONGTOS,MU_TOOBAD,CommInfo.PgmName);
		DialExit();
		exitgem();
		exit(1);
	}
#endif
	timerinit();
	hashinit();
	initdrivelist();
	resetintr();
	curdirinit();
#if MERGED
	LAinit();
	greetings();
#endif
	envinit(envp);
	lineedinit();
	fkeyinit();
	shellinit();
	etvterminit();
	initcookie();
	initcheck();
	aliasinit();
	initerrfile();
#if STANDALONE
	if (getvarint("shellcount")==1)
		copyright();
#endif
	if (argc>1)
	{
		if (!stricmp(argv[1],"-c"))
		{
			setcmdline(argc,argv);
			cmdlineargs = TRUE;
			lateinit();
		}
		else
			autoexec(argv[1]);
	}
	else
		autoexec("MUPFEL.MUP");
}

void storevenuscmd(const char *cmdpath, int argc, const char *argv[])
{
	int i;
	
	strcpy(venuscmdline,cmdpath);
	for (i=1; i<argc; ++i)
	{
		strcat(venuscmdline," '");
		strcat(venuscmdline,argv[i]);
		strcat(venuscmdline,"'");
	}
}

static void domupfel(const char *cmd,int gemini)
{
	char *s, *av[MAXARGC];
	int ac, retcode;

	s = (cmd==NULL) ? getline(gemini) : cmd;

	/* empty line? */
	if (*s == '\0')
		return;

	ac=mkargcv(s,av);

	if (ac>0)
	{
		retcode = parse(ac,av);
		setvar("?",itos(retcode));
	}
	resetintr();
}

/*
 * char *mupfel(const char *cmd)
 * This is the main control loop.
 */
char *mupfel(const char *cmd)
{
	static int wascmd = FALSE;
	
	if (!execbatch())
		wascmd = (cmd!=NULL) || comefromvenus;
	
	venuscmd = gotovenus = FALSE;
	
	while (!gotovenus)
	{
		domupfel(cmd,wascmd);
		cmd = NULL;
		if (wascmd && !execbatch() && !curline->pushedline)
			gotovenus = TRUE;
		if (cmdlineargs && !execbatch())
		{
			char *argv[1];
			int m_exit(ARGCV);

			argv[0] = strdup("exit");
			m_exit(1,argv);
		}
	}
	/* Merged version: If it's a GEM app, let Venus do it */
	if (venuscmd)
	{
		savedrive = getdrv();
		return venuscmdline;
	}
	else
	{
		savedrive = '\0';
		return NULL;
	}
}

static void
mupfelmain(ARGCV,char **envp)
{
	/* Nasty trick to make shel_write work */
	if (argc>1 && !stricmp(argv[1],"-q"))
		exit(0);
		
	mupfelinit(argc,argv,envp);

	mupfel(NULL);
}

void
main (ARGCV, char *envp[])
{
	char **myargv;
	extern BASPAG *_BasPag;
	char *env;
	char *startpar;
	int count = 0;
	int i;

	/* Flag fÅr Verwendung von ARGV */
	if (_BasPag->p_cmdlin[0] != 127)
		mupfelmain (argc, argv, envp);

	/* Zeiger auf Env-Var merken */
	env = getenv("ARGV");
	if (!env)
		mupfelmain (argc, argv, envp);
		
	/* alle weiteren envp's lîschen */
	i = 0;
	while (strncmp (envp[i], "ARGV", 4)) i++;
	envp[i] = NULL;	

	/* alles, was dahinter kommt, abschneiden */	
	if (env[0] && env[-1])
	{
		*env++ = 0;			/* kill it */
		while (*env++);
	}
	
	/* Parameterstart */
	startpar = env;
	
	while (*env)
	{
		count++;
		while (*env++);
	}
	
	/* Speicher fÅr neuen Argument-Vektor */
	myargv = Malloc ((count+1)*sizeof (char *));
	env = startpar;
	
	count = 0;
	while (*env)
	{
		myargv[count++] = env;
		while (*env++);
	}
	myargv[count] = NULL;
	
	/* und ...argvmain() starten */
	mupfelmain (count, myargv, envp);	
}
