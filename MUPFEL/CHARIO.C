/*
 * chario.c  -  Character I/O primitives
 * 27.01.91
 */

#include <stdio.h> 
#include <string.h>
#include <tos.h>

#include "alert.h"
#include "batch.h"
#include "chario.h"
#include "comm.h"
#include "gemsubs.h"
#include "handle.h"
#include "keys.h"
#include "mupfel.h"
#include "redir.h"
#include "stand.h"
#include "sysvec.h"
#include "version.h"

#define ALTERNATE	0x08		/* bit to test in Kbshift() value */
#define CON		2 		/* BIOS screen device */
#define RAWCON		5		/* BIOS raw screen */
#define TYPEAHEADMAX	128	/* no of typeahead characters */

#define altpressed()	(Kbshift(-1) & ALTERNATE)
#define charpresent()	(Bconstat(CON)!=0)
#define waitaltchr()	while (altpressed() && !charpresent())
#define mkchr(a)		((char)(a & 0xFF)-'0')

SCCS(chario);

static int ctrlc = FALSE, wasctrlc = FALSE;
static int tapin = -1, tapout = -1;	/* typeahead pointers */
static long tabuf[TYPEAHEADMAX];		/* typeahead buffer */

static void (*gemdosout)(int c);

#if MERGED
extern void disp_rawchar(char ch);		/* Window raw i/o */
extern void disp_canchar(char ch);		/* Window canonical i/o */
static char errmsg[300];
#endif

static void doCconws(int c)
{
	static char *str = "x";
	
	*str = (char)c;
	Cconws(str);
}

void initoutput(void)
{
	if (tosversion()<0x104)
		gemdosout = doCconws;
	else
		gemdosout = Cconout;
}

int intr(void)
{
	return ctrlc;
}

int wasintr(void)
{
	return wasctrlc;
}

void resetintr(void)
{
	wasctrlc = ctrlc;
	ctrlc = FALSE;
}

int checkintr(void)
{
	char c = '\0';
	
	if (Bconstat(CON)!=0)
		c = Bconin(CON) & 0xFF;
	return c==CTRLC || c==ESC;
}

static void typeahead(long key)
{
	tabuf[++tapin] = key;
	if (tapin == TYPEAHEADMAX-1)
		tapin = -1;
}

static void checkxoff(void)
{
	long key;
	char ch;
	int xon;
	
	if (!Bconstat(CON))
		return;

	key = Bconin(CON);
	ch = (char)key;

	if (ch==CTRLS)
		do
		{
			key=Bconin(CON);
			xon=(char)key == CTRLQ || (char)key == CTRLC;
			if (!xon)
				typeahead(key);
		}
		while (!xon);

	ch = (char)key;
	if (!ctrlc)
		ctrlc = (ch==CTRLC);
	if (ch!=CTRLS && ch!=CTRLC && ch!=CTRLQ)
		typeahead(key);
}

void rawout(char c)
{
	if ((redirect.out.hnd < MINHND) && !shellcmd)
	{
		checkxoff();
#if MERGED
		if (conwindow)
			disp_rawchar(c);
		else
#endif
			Bconout(RAWCON,c);
	}
	else		
		gemdosout(c);
}

void canout(char c)
{
	if ((redirect.out.hnd < MINHND) && !shellcmd)
	{
		checkxoff();
#if MERGED
		if (conwindow)
			disp_canchar(c);
		else
#endif
			Bconout(CON,c);
	}
	else		
		gemdosout(c);
}

void rawoutn(char c,unsigned int n)
{
#if MERGED
	char tmpstr[301];

	while (n>300)
	{	
		memset(tmpstr,c,300);
		tmpstr[300]='\0';
		mprintf(tmpstr);
		n -= 300;
	}
	memset(tmpstr,c,n);
	tmpstr[n]='\0';
	mprintf(tmpstr);
#else
	while (n--)
		rawout(c);
#endif
}

void crlf(void)
{
	canout(CR);
	canout(LF);
}

void beep(void)
{
	canout(BELL);
}

/* VT-52 escape sequence */
void vt52(char c)
{
	canout(ESC);
	canout(c);
}

/* VT-52 cursor motion sequence */
void vt52cm(int x,int y)
{
	vt52('Y');
	canout(y+' ');
	canout(x+' ');
}

int inbuffchar(long *lp)
{
	if (tapin >= 0 && tapout < tapin)
	{
		*lp = tabuf[++tapout];
		if (tapout >= tapin)
			tapout = tapin = -1;
		return TRUE;
	}
	return FALSE;
}

long inchar(void)
{
	char c;
	long l, l1, key;

	if (inbuffchar(&key))
		return key;
		
loop:		
	c=NUL;
	l=0L;
	
	if (!charpresent())
	{
		while (altpressed())
		{
			waitaltchr();
			if (altpressed() && charpresent())
			{
				l1=Bconin(CON);
				c=mkchr(l1);
				if (c<0 || c>9)
					return l1;
				l=(10*l+c) & 0xFF;
			}
		}
		if (l!=0L)
			return l;
		else
			goto loop;
	}
	else
		return Bconin(CON);
}

static void domprint(char *str,int len)
{
	char *nl = strchr(str,'\n');
	int lastnl;
	
	lastnl = (nl==NULL || nl==&str[len-1]);

#if MERGED
	if (conwindow && redirect.out.hnd < MINHND && lastnl && !shellcmd)
	{
		if (nl)
			*nl = '\0';
		disp_string(str);
		if (nl)
		{
			*nl = '\n';
			crlf();
		}
	}
	else
#endif
		if (redirect.out.hnd >= MINHND && lastnl)
		{
			if (nl)
				*nl='\0';
			Cconws(str);
			if (nl)
			{
				*nl='\n';
				crlf();
			}
		}
		else
		{
			while (*str)
			{
				if (*str=='\n')
					crlf();
				else
					rawout(*str);
				++str;
			}
		}
}

int mprintf(char *fmt,...)
{
	va_list argpoint;
	int len;
	char tmpstr[300];
	char *tp;

	if (strchr(fmt,'%'))
	{
		va_start(argpoint,fmt);
		len = vsprintf(tmpstr,fmt,argpoint);
		va_end(argpoint);
		tp = tmpstr;
	}
	else
	{
		tp = fmt;
		len = (int)strlen(fmt);
	}
	
	if (shellcmd && oserr)
	{
#if MERGED
		strcpy(errmsg,tp);
		CommInfo.errmsg = errmsg;
#endif
		sysret = oserr;
	}
	else
	{
#if MERGED
		if (isautoexec())
			alertstr(tp);
		else
#endif
			domprint(tp,len);
	}
	oserr = 0;
	return len;
}

int eprintf(char *fmt,...)
{
	va_list argpoint;
	int len;
	char tmpstr[300];
	char *tp;

	if (strchr(fmt,'%'))
	{
		va_start(argpoint,fmt);
		len = vsprintf(tmpstr,fmt,argpoint);
		va_end(argpoint);
		tp = tmpstr;
	}
	else
	{
		tp = fmt;
		len = (int)strlen(fmt);
	}

	if (shellcmd && oserr)
	{
#if MERGED
		strcpy(errmsg,tp);
		CommInfo.errmsg = errmsg;
#endif
		sysret = oserr;
	}
	else
	{
		if (isautoexec())
			alertstr(tp);
		else
		{
			while (*tp)
			{
				if (*tp == '\n')
				{
					Cauxout('\r');
					Cauxout('\n');
				}
				else
					Cauxout(*tp);
				++tp;
			}
		}
	}
	oserr = 0;
	return len;
}

/* Debug Version of mprint. Uses RAWCON regardless of redirection */
void dprint(char *fmt,...)
{
	va_list argpoint;
	char tmp[300], *tp;
	
	va_start(argpoint,fmt);
	vsprintf(tmp,fmt,argpoint);
	va_end(argpoint);
	tp = tmp;
	while (*tp)
	{
		if (*tp=='\n')
		{
			Bconout(CON,'\r');
			Bconout(CON,'\n');
		}
		else
			Bconout(RAWCON,*tp);
		++tp;
	}
}
