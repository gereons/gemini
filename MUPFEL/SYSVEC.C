/*
 * sysvec.c  -  deals with system vectors like shell_p etc.
 * 25.01.91
 */
 
#include <stddef.h>
#include <string.h>
#include <tos.h>

#include "cookie.h"
#include "mupfel.h"
#include "mversion.h"
#include "shellvar.h"
#include "stand.h"
#include "sysvec.h"
#include "version.h"

#define JMPOP		0x4ef9	/* 68000 JMP <longaddr> Opcode */
#define SPA		4		/* Spanish TOS Country code */

/* TOS system variables */
#define XBIOS			((long *)0xb8L)
#define PROC_LIVES		((ulong *)0x380L)
#define ETV_CRITIC		((long *)0x404L)
#define ETV_TERM		((long *)0x408L)
#define CONTERM		((char *)0x484L)
#define _NFLOPS		((int *)0x4a6L)
#define _HZ_200		((ulong *)0x4baL)
#define _SYSBASE		((SYSHDR **)0x4f2L)
#define _SHELL_P		((long *)0x4f6L)
#define PUN_PTR		((PUN_INFO **)0x516L)
#define _O_CON			((long *)0x586L)
#define _O_RAWCON		((long *)0x592L)
#define _P_COOKIES		((long **)0x5a0L)
#define _CPUTYPE		((int *)0x59eL)
#define OS_ACT_PD		((BASPAG **)0x602cL)
#define OS_ACT_PD_SPA	((BASPAG **)0x873cL)

#define STANDALONE_XBRA	"MUPF"
#define MERGED_XBRA		"GMNI"

#if STANDALONE
#define XBRA_ID	STANDALONE_XBRA
#else
#define XBRA_ID	MERGED_XBRA
#endif

SCCS(sysvec);

typedef struct
{
	char xb_magic[4];		/* "XBRA" */
	char xb_id[4];			/* vector owner's ID */
	long xb_oldvec;		/* original vector */
	int  xb_jmp;			/* JMP opcode (Mupfel extension) */
	long xb_newvec;		/* Address for JMP (Mupfel extension */
} xbra;

static xbra xshell;		/* for _shell_p */
static xbra xetv_critic;	/* for etv_critic */
static xbra xetv_term;	/* for etv_term */
static int mycritic = 0;
static int myetvterm = 0;
static SYSHDR *sysbase;
#if MERGED
static xbra ocon, rocon;	/* for _o_con and _o_rawcon */
static xbra xb_xbios;
long OldXBios;
#endif

int conwindow = 0;

#if MERGED
static int savedLA = FALSE;
static int xold, yold;
#endif

static void supervisor(int flag)
{
	static long oldssp;
	
	if (flag)
		oldssp = Super(0L);
	else
		Super((void *)oldssp);
}

static int ismyxbra(char *magic,char *id)
{
	return !strncmp(magic,"XBRA",4) &&
		(!strncmp(id,STANDALONE_XBRA,4) ||
		 !strncmp(id,MERGED_XBRA,4));
}

/*
 * shellcount(void)
 * count number of times Mupfel and/or Gemini is in memory.
 */
static int shellcount(void)
{
	xbra *x;
	int shcount = 0;

	supervisor(TRUE);
	x = (xbra *)*_SHELL_P;
	supervisor(FALSE);

	while (x != NULL)
	{
		/* decrement by 12, not 12*sizeof(xbra) !!!! */
		(char *)x -= offsetof(xbra,xb_jmp);
		if (ismyxbra(x->xb_magic,x->xb_id))
			++shcount;
		else
			break;
		/* Prevent endless loops left after crashes */
		if ((xbra *)x->xb_oldvec != x)
			x = (xbra *)x->xb_oldvec;
		else
			x = NULL;
	}
	return shcount;
}

/*
 * void xbrainst(xbra *x,long *vector,void *routine)
 * Install routine at vector using XBRA
 */
static void xbrainst(xbra *x,long *vector,void *routine)
{
	supervisor(TRUE);	
	strncpy(x->xb_magic,"XBRA" XBRA_ID,8);
	x->xb_oldvec = *vector;
	x->xb_jmp = JMPOP;
	x->xb_newvec = (long)routine;
	*vector = (long)&x->xb_jmp;
	supervisor(FALSE);
}

static void xbradeinst(xbra *xb,long *vector)
{
	xbra *x;
	
	supervisor(TRUE);
	/* still my vector ? */
	if (*vector == (long)&xb->xb_jmp)
	{
		*vector = xb->xb_oldvec;
	}
	else
	{
		long *addr = vector;
		
		do
		{
			x = (xbra *)*addr;
			(char *)x -= offsetof(xbra,xb_jmp);
			if (strncmp(x->xb_magic,"XBRA",4))
			{
				/* 
				 * someone grabbed that vector without XBRA
				 * kill the fool
				 */
				*vector = xb->xb_oldvec;
				break;
			}
			addr = (long *)x->xb_oldvec;
		} while (x->xb_oldvec != (long)&xb->xb_jmp);
		x->xb_oldvec = xb->xb_oldvec;
	}
	supervisor(FALSE);
}

/*
 * shellinit(void)
 * initialize XBRA structure for _shell_p
 */
void shellinit(void)
{
	/* from shell.s */
	extern int cdecl shellcall(char *cmd);
	extern void initreset(void);
	extern char _xresetid[];

	xbrainst(&xshell,_SHELL_P,shellcall);
	setvar("shellcount",itos(shellcount()));
	supervisor(TRUE);
	initreset();
	supervisor(FALSE);
	strncpy(_xresetid,XBRA_ID,4);
}

/*
 * shellexit(void)
 * reset _shell_p to original value
 */
void shellexit(void)
{
	/* from shell.s */
	extern void exitreset(void);

	xbradeinst(&xshell,_SHELL_P);
	supervisor(TRUE);	
	exitreset();
	supervisor(FALSE);
}

void criticinit(void)
{
	/* from toscrit.s */
	extern int toscritic(void);
	
	if (mycritic == 0)
		xbrainst(&xetv_critic,ETV_CRITIC,toscritic);	
	++mycritic;
}

void criticexit(void)
{
	--mycritic;
	if (mycritic == 0)
		xbradeinst(&xetv_critic,ETV_CRITIC);
}

static void doetvterm(void)
{
#if MERGED
	if (conwindow)	/* if console i/o still in window */
	{
		*_O_CON = ocon.xb_oldvec;
		*_O_RAWCON = rocon.xb_oldvec;
		*XBIOS = xb_xbios.xb_oldvec;
	}
#endif
	*_SHELL_P = xshell.xb_oldvec;
	exitreset();
	*ETV_TERM = xetv_term.xb_oldvec;	
}

void etvterminit(void)
{
	if (myetvterm == 0)
		xbrainst(&xetv_term,ETV_TERM,doetvterm);
	++myetvterm;
}

void etvtermexit(void)
{
	--myetvterm;
	if (myetvterm == 0)
		xbradeinst(&xetv_term,ETV_TERM);
}

int getnflops(void)
{
	int nflops;

	supervisor(TRUE);	
	nflops = *_NFLOPS;
	supervisor(FALSE);
	return nflops;
}

ulong gethz200(void)
{
	ulong timer;

	supervisor(TRUE);	
	timer = *_HZ_200;
	supervisor(FALSE);
	return timer;
}

int pgmcrash(void)
{
	ulong magic;
	
	supervisor(TRUE);
	magic = *PROC_LIVES;
	supervisor(FALSE);
	return magic == 0x12345678UL;
}

void clearcrashflag(void)
{
	supervisor(TRUE);
	*PROC_LIVES = 0;
	supervisor(FALSE);
}

void getsysvar(uchar *osverlo, uchar *osverhi, dosdate **osdate,
	int *oscountry)
{
	supervisor(TRUE);	
	sysbase = *_SYSBASE;
	while (sysbase != sysbase->os_base)
		sysbase = sysbase->os_base;
	supervisor(FALSE);

	*osverlo = sysbase->os_version & 0xff;
	*osverhi = sysbase->os_version >> 8;
	*osdate = (dosdate *)&(sysbase->os_gendatg);
	*oscountry = sysbase->os_palmode >> 1;
}

/*
 * getactpd(void)
 * return a pointer to the basepage of the currently running
 * process.
 * For TOS 1.2 and higher, use the SYSHDR structure, for TOS 1.0
 * use the value at 0x602c (Worldwide) or 0x873c (Spain).
 * This is used to give child processes spawned via system() the
 * correct (i.e. not our own) parent process pointer in their xArg
 * structure (see setupxarg() in exec.c).
 */
BASPAG *getactpd(void)
{
	SYSHDR *sys = sysbase;
	
	if (tosversion() >= 0x102)
		return *sys->_run;
	else
	{
		if ((sys->os_palmode>>1) == SPA)
			return *OS_ACT_PD_SPA;
		else
			return *OS_ACT_PD;
	}
}

void setactpd(BASPAG *bp)
{
	SYSHDR *sys = sysbase;

	if (tosversion() >= 0x102)
		*sys->_run = bp;
	else
	{
		if ((sys->os_palmode>>1) == SPA)
			*OS_ACT_PD_SPA = bp;
		else
			*OS_ACT_PD = bp;
	}
}

void *getcookiejar(void)
{
	void *cj;

	supervisor(TRUE);	
	cj = *_P_COOKIES;
	supervisor(FALSE);
	return cj;
}

void initcookie(void)
{
	long *cj = getcookiejar();
	long jarsize, cookie = 0;
	
	if (getvarint("shellcount")!= 1 || !legalcookie(cj))
		return;
	
	while (*cj != 0)
	{
		if (!strncmp((char *)cj,XBRA_ID,4))
			return;
		cj += 2;
		++cookie;
	}
	jarsize = *(cj+1);
	if (cookie == jarsize)
		return;
	strncpy((char *)cj,XBRA_ID,4);
	*(cj+1) = ((MUPFELVERSION[0]-'0')<<8) | (MUPFELVERSION[2]-'0');
	
	*(cj+2) = 0L;
	*(cj+3) = jarsize;
}

void removecookie(void)
{
	long *mycookie = NULL, *cj = getcookiejar();
	long jarsize;
	
	if (getvarint("shellcount") != 1 || !legalcookie(cj))
		return;
	
	while (*cj != 0)
	{
		if (!strncmp((char *)cj,XBRA_ID,4))
			mycookie = cj;
		cj += 2;
	}
	if (mycookie == NULL)
		return;
	jarsize = *(cj+1);
	memmove(mycookie,mycookie+2,(char *)cj-(char *)mycookie);
	*(cj-1) = jarsize;
}

#if MERGED
/*
 * coninit(void)
 * initialize XBRA structure for _o_con and _o_rawcon
 */
void coninit(void)
{
	/* These are defined in conio.s */
	extern int conout(char c);
	extern int rconout(char c);

	xbrainst(&ocon,_O_CON,conout);	
	xbrainst(&rocon,_O_RAWCON,rconout);

	/* remember we touched these vectors, used in m_exit() */
	++conwindow;
}

/*
 * conexit(void)
 * reset _o_con and _o_rawcon to original values
 */
void conexit(void)
{
	xbradeinst(&ocon,_O_CON);
	xbradeinst(&rocon,_O_RAWCON);

	/* forget the pointers */
	--conwindow;
}

void cursinit(void)
{
	int cpu;
	extern void MyXBios(void), MyXBios030(void);
	
	supervisor(TRUE);
	cpu = *_CPUTYPE;
	supervisor(FALSE);
	if (!cpu)
		xbrainst(&xb_xbios,XBIOS,MyXBios);
	else	
		xbrainst(&xb_xbios,XBIOS,MyXBios030);
	OldXBios = xb_xbios.xb_oldvec;
}

void cursexit(void)
{
	xbradeinst(&xb_xbios,XBIOS);
}
#endif

int getclick(void)
{
	int click;
	
	supervisor(TRUE);
	click = *CONTERM & 1;
	supervisor(FALSE);
	return click;
}

void keyclick(int on)
{
	supervisor(TRUE);
	if (on)
		*CONTERM |= 1;
	else
		*CONTERM &= ~1;
	supervisor(FALSE);
}

#if MERGED
void setLAxy(int x, int y)
{
	extern char *_vdiesc;		/* from conio.s */
	
	if (!savedLA)
	{
		xold = *(int *)&_vdiesc[-0x2c];
		yold = *(int *)&_vdiesc[-0x2a];
		savedLA = TRUE;
	}
	*(int *)&_vdiesc[-0x2c] = x;
	*(int *)&_vdiesc[-0x2a] = y;
}

void resetLAxy(void)
{
	extern char *_vdiesc;		/* from conio.s */

	*(int *)&_vdiesc[-0x2c] = xold;
	*(int *)&_vdiesc[-0x2a] = yold;
	savedLA = FALSE;
}
#endif