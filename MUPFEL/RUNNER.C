/*
 * runner.c  -  run programs, restart mupfel/gemini afterwards
 * 05.01.91
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <aes.h>
#include <vdi.h>
#include <tos.h>

#include "alert.h"
#include "attrib.h"
#include "handle.h"
#include "mupfel.h"
#include "toserr.h"

#define TRUE	1
#define FALSE	0

#define DEBUG	0

#define canexec(x)		access(x,A_EXEC)

SCCS(runner);

extern BASPAG *_BasPag;

int vwk, xpix, ypix, wchar, hchar;
int failed;
int GEMversion;
char *cmddup;

static char nextcmd[300], nextargs[300];
static char newdir[300];

#define X_MAGIC	((int)0x601A)

/*
 * access(char *file, int mode)
 * determine accessibility of file according to mode
 * mode values:	check performed:
 *	A_EXITS		could be opened for reading
 *	A_READ		dto
 *	A_RDONLY		exists, is read-only
 *	A_WRITE		could be opened for writing
 *	A_RDWR		dto
 *	A_EXEC		exists, and is a TOS binary
 *				(magic number X_MAGIC in 1st 2 bytes == X_MAGIC)
 */
int access(const char *file, int mode)
{
	int hnd = ILLHND, retcode, magic, found, readonly;
	DTA dta, *olddta;
	
	int bioserr = 0;

	olddta = Fgetdta();
	Fsetdta(&dta);
	found = Fsfirst(file,FA_FILES);
	Fsetdta(olddta);

	if (bioserror(found))
		bioserr = found;
	if (found != 0 || dta.d_attrib & (FA_DIREC|FA_LABEL))
		return FALSE;

	readonly = dta.d_attrib & FA_RDONLY;
	
	switch (mode)
	{
		case A_EXIST:
		case A_READ:
			retcode = TRUE;
			break;
		case A_WRITE:
		case A_RDWR:
			retcode = !readonly;
			break;
		case A_RDONLY:
			retcode = readonly;
			break;
		case A_EXEC:
			if ((hnd=Fopen(file,0))>=MINHND)
			{
				if (Fread(hnd,sizeof(magic),&magic)!=sizeof(magic))
					magic = 0;
				retcode = (magic == X_MAGIC);
			}
			else
			{
				retcode=FALSE;
				if (bioserror(hnd))
					bioserr = hnd;
			}
			if (hnd >= MINHND)
				Fclose(hnd);
			break;
		default:
			retcode=FALSE;
			break;
	}
	if (bioserr)
		alert(1,1,1,RN_IOERR,"OK",file);
	return retcode;
}

void delaccwindows(void)
{
	int window;
	
	while (wind_get(0,WF_TOP,&window)>0 && window>0)
		wind_delete(window);
}

int windnew(void)
{
	if (GEMversion==0x104 || (GEMversion>=0x130 && GEMversion!=0x210))
	{
		wind_new();
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * gemgimmick(char *cmd)
 * display desktop-like opening sequence
 */
void gemgimmick(char *cmd)
{
	int pxy[4];
	int textx, texty;
	
	wind_update(BEG_UPDATE);
	/* force redraw of desktop background */
	wind_set(0,WF_NEWDESK,0,0,0,0);
	form_dial(FMD_FINISH,0,0,0,0,0,0,xpix,ypix);
	/* white bar at top */
	vsf_color(vwk,WHITE);
	pxy[0] = pxy[1] = 0;
	pxy[2] = xpix;
	pxy[3] = hchar+2;
	v_bar(vwk,pxy);
	/* command's name centered */
	textx = (xpix - (int)strlen(cmd)*wchar)/2;
	texty = hchar-1;
	v_gtext(vwk,textx,texty,cmd);
	/* black line under bar */
	pxy[0] = 0;
	pxy[1] = pxy[3] = hchar+2;
	pxy[2] = xpix;
	v_pline(vwk,2,pxy);
	wind_update(END_UPDATE);
}

static void openvwork(void)
{
	int workin[11] = { 1,1,1,1,1,1,1,1,1,1,2 };
	int workout[57];
	int dummy;

	vwk = graf_handle(&wchar, &hchar, &dummy, &dummy);
	v_opnvwk(workin,&vwk,workout);
	if (vwk <= 0)
	{
		form_alert(1,RN_NOVWK);
		exit(2);
	}
	xpix = workout[0];	/* x-axis resolution */
	ypix = workout[1];	/* y-axis resolution */
}

static void buildcmd(int argc, char **argv,COMMAND *c)
{
	int i;
	
	if (argc>1)
	{
		strcpy(c->command_tail,argv[1]);
		i=2;
		while (i<argc && strlen(c->command_tail)+strlen(argv[i])+1<125)
		{
			strcat(c->command_tail," ");
			strcat(c->command_tail,argv[i]);
			++i;
		}
		if (i<argc)
			strcpy(c->command_tail,"");
		c->length=(char)strlen(c->command_tail);
	}
	else
	{
		strcpy(c->command_tail,"");
		c->length = (char)0;
	}
}

/*
 * gemprg(char *cmd)
 * return TRUE if cmd ends with .PRG, .APP or .ACC, FALSE otherwise
 */
static int gemprg(char *cmd)
{
	char *dot;

	dot = strrchr(cmd,'.')+1;
	return !strcmp(dot,"PRG") || !strcmp(dot,"APP") || !strcmp(dot,"ACC");
}

/*
 * strfnm(char *path)
 * return pointer to filename part of path
 */
char *strfnm(char *path)
{
	char *s;
	
	if ((s=strrchr(path,'\\'))!=NULL)
		return s+1;
	else
		if (path[1]==':')
			return &path[2];
		else
			return path;
}

/*
 * strrpos(char *str, char ch)
 * return position from right of character ch in str, -1 if not found
 */
int strrpos(const char *str, char ch)
{
	int i;
	
	for (i=(int)strlen(str); i>=0; --i)
		if (str[i] == ch)
			return i;
	return -1;
}

/*
 * strdnm(const char *path)
 * return pointer to allocated copy of directory part of path
 */
char *strdnm(const char *path)
{
	char *new = strdup(path);
	int backslash;

	backslash = strrpos(new,'\\');	/* find last \ */
	if (backslash > -1)
	{
		/* if 1st char or root dir on drive, increment */
		if (backslash==0 || (backslash==2 && new[1]==':'))
			++backslash;
		/* terminate string after dir name */
		new[backslash]='\0';
	}
	else
		*new='\0';
	return new;
}

static void checkerr(long excode,char *cmdpath)
{
	if (excode < 0L)
	{
		switch ((int)excode)
		{
			case EFILNF:
				alert(1,1,1,"RUNNER: " RN_CANTEXEC,"OK",cmdpath);
				failed = TRUE;
				break;
			case ENSMEM:
				alert(1,1,1,"RUNNER: " RN_NOMEM,"OK",cmdpath);
				failed = TRUE;
				break;
			case EPLFMT:
				alert(1,1,1,"RUNNER: " RN_NOPGM ,"OK",cmdpath);
				failed = TRUE;
				break;
			default:
				if (bioserror(excode))
				{
					alert(1,1,1,"RUNNER: " RN_LOADERR,"OK",cmdpath);
					failed = TRUE;
				}
				break;
		}
	}
	else
		failed = (int)excode != 0;
}

/*
 * lastchr(char *str)
 * return last character in string
 */
char lastchr(const char *str)
{
	return str[strlen(str)-1];
}
/*
 * chrcat(char *str, char c)
 * append character c to str
 */
void chrcat(char *str,char c)
{
	int l=(int)strlen(str);
	
	str[l++]=c;
	str[l]='\0';
}

/*
 * chrapp(char *str, char ch)
 * append ch to str if it's not already the last character
 */
void chrapp(char *str,char ch)
{
	if (lastchr(str)!=ch)
		chrcat(str,ch);
}

/* get aes shell args */
static void getshellargs(char *nextcmd, char *nextargs,char *newdir)
{
	char *savecmd, *nextdir;

	*nextcmd = *nextargs = '\0';
	shel_read(nextcmd,nextargs);
	nextdir = strdnm(nextcmd);
	if (*nextdir=='\0')
	{
		savecmd=strdup(nextcmd);
		strcpy(nextcmd,newdir);
		if (*savecmd != '\\')
			chrapp(nextcmd,'\\');
		strcat(nextcmd,savecmd);
		free(savecmd);
	}
	free(nextdir);
#if DEBUG
	alert(1,1,1,"Next cmd = %s","OK",nextcmd);
#endif
}

int main(int argc, char **argv)
{
	COMMAND cmd;
	COMMAND notail = { 0, "" };
	long excode;
	int isgem = -1;
	int backslash;
	char *restartcmd;
	char restartpath[300], dummy[100];
	char cmdpath[300];
	
	if (appl_init()<0)
		exit(1);

	shel_read(restartpath,cmdpath);
	if (restartpath[1] != ':')
	{
		strcpy(dummy,restartpath);		/* save name */
		restartpath[0] = Dgetdrv() + 'A';
		restartpath[1] = ':';
		Dgetpath(&restartpath[2],0);
		chrapp(restartpath,'\\');
		strcat(restartpath,dummy);
	}
	
	GEMversion = _GemParBlk.global[0];
	
	if (argc<3)
	{
		form_alert(1,RN_NOARGS);
		exit(1);
	}

	openvwork();

	if (strlen(argv[2])==1)		
		strcpy(cmdpath,argv[3]);
	else
		strcpy(cmdpath,argv[2]);
	strupr(cmdpath);

	++argv;
	switch (toupper(**argv))
	{
		case 'G':
			restartcmd = "GEMINI.APP";
			break;
		case 'V':
			restartcmd = "VENUS.APP";
			break;
		default:
		case 'M':
			restartcmd = "MUPFEL.APP";
			break;
	}
	++argv;
	if (strlen(*argv)==1)
	{
		switch (toupper(**argv))
		{
			case 'G':
				isgem = TRUE;
				break;
			case 'T':
				isgem = FALSE;
				break;
		}
		--argc;
		++argv;
	}
	argc -= 2;
		
	buildcmd(argc,argv,&cmd);

restart:

	cmddup = strdup(cmdpath);

	if (isgem == -1)
		isgem = gemprg(cmdpath);
	if (isgem)
	{
		char *s = strdnm(cmdpath);

		gemgimmick(strfnm(cmdpath));
#if DEBUG
		alert(1,1,1,"cd to %s","OK",s);
#endif
		if (s[1] == ':')
			Dsetdrv(*s-'A');
		Dsetpath(&s[2]);
		strcpy(newdir,s);
		free(s);
	}
	else
	{
		graf_mouse(M_OFF, NULL);
		Cconws("\033E\033e\033q"); /* cls, cursor on, normal video */
	}

#if DEBUG
	alert(1,1,1,"shel_write(%s)","OK",cmdpath);
#endif
	shel_write(1,isgem,1,strfnm(cmdpath),(char *)&cmd);
#if DEBUG
	alert(1,1,1,"Pexec(%s)","OK",cmdpath);
#endif
	delaccwindows();

	appl_exit();
	excode = Pexec(0,cmdpath,&cmd,_BasPag->p_env);
	appl_init();
	
	Cconws("\033f"); /* cursor off */
	if (!isgem)
		graf_mouse(M_ON, NULL);
		
	checkerr(excode,cmdpath);
	
	getshellargs(nextcmd,nextargs,newdir);
	if (!failed && strcmp(strfnm(nextcmd),strfnm(cmddup)) && canexec(nextcmd))
	{
		if (!windnew())
			delaccwindows();
		gemgimmick(nextcmd);
		strcpy(cmdpath,nextcmd);
		cmd.length = nextargs[0];
		strcpy(cmd.command_tail,&nextargs[1]);
		free(cmddup);
#if DEBUG
		if (alert(1,1,2,"Should exec %s","OK","Cancel",cmdpath)==1)
#endif
		goto restart;
	}

	/* restart Gemini/Mupfel/Venus */

	backslash = strrpos(restartpath,'\\');	/* find last \ */
	if (backslash > -1)
	{
		/* if 1st char or root dir on drive, increment */
		if (backslash==0 || (backslash==2 && restartpath[1]==':'))
			++backslash;
		/* terminate string after dir name */
		restartpath[backslash]='\0';
	}
	else
		restartpath[0] = '\0';
	chrapp(restartpath,'\\');
	strcat(restartpath,restartcmd);

#if DEBUG
	alert(1,1,1,"shel_write(%s)","OK", restartpath);
#endif
	shel_write(1,1,1, restartpath, (char *)&notail);

	v_clsvwk(vwk);
	appl_exit();
		
	return (int)excode;
}
